/*
    Playground - An active arena to learn multi-robots programming
    Copyright (C) 1999--2013:
        Stephane Magnenat <stephane at magnenat dot net>
        (http://stephane.magnenat.net)
    3D models
    Copyright (C) 2008:
        Basilio Noris
    Aseba - an event-based framework for distributed robot control
    Copyright (C) 2007--2013:
        Stephane Magnenat <stephane at magnenat dot net>
        (http://stephane.magnenat.net)
        and other contributors, see authors.txt for details
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published
    by the Free Software Foundation, version 3 of the License.
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.
    You should have received a copy of the GNU Lesser General Public License
    along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#include "common/utils/FormatableString.h"
#include "DashelAsebaGlue.h"
#include "Door.h"
#include "PlaygroundViewer.h"
#include "Robots.h"
#include <QtXml>
#include <QApplication>
#include <QFileDialog>
#include <QMessageBox>
#include <QProcess>
#include <QString>
#include <QDesktopServices>
#include <QDir>
#include <QHash>
#include <QHostInfo>
#include <utility>
#include <quazip.h>
#include <quazipfile.h>
#include <ctime>
#include <map>
#include "enki/Geometry.h"
#include "PhysicalEngine.h"


#ifdef HAVE_DBUS
#    include "PlaygroundDBusAdaptors.h"
#endif  // HAVE_DBUS

#ifdef ZEROCONF_SUPPORT
#    include "common/zeroconf/zeroconf-qt.h"
#endif  // ZEROCONF_SUPPORT

class IRMessage {
public:
    std::string robotId;
    int16_t message;
    time_t irmsgtime;
    Enki::Point position1;
    double orint;
    bool empty;
    

public:
    IRMessage(): empty(true){}
    IRMessage(std::string robotId, int16_t message, time_t irmsgtime, Enki::Point position1, double orint): robotId( robotId ), message( message ), irmsgtime( irmsgtime ), position1(position1), orint(orint), empty(false) {}

    int16_t getMessage(){
        return message;
    }

    std::string getRobotId(){
        return robotId;
    }  

    time_t getMsgtime(){
     //   irmsgtime = time(0);
        return irmsgtime;
    }

    Enki::Point getPos(){
        return position1;
     
    }

    double getorint(){
        return orint;
    }

    bool isEmpty(){
        return empty;
    }
};

class MessageQueue {
public:
    IRMessage queue[32];
    int8_t last_index; 
    int8_t iteratorI;  
    int8_t length;

public:
    MessageQueue(int8_t length){
        if ( length > 0 && length <= 32 ) {
            this->length = length;
        } else {
            this->length = 32;
        }

        iteratorI = -1 ;
        last_index = 0 ;
    }

    IRMessage getCurrent(){
        if ( iteratorI >= 0 ){
            return queue[iteratorI];
        } else {
            return IRMessage();
        }
    }

    IRMessage getNext(){
        if ( iteratorI < last_index){
            iteratorI = iteratorI + 1;
        } else {
            return IRMessage();
        }
        if ( iteratorI >= 0 ){
            return queue[iteratorI];
        } else {
            return IRMessage();
        }
    }

    IRMessage top(){
        if ( last_index > 0 ){
            iteratorI = 0;
            return queue[iteratorI];
        } else {
            iteratorI = -1;
        }
        return IRMessage();
    }

    void push( IRMessage message ){
        if (  last_index >= 0 ){
            if ( last_index < length - 1 ) {
                last_index = last_index + 1;
            }
            for ( int8_t i=last_index; i > 0 ; i-- ){
                queue[i] = queue[i-1];
            }
            queue[0] = message;
        } 
    }

    void removeCurrent(){
        for ( int8_t i=iteratorI; i < last_index ; i++ ){
            queue[i] = queue[i+1];
        }
        queue[last_index] = IRMessage();
        iteratorI = iteratorI - 1;
        last_index = last_index - 1;
    } 

    bool empty(){
        return last_index==0;
    }

};

namespace Enki {
//! The simulator environment for playground

class PlaygroundSimulatorEnvironment : public SimulatorEnvironment {
public:
    const QString sceneFileName;
    PlaygroundViewer& viewer;
    MessageQueue commBuffer;

public:
    PlaygroundSimulatorEnvironment(QString sceneFileName, PlaygroundViewer& viewer)
        : sceneFileName(std::move(sceneFileName)), viewer(viewer), commBuffer( MessageQueue(8) ) {}

    void notify(const EnvironmentNotificationType type, const std::string& description,
                const strings& arguments) override {
        viewer.notifyAsebaEnvironment(type, description, arguments);
    }

    std::string getSDFilePath(const std::string& robotName, unsigned fileNumber) const override {
        auto paths = QStandardPaths::standardLocations(QStandardPaths::DataLocation);
        auto fileName(QString("%1/%2/%3/U%4.DAT")
                          .arg(paths.empty() ? "" : paths.first())
                          .arg(qHash(QDir(sceneFileName).canonicalPath()), 8, 16, QChar('0'))
                          .arg(QString::fromStdString(robotName))
                          .arg(fileNumber));
        QDir().mkpath(QFileInfo(fileName).absolutePath());
        // dump
        std::cerr << fileName.toStdString() << std::endl;
        return fileName.toStdString();
    }

    World* getWorld() const override {
        return viewer.getWorld();
    }

void sendIRMessage(std::string robotName, int16_t message, Point position1, double orint1) override{
        World* world = viewer.getWorld();
        for( World::ObjectsIterator i = world->objects.begin(); i != world->objects.end(); ++i) {
			PhysicalObject* o = *i;
            auto* thymio2(dynamic_cast<AsebaThymio2*>(o));

            if (thymio2){
                std::string robotName2 = thymio2->robotName;
                Point position2 = Point(thymio2->pos.x, thymio2->pos.y);
                if ( robotName == robotName2 ){
                    continue;
                }

                Enki::Vector distance(position1.x-position2.x , position1.y-position2.y);
               	double distance_final= distance.norm();
                double vector_angle1 = distance.angle();

                double vector_angle = vector_angle1*(180/M_PI);

                double anglee1 = (thymio2->angle)*(180/M_PI);
                double orient_deg = orint1 * (180/M_PI);
              
	//	if (vector_angle<0){ vector_angle=vector_angle+180; }

//std::cerr << " Retta " << vector_angle << " Orientamento rispetto robot RX " << vector_angle - anglee1 << " Orientamento rispetto robot TX " << vector_angle - orient_deg << " robot name " << robotName << " \n";

                if (distance_final<=25.0 )
                {   

                    if((vector_angle <= anglee1+70.0 && vector_angle >= anglee1-70.0) || (vector_angle >= anglee1+130.0 && vector_angle <= anglee1+230.0))   
                {     
                    if((vector_angle <= orient_deg+70.0 && vector_angle>= orient_deg-70.0) || (vector_angle>= orient_deg+130.0 && vector_angle<= orient_deg+230.0)){ 


                    thymio2->setIRMessageReceived(message);                    
                }   }  
                } 
            }
		}
    
    }
   
};
}  // namespace Enki

using RobotFactory = std::function<Enki::Robot*(QString, QString, unsigned&, int16_t)>;
template <typename RobotT>
Enki::Robot* createRobotSingleVMNode(QString robotName, QString typeName, unsigned& port, int16_t nodeId) {
    return new RobotT(typeName, robotName, port, nodeId);
}

//! A type of robot
struct RobotType {
    RobotType(QString prettyName, RobotFactory factory)
        : prettyName(std::move(prettyName)), factory(std::move(factory)) {}
    const QString prettyName;    //!< a nice-looking name of this type
    const RobotFactory factory;  //!< the factory function to create a robot of this type
    unsigned number = 0;         //!< number of robots of this type instantiated
};


int main(int argc, char* argv[]) {
    Q_INIT_RESOURCE(asebaqtabout);
    QApplication app(argc, argv);
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QCoreApplication::setOrganizationName("mobsya");
    QCoreApplication::setOrganizationDomain(ASEBA_ORGANIZATION_DOMAIN);
    app.setApplicationName("Playground");


    // Translation support
    QTranslator qtTranslator;
    qtTranslator.load("qtbase_" + QLocale::system().name());
    app.installTranslator(&qtTranslator);

    QTranslator translator;
    translator.load(QString(":/asebaplayground_") + QLocale::system().name());
    app.installTranslator(&translator);

    QTranslator aboutTranslator;
    aboutTranslator.load(QString(":/qtabout_") + QLocale::system().name());
    app.installTranslator(&aboutTranslator);

    // create document
    QDomDocument domDocument("aseba-playground");
    QString sceneFileName;
    QuaZip zipFile;

    // Get cmd line arguments
    bool ask = true;
    if(argc > 1) {
        sceneFileName = argv[1];
        ask = false;
    }

    // Try to load xml config file
    while(ask) {
        QString lastFileName = QSettings("EPFL-LSRO-Mobots", "Aseba Playground").value("last file").toString();
        if(lastFileName.isEmpty()) {


// On windows go look for scenarios in the examples folder
#ifdef Q_OS_WIN32
            lastFileName = QCoreApplication::applicationDirPath() + "/../examples/";
#else
            auto loc = QStandardPaths::standardLocations(QStandardPaths::HomeLocation);
            if(!loc.empty())
                lastFileName = loc.first();
#endif
        }

        sceneFileName = QFileDialog::getOpenFileName(nullptr, QObject::tr("Open Scenario"), lastFileName,
                                                     QObject::tr("playground scenario") + " (*.playground)");
        ask = true;

        if(sceneFileName.isEmpty()) {
            std::cerr << "You must specify a valid setup scenario on the command line or choose "
                         "one in the file dialog."
                      << std::endl;
            exit(1);
        }

        QString data;
        QFile file(sceneFileName);
        if(!file.open(QIODevice::ReadOnly)) {
            QMessageBox::information(nullptr, "Aseba Playground",
                                     QObject::tr("Unable to open file %1").arg(sceneFileName));
            continue;
        };
        // Try zip
        zipFile.setZipName(sceneFileName);
        if(zipFile.open(QuaZip::Mode::mdUnzip)) {
            zipFile.setCurrentFile("world.xml");
            QuaZipFile entry(&zipFile);
            entry.open(QIODevice::ReadOnly);
            data = entry.readAll();
        } else {
            data = file.readAll();
        }


        QString errorStr;
        int errorLine, errorColumn;


        if(!domDocument.setContent(data, false, &errorStr, &errorLine, &errorColumn)) {
            QMessageBox::information(nullptr, "Aseba Playground",
                                     QObject::tr("Parse error at file %1, line %2, column %3:\n%4")
                                         .arg(sceneFileName)
                                         .arg(errorLine)
                                         .arg(errorColumn)
                                         .arg(errorStr));
        } else {
            QSettings("EPFL-LSRO-Mobots", "Aseba Playground").setValue("last file", sceneFileName);
            break;
        }
    }

    // Scan for colors
    typedef QMap<QString, Enki::Color> ColorsMap;
    ColorsMap colorsMap;
    QDomElement colorE = domDocument.documentElement().firstChildElement("color");
    while(!colorE.isNull()) {
        colorsMap[colorE.attribute("name")] = Enki::Color(
            colorE.attribute("r").toDouble(), colorE.attribute("g").toDouble(), colorE.attribute("b").toDouble());

        colorE = colorE.nextSiblingElement("color");
    }

    // Scan for areas
    typedef QMap<QString, Enki::Polygon> AreasMap;
    AreasMap areasMap;
    QDomElement areaE = domDocument.documentElement().firstChildElement("area");
    while(!areaE.isNull()) {
        Enki::Polygon p;
        QDomElement pointE = areaE.firstChildElement("point");
        while(!pointE.isNull()) {
            p.push_back(Enki::Point(pointE.attribute("x").toDouble(), pointE.attribute("y").toDouble()));
            pointE = pointE.nextSiblingElement("point");
        }
        areasMap[areaE.attribute("name")] = p;
        areaE = areaE.nextSiblingElement("area");
    }

    // Create the world
    QDomElement worldE = domDocument.documentElement().firstChildElement("world");
    Enki::Color worldColor(Enki::Color::gray);
    if(!colorsMap.contains(worldE.attribute("color")))
        std::cerr << "Warning, world walls color " << worldE.attribute("color").toStdString() << " undefined\n";
    else
        worldColor = colorsMap[worldE.attribute("color")];
    Enki::World::GroundTexture groundTexture;
    if(worldE.hasAttribute("groundTexture")) {
        const QString textureName = worldE.attribute("groundTexture");
        QImage image;
        if(zipFile.isOpen() && zipFile.setCurrentFile(textureName)) {
            QuaZipFile entry(&zipFile);
            entry.open(QIODevice::ReadOnly);
            image.load(&entry, QFileInfo(textureName).suffix().toUtf8().data());
        } else if(!zipFile.isOpen()) {
            const QString groundTextureFileName(QFileInfo(sceneFileName).absolutePath() + QDir::separator() +
                                                textureName);
            image.load(groundTextureFileName);
        }

        if(!image.isNull()) {
            // flip vertically as y-coordinate is inverted in an image
            image = image.mirrored();
            // convert to a specific format and copy the underlying data to Enki
            image = image.convertToFormat(QImage::Format_ARGB32);
            groundTexture.width = image.width();
            groundTexture.height = image.height();
            const auto* imageData(reinterpret_cast<const uint32_t*>(image.constBits()));
            std::copy(imageData, imageData + image.width() * image.height(), std::back_inserter(groundTexture.data));
            // Note: this works in little endian, in big endian data should be swapped
        } else {
            qDebug() << "Could not load ground texture file named" << textureName;
        }
    }
    Enki::World world(worldE.attribute("w").toDouble(), worldE.attribute("h").toDouble(), worldColor, groundTexture);

    // Create viewer
    Enki::PlaygroundViewer viewer(&world, worldE.attribute("energyScoringSystemEnabled", "false").toLower() == "true");
    if(Enki::simulatorEnvironment)
        qDebug() << "A simulator environment already exists, replacing";
    Enki::simulatorEnvironment.reset(new Enki::PlaygroundSimulatorEnvironment(sceneFileName, viewer));

    // Zeroconf support to advertise targets
#ifdef ZEROCONF_SUPPORT
    Aseba::QtZeroconf zeroconf;
#endif  // ZEROCONF_SUPPORT

    // Scan for camera
    QDomElement cameraE = domDocument.documentElement().firstChildElement("camera");
    if(!cameraE.isNull()) {
        const double largestDim(qMax(world.h, world.w));
        viewer.setCamera(QPointF(cameraE.attribute("x", QString::number(world.w / 2)).toDouble(),
                                 cameraE.attribute("y", QString::number(0)).toDouble()),
                         cameraE.attribute("altitude", QString::number(0.85 * largestDim)).toDouble(),
                         cameraE.attribute("yaw", QString::number(-M_PI / 2)).toDouble(),
                         cameraE.attribute("pitch", QString::number((3 * M_PI) / 8)).toDouble());
    }

    // Scan for walls
    QDomElement wallE = domDocument.documentElement().firstChildElement("wall");
    while(!wallE.isNull()) {
        auto* wall = new Enki::PhysicalObject();
        if(!colorsMap.contains(wallE.attribute("color")))
            std::cerr << "Warning, color " << wallE.attribute("color").toStdString() << " undefined\n";
        else
            wall->setColor(colorsMap[wallE.attribute("color")]);
        wall->pos.x = wallE.attribute("x").toDouble();
        wall->pos.y = wallE.attribute("y").toDouble();
        wall->setRectangular(
            wallE.attribute("l1").toDouble(), wallE.attribute("l2").toDouble(), wallE.attribute("h").toDouble(),
            !wallE.attribute("mass").isNull() ? wallE.attribute("mass").toDouble() : -1  // normally -1 because immobile
        );
        if(!wallE.attribute("angle").isNull())
            wall->angle = wallE.attribute("angle").toDouble();  // radians
        world.addObject(wall);

        wallE = wallE.nextSiblingElement("wall");
    }

    // Scan for cylinders
    QDomElement cylinderE = domDocument.documentElement().firstChildElement("cylinder");
    while(!cylinderE.isNull()) {
        auto* cylinder = new Enki::PhysicalObject();
        if(!colorsMap.contains(cylinderE.attribute("color")))
            std::cerr << "Warning, color " << cylinderE.attribute("color").toStdString() << " undefined\n";
        else
            cylinder->setColor(colorsMap[cylinderE.attribute("color")]);
        cylinder->pos.x = cylinderE.attribute("x").toDouble();
        cylinder->pos.y = cylinderE.attribute("y").toDouble();
        cylinder->setCylindric(cylinderE.attribute("r").toDouble(), cylinderE.attribute("h").toDouble(),
                               !cylinderE.attribute("mass").isNull() ? cylinderE.attribute("mass").toDouble() :
                                                                       -1  // normally -1 because immobile
        );
        world.addObject(cylinder);

        cylinderE = cylinderE.nextSiblingElement("cylinder");
    }

    // Scan for feeders
    QDomElement feederE = domDocument.documentElement().firstChildElement("feeder");
    while(!feederE.isNull()) {
        auto* feeder = new Enki::EPuckFeeder;
        feeder->pos.x = feederE.attribute("x").toDouble();
        feeder->pos.y = feederE.attribute("y").toDouble();
        world.addObject(feeder);

        feederE = feederE.nextSiblingElement("feeder");
    }
    // TODO: if needed, custom color to feeder

    // Scan for doors
    typedef QMap<QString, Enki::SlidingDoor*> DoorsMap;
    DoorsMap doorsMap;
    QDomElement doorE = domDocument.documentElement().firstChildElement("door");
    while(!doorE.isNull()) {
        Enki::SlidingDoor* door = new Enki::SlidingDoor(
            Enki::Point(doorE.attribute("closedX").toDouble(), doorE.attribute("closedY").toDouble()),
            Enki::Point(doorE.attribute("openedX").toDouble(), doorE.attribute("openedY").toDouble()),
            Enki::Point(doorE.attribute("l1").toDouble(), doorE.attribute("l2").toDouble()),
            doorE.attribute("h").toDouble(), doorE.attribute("moveDuration").toDouble());
        if(!colorsMap.contains(doorE.attribute("color")))
            std::cerr << "Warning, door color " << doorE.attribute("color").toStdString() << " undefined\n";
        else
            door->setColor(colorsMap[doorE.attribute("color")]);
        doorsMap[doorE.attribute("name")] = door;
        world.addObject(door);

        doorE = doorE.nextSiblingElement("door");
    }

    // Scan for activation, and link them with areas and doors
    QDomElement activationE = domDocument.documentElement().firstChildElement("activation");
    while(!activationE.isNull()) {
        if(areasMap.find(activationE.attribute("area")) == areasMap.end()) {
            std::cerr << "Warning, area " << activationE.attribute("area").toStdString() << " undefined\n";
            activationE = activationE.nextSiblingElement("activation");
            continue;
        }

        if(doorsMap.find(activationE.attribute("door")) == doorsMap.end()) {
            std::cerr << "Warning, door " << activationE.attribute("door").toStdString() << " undefined\n";
            activationE = activationE.nextSiblingElement("activation");
            continue;
        }

        const Enki::Polygon& area = *areasMap.find(activationE.attribute("area"));
        Enki::Door* door = *doorsMap.find(activationE.attribute("door"));

        Enki::DoorButton* activation = new Enki::DoorButton(
            Enki::Point(activationE.attribute("x").toDouble(), activationE.attribute("y").toDouble()),
            Enki::Point(activationE.attribute("l1").toDouble(), activationE.attribute("l2").toDouble()), area, door);

        world.addObject(activation);

        activationE = activationE.nextSiblingElement("activation");
    }

    // load all robots in one loop
    std::map<QString, RobotType> robotTypes{
        {"thymio2", RobotType{"Thymio II", createRobotSingleVMNode<Enki::DashelAsebaThymio2>}},
        {"e-puck", RobotType{"E-Puck", createRobotSingleVMNode<Enki::DashelAsebaFeedableEPuck>}},
    };
    QDomElement robotE = domDocument.documentElement().firstChildElement("robot");
    unsigned asebaServerCount(0);
    while(!robotE.isNull()) {
        const auto type(robotE.attribute("type", "thymio2"));
        auto typeIt(robotTypes.find(type));
        if(typeIt != robotTypes.end()) {
            // retrieve informations
            const auto& cppTypeName(typeIt->second.prettyName);
            const auto qTypeName(cppTypeName);
            auto& countOfThisType(typeIt->second.number);
            const auto qRobotNameRaw(robotE.attribute("name", QString("%1 %2").arg(qTypeName).arg(countOfThisType)));
            const auto qRobotNameFull(QObject::tr("%2 on %3").arg(qRobotNameRaw).arg(QHostInfo::localHostName()));
            const auto cppRobotName(qRobotNameFull.toStdString());
            unsigned port = 0;
            const int16_t nodeId(robotE.attribute("nodeId", "1").toInt());

            // create
            const auto& creator(typeIt->second.factory);
            auto robot(creator(qRobotNameFull, cppTypeName, port, nodeId));
            asebaServerCount++;
            countOfThisType++;

            // setup in the world
            robot->pos.x = robotE.attribute("x").toDouble();
            robot->pos.y = robotE.attribute("y").toDouble();
            robot->angle = robotE.attribute("angle").toDouble();
            world.addObject(robot);

            // log
            viewer.log(QObject::tr("New robot %0 of type %1 on port %2").arg(qRobotNameRaw).arg(qTypeName).arg(port),
                       Qt::white);
        } else
            viewer.log("Error, unknown robot type " + type, Qt::red);

        robotE = robotE.nextSiblingElement("robot");
    }

    // Scan for external processes
    QList<QProcess*> processes;

    // Show and run
    viewer.setWindowTitle(QObject::tr("Aseba Playground - Simulate your robots!"));
    viewer.show();

// If D-Bus is used, register the viewer object
#ifdef HAVE_DBUS
    new Enki::EnkiWorldInterface(&viewer);
    QDBusConnection::sessionBus().registerObject("/world", &viewer);
    QDBusConnection::sessionBus().registerService("ch.epfl.mobots.AsebaPlayground");
#endif  // HAVE_DBUS

    // Run the application
    const int exitValue(app.exec());

    // Stop and delete ongoing processes
    foreach(QProcess* process, processes) {
        process->terminate();
        if(!process->waitForFinished(1000))
            process->kill();
        delete process;
    }

    return exitValue;
}
