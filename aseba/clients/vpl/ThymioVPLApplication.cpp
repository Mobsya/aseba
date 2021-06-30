/*
    Aseba - an event-based framework for distributed robot control
    Created by Stéphane Magnenat <stephane at magnenat dot net> (http://stephane.magnenat.net)
    with contributions from the community.
    Copyright (C) 2007--2018 the authors, see authors.txt for details.

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

#include "ThymioVPLApplication.h"
#include "AeslEditor.h"
#include "ThymioVisualProgramming.h"
#include "common/consts.h"
#include "common/productids.h"
#include "translations/CompilerTranslator.h"
#include <QMessageBox>
#include <QVBoxLayout>
#include <QFileDialog>
#include <QScrollBar>
#include <QSplitter>
#include <QShortcut>
#include <QDesktopServices>
#include <QLabel>
#include <qt-thymio-dm-client-lib/thymiodevicemanagerclient.h>

namespace Aseba {

ThymioVPLApplication::ThymioVPLApplication(mobsya::ThymioDeviceManagerClient* client, const QUuid& thymioId)
    : m_client(client), m_thymioId(thymioId), vpl(nullptr), allocatedVariablesCount(0) {

    connect(m_client, &mobsya::ThymioDeviceManagerClient::nodeAdded, this, &ThymioVPLApplication::onNodeChanged);
    connect(m_client, &mobsya::ThymioDeviceManagerClient::nodeRemoved, this, &ThymioVPLApplication::onNodeChanged);
    connect(m_client, &mobsya::ThymioDeviceManagerClient::nodeModified, this, &ThymioVPLApplication::onNodeChanged);

    // create gui
    setupWidgets();
    setupConnections();
    setWindowIcon(QIcon(":/images/icons/thymiovpl.svgz"));

// resize if not android
#ifndef ANDROID
    resize(1000, 700);
#endif  // ANDROID
}

ThymioVPLApplication::~ThymioVPLApplication() {}


void ThymioVPLApplication::displayCode(const QList<QString>& code, int elementToHighlight) {
    editor->replaceAndHighlightCode(code, elementToHighlight);
    const auto text = editor->toPlainText();
    if(m_thymio)
        m_thymio->setScratchPad(text.toUtf8());
}

void ThymioVPLApplication::loadAndRun() {
    if(!m_thymio)
        return;
    auto code = editor->toPlainText();
    m_compilation_watcher.setRequest(m_thymio->load_aseba_code(code.toUtf8()));
    connect(
        &m_compilation_watcher, &mobsya::CompilationRequestWatcher::finished, this, [this]() { m_thymio->run(); },
        Qt::UniqueConnection);
}

void ThymioVPLApplication::stop() {
    if(!m_thymio)
        return;
    m_thymio->load_aseba_code({});
    m_thymio->stop();
}

bool ThymioVPLApplication::newFile() {
    Q_ASSERT(vpl);
    if(vpl->preDiscardWarningDialog(false)) {
        fileName = "";
        return true;
    }
    return false;
}

void ThymioVPLApplication::setupWidgets() {
    // VPL part
    updateWindowTitle(false);
    vplLayout = new QVBoxLayout;
#ifdef Q_WS_MACX
    vplLayout->setContentsMargins(0, 0, 0, 0);
#endif  // Q_WS_MACX
    disconnectedMessage = new QLabel(tr("Connecting to Thymio..."));
    disconnectedMessage->setAlignment(Qt::AlignCenter);
    disconnectedMessage->setWordWrap(true);
    vplLayout->addWidget(disconnectedMessage);
    QWidget* vplContainer = new QWidget;
    vplContainer->setLayout(vplLayout);

    addWidget(vplContainer);

    // editor part
    editor = new AeslEditor;
    editor->setReadOnly(true);
#ifdef ANDROID
    editor->setTextInteractionFlags(Qt::NoTextInteraction);
    editor->setStyleSheet("QTextEdit { font-size: 10pt; font-family: \"Courrier\" }");
    editor->setTabStopWidth(QFontMetrics(editor->font()).width(' '));
#endif  // ANDROID
    new AeslHighlighter(editor, editor->document());

    QVBoxLayout* layout = new QVBoxLayout;
    layout->addWidget(editor, 1);
    const QString text =
        tr("Aseba ver. %0 (build %1/protocol %2);").arg(ASEBA_VERSION).arg(ASEBA_REVISION).arg(ASEBA_PROTOCOL_VERSION);
    QLabel* label(new QLabel(text));
    label->setWordWrap(true);
    label->setStyleSheet("QLabel { font-size: 8pt; }");
    layout->addWidget(label);
    QWidget* textWidget = new QWidget;
    textWidget->setLayout(layout);

    addWidget(editor);

    // shortcut
    QShortcut* shwHide = new QShortcut(QKeySequence("Ctrl+f"), this);
    connect(shwHide, SIGNAL(activated()), SLOT(toggleFullScreen()));
}

void ThymioVPLApplication::setupConnections() {


    // target events
    // connect(target.get(), SIGNAL(nodeConnected(unsigned)), SLOT(nodeConnected(unsigned)));
    // connect(target.get(), SIGNAL(nodeDisconnected(unsigned)), SLOT(nodeDisconnected(unsigned)));

    // connect(target.get(), SIGNAL(variablesMemoryEstimatedDirty(unsigned)),
    //        SLOT(variablesMemoryEstimatedDirty(unsigned)));
    // connect(target.get(), SIGNAL(variablesMemoryChanged(unsigned, unsigned, const
    // VariablesDataVector&)),
    //        SLOT(variablesMemoryChanged(unsigned, unsigned, const VariablesDataVector&)));
}

void ThymioVPLApplication::resizeEvent(QResizeEvent* event) {
    if(event->size().height() > event->size().width())
        setOrientation(Qt::Vertical);
    else
        setOrientation(Qt::Horizontal);
    QSplitter::resizeEvent(event);
    // resetSizes();
}

void ThymioVPLApplication::resetSizes() {
    // make sure that VPL is larger than the editor
    QList<int> sizes;
    if(orientation() == Qt::Vertical) {
        sizes.push_back((height() * 5) / 7);
        sizes.push_back((height() * 2) / 7);
    } else {
        sizes.push_back((width() * 2) / 3);
        sizes.push_back((width() * 1) / 3);
    }
    setSizes(sizes);
}

//! Received a close event, forward to VPL
void ThymioVPLApplication::closeEvent(QCloseEvent* event) {
    if(vpl && !vpl->closeFile())
        event->ignore();
}

//! Save a minimal but valid aesl file
bool ThymioVPLApplication::saveFile(bool as) {
    QSettings settings;

    // we need a valid VPL to save something
    if(!vpl)
        return false;

    // open file
    if(as || fileName.isEmpty()) {
        if(fileName.isEmpty()) {
            // get last file name

#ifdef ANDROID
            fileName = settings.value("ThymioVPLClassic/fileName", "/sdcard/").toString();
#else   // ANDROID
            fileName = settings
                           .value("ThymioVPLClassic/fileName",
                                  QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation))
                           .toString();
#endif  // ANDROID
        // keep only the path of the directory
            fileName = QFileInfo(fileName).dir().path();
        }
        fileName = QFileDialog::getSaveFileName(nullptr, tr("Save Script"), fileName, "Aseba scripts (*.aesl)");
    }

    if(fileName.isEmpty())
        return false;

    if(fileName.lastIndexOf(".") < 0)
        fileName += ".aesl";

    QFile file(fileName);
    if(!file.open(QFile::WriteOnly | QFile::Truncate))
        return false;

    // save file name to settings
    settings.setValue("ThymioVPLClassic/fileName", fileName);

    // initiate DOM tree
    QDomDocument document("aesl-source");
    QDomElement root = document.createElement("network");
    document.appendChild(root);

    // add a node for VPL
    QDomElement element = document.createElement("node");
    if(m_thymio) {
        element.setAttribute("name", m_thymio->name());
        element.setAttribute("nodeId", m_thymio->uuid().toString());
    }
    element.appendChild(document.createTextNode(editor->toPlainText()));

    if(m_thymio && m_thymio->group()) {
        for(auto&& e : m_thymio->group()->eventsDescriptions()) {
            QDomElement element = document.createElement("event");
            element.setAttribute("name", e.name());
            element.setAttribute("size", e.size());
            root.appendChild(element);
        }

        for(auto&& v : m_thymio->group()->sharedVariables().toStdMap()) {
            QDomElement element = document.createElement("constant");
            element.setAttribute("name",  v.first);
            element.setAttribute("value", v.second.value().toInt());
            root.appendChild(element);
        }
    }

    QDomDocument vplDocument(vpl->saveToDom());
    if(!vplDocument.isNull()) {
        QDomElement plugins = document.createElement("toolsPlugins");
        QDomElement plugin(document.createElement("ThymioVisualProgramming"));
        plugin.appendChild(document.importNode(vplDocument.documentElement(), true));
        plugins.appendChild(plugin);
        element.appendChild(plugins);
    }
    root.appendChild(element);

    QTextStream out(&file);
    document.save(out, 0);

    return true;
}

//! Load a aesl file
void ThymioVPLApplication::openFile() {
    // we need a valid VPL to save something
    if(!vpl)
        return;

    // ask user to save existing changes
    if(!vpl->preDiscardWarningDialog(false))
        return;

    QString dir;
    if(fileName.isEmpty()) {
        QSettings settings;
#ifdef ANDROID
        dir = settings.value("ThymioVPLClassic/fileName", "/sdcard/").toString();
#else   // ANDROID
        const QStringList stdLocations(QStandardPaths::standardLocations(QStandardPaths::DocumentsLocation));
        dir = settings.value("ThymioVPLClassic/fileName", !stdLocations.empty() ? stdLocations[0] : "").toString();
#endif  // ANDROID
    } else
        dir = fileName;

    // get file name
    const QString newFileName(QFileDialog::getOpenFileName(nullptr, tr("Open Script"), dir, "Aseba scripts (*.aesl)"));
    QFile file(newFileName);
    if(!file.open(QFile::ReadOnly))
        return;

    // open DOM document
    QDomDocument document("aesl-source");
    QString errorMsg;
    int errorLine;
    int errorColumn;
    if(document.setContent(&file, false, &errorMsg, &errorLine, &errorColumn)) {
        // load Thymio node data
        bool dataLoaded(false);
        QDomNode domNode = document.documentElement().firstChild();
        while(!domNode.isNull()) {
            // if the element is a node
            if(domNode.isElement()) {
                QDomElement element = domNode.toElement();
                if(element.tagName() == "node") {
                    QDomElement toolsPlugins(element.firstChildElement("toolsPlugins"));
                    QDomElement toolPlugin(toolsPlugins.firstChildElement());
                    while(!toolPlugin.isNull()) {
                        // if VPL found
                        if(toolPlugin.nodeName() == "ThymioVisualProgramming") {
                            // restore VPL data
                            QDomDocument pluginDataDocument("tool-plugin-data");
                            pluginDataDocument.appendChild(
                                pluginDataDocument.importNode(toolPlugin.firstChildElement(), true));
                            vpl->loadFromDom(pluginDataDocument, true);
                            dataLoaded = true;
                            break;
                        }
                        toolPlugin = toolPlugin.nextSiblingElement();
                    }
                }
            }
            domNode = domNode.nextSibling();
        }
        // check whether we did load data
        if(dataLoaded) {
            fileName = newFileName;
            QSettings().setValue("ThymioVPLClassic/fileName", fileName);
            updateWindowTitle(vpl->isModified());
        } else {
            QMessageBox::warning(this, tr("Loading"),
                                 tr("No Thymio VPL data were found in the script file, file ignored."));
        }
    } else {
        QMessageBox::warning(
            this, tr("Loading"),
            tr("Error in XML source file: %0 at line %1, column %2").arg(errorMsg).arg(errorLine).arg(errorColumn));
    }

    file.close();
}

QString ThymioVPLApplication::openedFileName() const {
    return fileName;
}

void ThymioVPLApplication::onNodeChanged(std::shared_ptr<mobsya::ThymioNode> node) {
    if(!m_thymio) {
        if(node->uuid() == m_thymioId) {
            m_thymio = node;
        }
    }
    disconnectedMessage->setText(tr("Connection to Thymio lost... make sure Thymio is on and "
                                    "connect the USB cable/dongle"));
    disconnectedMessage->setVisible(!m_thymio || m_thymio->status() != mobsya::ThymioNode::Status::Ready);

    if(vpl)
        vpl->onDeviceReadyChanged(m_thymio && m_thymio->status() == mobsya::ThymioNode::Status::Ready);

    if(node != m_thymio)
        return;

    if(node->status() == mobsya::ThymioNode::Status::Ready) {
        setupConnection();
    } else if(node->status() == mobsya::ThymioNode::Status::Available) {
        node->lock();
    } else {
        m_thymio.reset();
    }

    updateWindowTitle(vpl && vpl->isModified());
}

void ThymioVPLApplication::connectToDevice(QUuid id) {
    if(!m_thymioId.isNull())
        return;
    m_thymioId = id;
    auto n = m_client->node(m_thymioId);
    if(n)
        onNodeChanged(n);
}

void ThymioVPLApplication::setupConnection() {

    m_thymio->setWatchEventsEnabled(true);
    m_thymio->addEvent(mobsya::EventDescription{"pair_run", 1});
    m_thymio->addEvent(mobsya::EventDescription{"debug_log", 14});
    connect(m_thymio.get(), &mobsya::ThymioNode::events, this, &ThymioVPLApplication::eventsReceived);

    if(vpl)
        return;

    // create the VPL widget and add it
    vpl = new ThymioVPL::ThymioVisualProgramming(this);
    vplLayout->addWidget(vpl);

    // connect callbacks
    connect(vpl, SIGNAL(modifiedStatusChanged(bool)), SLOT(updateWindowTitle(bool)));
    connect(vpl, SIGNAL(compilationOutcome(bool)), editor, SLOT(setEnabled(bool)));

    // reload data
    if(!savedContent.isNull())
        vpl->loadFromDom(savedContent, false);

    // reset sizes
    resetSizes();
}

//! A node has disconnected from the network.
void ThymioVPLApplication::teardownConnection() {
    if(!vpl)
        return;
    savedContent = vpl->saveToDom();
    vplLayout->removeWidget(vpl);
    // explicitely set no parent to avoid crash on Windows
    vpl->setParent(nullptr);
    vpl->deleteLater();
    vpl = nullptr;
    disconnectedMessage->show();
    updateWindowTitle(vpl && vpl->isModified());
}

//! The execution state logic thinks variables might need a refresh
void ThymioVPLApplication::variablesMemoryEstimatedDirty(unsigned) {}

//! Update the window title with filename and modification status
void ThymioVPLApplication::updateWindowTitle(bool modified) {
    QString modifiedText;
    if(modified)
        modifiedText = tr("[modified] ");

    QString docName(tr("Untitled"));
    if(!fileName.isEmpty())
        docName = fileName.mid(fileName.lastIndexOf("/") + 1);

    QString name;
    if(m_thymio && m_thymio->status() == mobsya::ThymioNode::Status::Ready) {
        name = tr("on %1").arg(m_thymio->name());
    } else {
        name = tr("[not connected]");
    }
    setWindowTitle(tr("%0 %1 %2 - Thymio Visual Programming Language").arg(docName).arg(modifiedText).arg(name));
}

//! Toggle on/off full screen
void ThymioVPLApplication::toggleFullScreen() {
    if(isFullScreen())
        showNormal();
    else
        showFullScreen();
}

/*@}*/
}  // namespace Aseba
