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

#include "MainWindow.h"
#include "ClickableLabel.h"
#include "TargetModels.h"
#include "NamedValuesVectorModel.h"
#include "StudioAeslEditor.h"
#include "FindDialog.h"
#include "ModelAggregator.h"
#include "translations/CompilerTranslator.h"
#include "common/consts.h"
#include "common/productids.h"
#include "common/utils/utils.h"
#include "common/about/AboutDialog.h"
#include "NodeTab.h"
#include "NodeTabsManager.h"

#include <QtGui>
#include <QtWidgets>
#include <QtXml>
#include <QJsonDocument>
#include <sstream>
#include <iostream>
#include <cassert>
#include <QTabWidget>
#include <QDesktopServices>
#include <QSvgRenderer>
#include <utility>
#include <iostream>

using std::copy;


namespace Aseba {
/** \addtogroup studio */
/*@{*/

CompilationLogDialog::CompilationLogDialog(QWidget* parent) : QDialog(parent), te(new QTextEdit()) {
    auto* l(new QVBoxLayout);
    l->addWidget(te);
    setLayout(l);

    QFont font;
    font.setFamily(QLatin1String(""));
    font.setStyleHint(QFont::TypeWriter);
    font.setFixedPitch(true);
    font.setPointSize(10);

    te->setFont(font);
    te->setTabStopWidth(QFontMetrics(font).width(' ') * 4);
    te->setReadOnly(true);

    setWindowTitle(tr("Aseba Studio: Output of last compilation"));

    resize(600, 560);
}

void CompilationLogDialog::hideEvent(QHideEvent*) {
    if(!isVisible())
        emit hidden();
}

MainWindow::MainWindow(const mobsya::ThymioDeviceManagerClient& client, const QVector<QUuid>& targetUuids,
                       QWidget* parent)
    : QMainWindow(parent) {

    nodes = new NodeTabsManager(client);
    connect(nodes, &NodeTabsManager::tabAdded, this, &MainWindow::tabAdded);
    connect(nodes, &NodeTabsManager::nodeStatusChanged, this, &MainWindow::regenerateToolsMenus);

    // create config dialog + read settings on-disk
    ConfigDialog::init(this);

    // create gui
    setupWidgets();
    setupMenu();
    setupConnections();
    setWindowIcon(QIcon(":/images/icons/asebastudio.svgz"));

    // cosmetic fix-up
    updateWindowTitle();
    if(readSettings() == false)
        resize(1000, 700);

    for(auto&& id : targetUuids) {
        nodes->addTab(id);
    }
}

MainWindow::~MainWindow() {}


void MainWindow::tabAdded(int index) {
    NodeTab* tab = qobject_cast<NodeTab*>(nodes->widget(index));
    if(!tab)
        return;
    connect(showHiddenAct, &QAction::toggled, tab, &NodeTab::showHidden);
    tab->showHidden(showHiddenAct->isChecked());
}

void MainWindow::connectToDevice(QUuid id) {
    nodes->addTab(id);
}


void MainWindow::about() {
    const AboutBox::Parameters aboutParameters = {
        "Aseba Studio",
        ":/images/icons/asebastudio.svgz",
        tr("Aseba Studio is an environment for interactively programming robots with a text "
           "language."),
        tr("https://www.thymio.org/en:asebastudio"),
        "",
        {"core", "studio", "vpl", "packaging", "translation"}};
    AboutBox aboutBox(this, aboutParameters);
    aboutBox.exec();
}

bool MainWindow::newFile() {
    if(askUserBeforeDiscarding()) {
        // we must only have NodeTab* left, clear content of editors in tabs
        for(auto&& tab : nodes->devicesTabs()) {
            tab->clearEverything();
        }
        // reset opened file name
        clearOpenedFileName(false);
        return true;
    }
    return false;
}


/* ************************************************
*
* The function opens .aesl files for the studio project
* input: the path strin is optional: 
*     if not given a dialog for selecting and retrieving a file would be opened 
*     if given, the filepath must be comprehensive of its own full absoulute path 
* the function will manage file non presents nor accesible in the file system. 
If everything goes fine the aesl file would be loaded onto the studio interface
* \return code:
*    -1 : failed due to file unavailability
*    -2 : aborted by user due to discard changes
*    -3 : ??? case
************************************************ */
int MainWindow::openFile(const QString& path) {

    // make sure we do not loose changes
    if(askUserBeforeDiscarding() == false){
        return -2;
    } 

    // open the file
    QString fileName = path;

    // if no file to open is passed, show a dialog
    if(fileName.isEmpty()) {
        // try to guess the directory of the last opened or saved file
        QString dir;
        if(!actualFileName.isEmpty()) {
            // a document is opened, propose to open it again
            dir = actualFileName;
        } else {
            // no document is opened, try recent files
            QSettings settings;
            QStringList recentFiles = settings.value(QStringLiteral("recent files")).toStringList();
            if(recentFiles.size() > 0) {
                dir = recentFiles[0];
            } else {
                const QStringList stdLocations(QStandardPaths::standardLocations(QStandardPaths::DocumentsLocation));
                dir = !stdLocations.empty() ? stdLocations[0] : QLatin1String("");
            }
        }

        fileName = QFileDialog::getOpenFileName(this, tr("Open Script"), dir, QStringLiteral("Aseba scripts (*.aesl)"));
        qDebug() << fileName;

    }


    QFile file(fileName);
    if(!file.open(QFile::ReadOnly)){
        return -1;
    }

    auto groups = nodes->groups();
    if(groups.size() != 1){
        return -3;
    }

    // --- from here on we assume everything went fine --- 
    // [2do] this on should be in the calling methods or in a separate function
    for(auto&& tab : nodes->devicesTabs()) {
        tab->clearEverything();
    }

    actualFileName = fileName;
    updateRecentFiles(actualFileName);
    regenerateOpenRecentMenu();
    updateWindowTitle();

    groups.front()->loadAesl(file.readAll());

    return 0;
}

/* ************************************************
*
* The function called asycronously by the recent file menu 
* it will eventually open a recent file if available 
* if the file is not available would ask to the user to remove it from the list
*
************************************************ */

void MainWindow::openRecentFile() {
    auto* entry = dynamic_cast<QAction*>(sender());

    if(openFile(entry->text()) == -1){

        QMessageBox msgBox;
        msgBox.setWindowTitle(tr("Aseba Studio - File Exception"));
        msgBox.setText(tr("The file \"%0\" is not present anymore in the location.").arg(entry->text()));
        msgBox.setInformativeText(tr("Do you want to delete it from the list?"));
        msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Ignore);
        msgBox.setDefaultButton(QMessageBox::Ok);

        int ret = msgBox.exec();
        switch(ret) {
            case QMessageBox::Ok:
                updateRecentFiles(entry->text(), true);
                regenerateOpenRecentMenu();
                break ;
            case QMessageBox::Ignore:
                // Cancel was clicked
                break ;
            default:
                // should never be reached
                assert(false);
                break;
        }
    }
    return;
}

bool MainWindow::save() {
    return saveFile(actualFileName);
}

bool MainWindow::saveFile(const QString& previousFileName) {

    if(ranges::empty(nodes->devicesTabs()))
        return false;

    QString fileName = previousFileName;

    if(fileName.isEmpty())
        fileName = QFileDialog::getSaveFileName(
            this, tr("Save Script"),
            actualFileName.isEmpty() ? QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) :
                                       actualFileName,
            "Aseba scripts (*.aesl)");

    if(fileName.isEmpty())
        return false;

    if(fileName.lastIndexOf(".") < 0)
        fileName += ".aesl";

    QFile file(fileName);
    if(!file.open(QFile::WriteOnly | QFile::Truncate))
        return false;

    actualFileName = fileName;
    updateRecentFiles(fileName);
    regenerateOpenRecentMenu();

    // --> from here on creation of the document  <--- 
    // initiate DOM tree
    QDomDocument document("aesl-source");
    QDomElement root = document.createElement("network");
    document.appendChild(root);

    root.appendChild(document.createTextNode("\n\n\n"));
    root.appendChild(document.createComment("list of global events"));


    std::map<QString, mobsya::EventDescription> events;
    std::map<QString, mobsya::ThymioVariable> sharedVariables;
    for(auto&& tab : nodes->devicesTabs()) {
        auto nodeEvents = tab->eventsDescriptions();
        for(auto&& event : nodeEvents) {
            events.emplace(event.name(), event);
        }
        auto nodeSharedVars = tab->groupVariables().toStdMap();
        sharedVariables.insert(nodeSharedVars.begin(), nodeSharedVars.end());
    }

    for(auto&& e : events) {
        QDomElement element = document.createElement("event");
        element.setAttribute("name", e.second.name());
        element.setAttribute("size", e.second.size());
        root.appendChild(element);
    }

    for(auto&& v : sharedVariables) {
        QDomElement element = document.createElement("constant");
        element.setAttribute("name", v.first);
        element.setAttribute("value", v.second.value().toInt());
        root.appendChild(element);
    }
    // source code
    for(auto&& tab : nodes->devicesTabs()) {
        QString nodeName;
        const auto thymio = tab->thymio();
        if(thymio) {
            nodeName = thymio->name();
        }

        root.appendChild(document.createTextNode("\n\n\n"));
        root.appendChild(document.createComment(QString("node %0").arg(nodeName)));

        QDomElement element = document.createElement("node");
        element.setAttribute("name", nodeName);
        if(thymio)
            element.setAttribute("nodeId", thymio->uuid().toString());
        QDomText text = document.createCDATASection(tab->editor->toPlainText());
        element.appendChild(text);
        root.appendChild(element);
    }
    root.appendChild(document.createTextNode("\n\n\n"));

    QTextStream out(&file);
    document.save(out, 0);
    // --> END creation of the document  <--- 

    updateWindowTitle();

    return true;
}

void MainWindow::exportMemoriesContent() {
    QString exportFileName = QFileDialog::getSaveFileName(
        this, tr("Export memory content"), QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation),
        "All Files (*);;CSV files (*.csv);;Text files (*.txt)");

    QFile file(exportFileName);
    if(!file.open(QFile::WriteOnly | QFile::Truncate))
        return;

    QTextStream out(&file);

    for(auto&& tab : nodes->devicesTabs()) {
        auto thymio = tab->thymio();
        if(!thymio)
            continue;

        const QString nodeName = thymio->name();
        const QVariantMap variables = tab->getVariables();
        for(auto it = variables.begin(); it != variables.end(); ++it) {
            out << nodeName << "." << it.key() << " "
                << QJsonDocument::fromVariant(it.value()).toJson(QJsonDocument::JsonFormat::Compact);
            out << "\n";
        }
    }
}

void MainWindow::copyAll() {
    QString toCopy;
    for(auto&& tab : nodes->devicesTabs()) {
        auto thymio = tab->thymio();
        if(!thymio)
            continue;
        toCopy += QString("# node %0\n").arg(thymio->name());
        toCopy += tab->editor->toPlainText();
        toCopy += "\n\n";
    }
    QApplication::clipboard()->setText(toCopy);
}

void MainWindow::findTriggered() {
    auto* tab = dynamic_cast<ScriptTab*>(nodes->currentWidget());
    if(tab && tab->editor->textCursor().hasSelection())
        findDialog->setFindText(tab->editor->textCursor().selectedText());
    findDialog->replaceGroupBox->setChecked(false);
    findDialog->show();
}

void MainWindow::replaceTriggered() {
    findDialog->replaceGroupBox->setChecked(true);
    findDialog->show();
}

void MainWindow::commentTriggered() {
    if(!currentScriptTab)
        return;
    currentScriptTab->editor->commentAndUncommentSelection(AeslEditor::CommentSelection);
}

void MainWindow::uncommentTriggered() {
    if(!currentScriptTab)
        return;
    currentScriptTab->editor->commentAndUncommentSelection(AeslEditor::UncommentSelection);
}

void MainWindow::showLineNumbersChanged(bool state) {
    for(auto&& tab : nodes->devicesTabs()) {
        tab->linenumbers->showLineNumbers(state);
    }
    ConfigDialog::setShowLineNumbers(state);
}

void MainWindow::goToLine() {
    if(!currentScriptTab)
        return;
    QTextEdit* editor(currentScriptTab->editor);
    const QTextDocument* document(editor->document());
    QTextCursor cursor(editor->textCursor());
    bool ok;
    const int curLine = cursor.blockNumber() + 1;
    const int minLine = 1;
    const int maxLine = document->lineCount();
    const int line = QInputDialog::getInt(this, tr("Go To Line"), tr("Line:"), curLine, minLine, maxLine, 1, &ok);
    if(ok)
        editor->setTextCursor(QTextCursor(document->findBlockByLineNumber(line - 1)));
}

void MainWindow::zoomIn() {
    if(!currentScriptTab)
        return;
    QTextEdit* editor(currentScriptTab->editor);
    editor->zoomIn();
}

void MainWindow::zoomOut() {
    if(!currentScriptTab)
        return;
    QTextEdit* editor(currentScriptTab->editor);
    editor->zoomOut();
}

void MainWindow::showSettings() {
    ConfigDialog::showConfig();
}

void MainWindow::toggleBreakpoint() {
    if(!currentScriptTab)
        return;
    currentScriptTab->editor->toggleBreakpoint();
}

void MainWindow::clearAllBreakpoints() {
    if(!currentScriptTab)
        return;
    currentScriptTab->editor->clearAllBreakpoints();
}

void MainWindow::resetAll() {
    for(auto&& tab : nodes->devicesTabs()) {
        tab->reset();
    }
}

void MainWindow::runAll() {
    for(auto&& tab : nodes->devicesTabs()) {
        tab->run();
    }
}

void MainWindow::pauseAll() {
    for(auto&& tab : nodes->devicesTabs()) {
        tab->run();
    }
}

void MainWindow::stopAll() {
    for(auto&& tab : nodes->devicesTabs()) {
        tab->reset();
    }
}

void MainWindow::clearAllExecutionError() {
    for(auto&& tab : nodes->devicesTabs()) {
        tab->clearExecutionErrors();
    }
    logger->setStyleSheet(QLatin1String(""));
}

void MainWindow::eventContextMenuRequested(const QPoint&) {}

void MainWindow::tabChanged(int index) {
    findDialog->close();
    auto* tab = dynamic_cast<ScriptTab*>(nodes->widget(index));
    if(currentScriptTab && tab != currentScriptTab) {
        disconnect(cutAct, &QAction::triggered, currentScriptTab->editor, &QTextEdit::cut);
        disconnect(copyAct, &QAction::triggered, currentScriptTab->editor, &QTextEdit::copy);
        disconnect(pasteAct, &QAction::triggered, currentScriptTab->editor, &QTextEdit::paste);
        disconnect(undoAct, &QAction::triggered, currentScriptTab->editor, &QTextEdit::undo);
        disconnect(redoAct, &QAction::triggered, currentScriptTab->editor, &QTextEdit::redo);
        disconnect(currentScriptTab->editor, &QTextEdit::copyAvailable, cutAct, &QAction::setEnabled);
        disconnect(currentScriptTab->editor, &QTextEdit::copyAvailable, copyAct, &QAction::setEnabled);
        disconnect(currentScriptTab->editor, &QTextEdit::undoAvailable, undoAct, &QAction::setEnabled);
        disconnect(currentScriptTab->editor, &QTextEdit::redoAvailable, redoAct, &QAction::setEnabled);
    }
    currentScriptTab = tab;
    if(tab) {
        connect(cutAct, &QAction::triggered, currentScriptTab->editor, &QTextEdit::cut);
        connect(copyAct, &QAction::triggered, currentScriptTab->editor, &QTextEdit::copy);
        connect(pasteAct, &QAction::triggered, currentScriptTab->editor, &QTextEdit::paste);
        connect(undoAct, &QAction::triggered, currentScriptTab->editor, &QTextEdit::undo);
        connect(redoAct, &QAction::triggered, currentScriptTab->editor, &QTextEdit::redo);
        connect(currentScriptTab->editor, &QTextEdit::copyAvailable, cutAct, &QAction::setEnabled);
        connect(currentScriptTab->editor, &QTextEdit::copyAvailable, copyAct, &QAction::setEnabled);
        connect(currentScriptTab->editor, &QTextEdit::undoAvailable, undoAct, &QAction::setEnabled);
        connect(currentScriptTab->editor, &QTextEdit::redoAvailable, redoAct, &QAction::setEnabled);

        findDialog->editor = tab->editor;
    } else {
        findDialog->editor = nullptr;
    }

    cutAct->setEnabled(currentScriptTab);
    copyAct->setEnabled(currentScriptTab);
    pasteAct->setEnabled(currentScriptTab);
    findAct->setEnabled(currentScriptTab);
    undoAct->setEnabled(currentScriptTab);
    redoAct->setEnabled(currentScriptTab);
    goToLineAct->setEnabled(currentScriptTab);
    zoomInAct->setEnabled(currentScriptTab);
    zoomOutAct->setEnabled(currentScriptTab);
    findDialog->replaceGroupBox->setEnabled(currentScriptTab);
}

void MainWindow::showCompilationMessages(bool doShow) {
    // this slot shouldn't be callable when an unactive tab is show
    compilationMessageBox->setVisible(doShow);
    if(nodes->currentWidget())
        dynamic_cast<NodeTab*>(nodes->currentWidget())->compileCodeOnTarget();
}

/* ********** 
  * The function triggers a request to the TDM to compile the code in the editor 
 * after the compilation the compiledcode is sent back to the client who asked 
 * and saved locally to the file location requested from the user 
 * 
 * .abo extension is assigned to the file - the format is described here http://wiki.thymio.org/asebaspecifications001
 * **********/

void MainWindow::ExportCode() {

    QString exportFileName = QFileDialog::getSaveFileName(
    
    this, tr("Export current program to binary"), 

        dynamic_cast<NodeTab*>(nodes->currentWidget())->lastFileLocation.isEmpty() ? 
        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) : 
        dynamic_cast<NodeTab*>(nodes->currentWidget())->lastFileLocation,
        
        "Thymio Bytecode (*.abo);;All Files (*)");

    QFile file(exportFileName);
    if(!file.open(QFile::WriteOnly | QFile::Truncate))
        return;

    if(nodes->currentWidget()){
        dynamic_cast<NodeTab*>(nodes->currentWidget())->lastFileLocation = exportFileName;
        dynamic_cast<NodeTab*>(nodes->currentWidget())->saveCodeOnTarget();
    }

}

void MainWindow::compilationMessagesWasHidden() {
    showCompilationMsg->setChecked(false);
}

void MainWindow::showMemoryUsage(bool show) {
    for(auto&& tab : nodes->devicesTabs()) {
        tab->showMemoryUsage(show);
    }
    ConfigDialog::setShowMemoryUsage(show);
}


void MainWindow::recompileAll() {
    for(auto&& tab : nodes->devicesTabs()) {
        tab->compileCodeOnTarget();
    }
}

void MainWindow::writeAllBytecodes() {
    for(auto&& tab : nodes->devicesTabs()) {
        tab->writeProgramToDeviceMemory();
    }
}

void MainWindow::rebootAllNodes() {
    for(auto&& tab : nodes->devicesTabs()) {
        tab->reboot();
    }
}

void MainWindow::sourceChanged() {
    updateWindowTitle();
}

void MainWindow::setupWidgets() {
    currentScriptTab = nullptr;
    nodes->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));

    auto* splitter = new QSplitter();
    splitter->addWidget(nodes);
    setCentralWidget(splitter);

    // dialog box
    compilationMessageBox = new CompilationLogDialog(this);
    connect(this, &MainWindow::MainWindowClosed, compilationMessageBox, &QWidget::close);
    findDialog = new FindDialog(this);
    findDialog->setWindowFlag(Qt::Popup);
    connect(this, &MainWindow::MainWindowClosed, findDialog, &QWidget::close);
}

void MainWindow::setupConnections() {
    // general connections
    connect(nodes, &QTabWidget::currentChanged, this, &MainWindow::tabChanged);
    // connect(logger, &QListWidget::itemDoubleClicked, this, &MainWindow::logEntryDoubleClicked);
    connect(ConfigDialog::getInstance(), &ConfigDialog::settingsChanged, this, &MainWindow::applySettings);

    // global actions
    connect(stopAllAct, &QAction::triggered, this, &MainWindow::stopAll);
    connect(runAllAct, &QAction::triggered, this, &MainWindow::runAll);
    connect(pauseAllAct, &QAction::triggered, this, &MainWindow::pauseAll);
}

/* ************************
* The function generate the interface OpenRecent menu starting from the system stored value  
* \warning: this function must be called by the dev all times the system updates the 
recentFiles data structure otherways interface and system would be in detached state   
* [2do] this can improved in several ways to avoid detached interface 
************************ */
void MainWindow::regenerateOpenRecentMenu() {
    openRecentMenu->clear();

    // Add all other actions excepted the one we are processing
    QSettings settings;
    QStringList recentFiles = settings.value(QStringLiteral("recent files")).toStringList();
    for(int i = 0; i < recentFiles.size(); i++) {
        const QString& fileName(recentFiles.at(i));
        openRecentMenu->addAction(fileName, this, SLOT(openRecentFile()));
    }
}

/* ************************
* The function updates system data structure related to recent files implementing the following: 
* - add of a new entry
* - del of a single entry already present in the list
* - avoid duplicated entries
************************ */
void MainWindow::updateRecentFiles(const QString& fileName, bool to_delete ) {
    QSettings settings;
    QStringList recentFiles = settings.value(QStringLiteral("recent files")).toStringList();
    const int maxRecentFiles = 8;
    
    if(recentFiles.contains(fileName) ){
        recentFiles.removeAt(recentFiles.indexOf(fileName));
    } 
    
    if(!to_delete){
        recentFiles.push_front(fileName);   
    }

    if(recentFiles.size() > maxRecentFiles)
        recentFiles.pop_back();

    settings.setValue(QStringLiteral("recent files"), recentFiles);
}

void MainWindow::regenerateToolsMenus() {
    writeBytecodeMenu->clear();
    rebootMenu->clear();
    unsigned activeVMCount(0);
    for(auto&& tab : nodes->devicesTabs()) {
        if(tab->thymio() && tab->thymio()->status() == mobsya::ThymioNode::Status::Ready) {
            QAction* act = writeBytecodeMenu->addAction(tr("...inside %0").arg(tab->thymio()->name()), tab,
                                                        &NodeTab::writeProgramToDeviceMemory);
            connect(tab, SIGNAL(uploadReadynessChanged(bool)), act, SLOT(setEnabled(bool)));
            rebootMenu->addAction(tr("...%0").arg(tab->thymio()->name()), tab, SLOT(reboot()));
        }
        ++activeVMCount;
    }
    writeBytecodeMenu->addSeparator();
    writeAllBytecodesAct = writeBytecodeMenu->addAction(tr("...inside all nodes"), this, SLOT(writeAllBytecodes()));
    rebootMenu->addSeparator();
    rebootMenu->addAction(tr("...all nodes"), this, SLOT(rebootAllNodes()));
    globalToolBar->setVisible(activeVMCount > 1);
}

void MainWindow::generateHelpMenu() {
    helpMenuTargetSpecificSeparator = helpMenu->addSeparator();
    helpMenu->addAction(tr("Web site Aseba..."), this, SLOT(openToUrlFromAction()))
        ->setData(QUrl(tr("http://aseba.wikidot.com/en:start")));
    helpMenu->addAction(tr("Report bug..."), this, SLOT(openToUrlFromAction()))
        ->setData(QUrl(tr("http://github.com/mobsya/aseba/issues/new")));

#ifdef Q_WS_MAC
    helpMenu->addAction("about", this, SLOT(about()));
    helpMenu->addAction("About &Qt...", qApp, SLOT(aboutQt()));
#else   // Q_WS_MAC
    helpMenu->addSeparator();
    helpMenu->addAction(tr("&About..."), this, SLOT(about()));
    helpMenu->addAction(tr("About &Qt..."), qApp, SLOT(aboutQt()));
#endif  // Q_WS_MAC
}

void MainWindow::regenerateHelpMenu() {
    // remove old target-specific actions
    while(!targetSpecificHelp.isEmpty()) {
        QAction* action(targetSpecificHelp.takeFirst());
        helpMenu->removeAction(action);
        delete action;
    }

    // add back target-specific actions
    using ProductIds = std::set<int>;
    ProductIds productIds;
    for(auto&& tab : nodes->devicesTabs()) {
        productIds.insert(tab->productId());
    }
    for(auto it(productIds.begin()); it != productIds.end(); ++it) {
        QAction* action;
        switch(*it) {
            case ASEBA_PID_THYMIO2:
                action = new QAction(tr("Thymio programming tutorial..."), helpMenu);
                connect(action, &QAction::triggered, this, &MainWindow::openToUrlFromAction);
                action->setData(QUrl(tr("http://aseba.wikidot.com/en:thymiotutoriel")));
                targetSpecificHelp.append(action);
                helpMenu->insertAction(helpMenuTargetSpecificSeparator, action);
                action = new QAction(tr("Thymio programming interface..."), helpMenu);
                connect(action, &QAction::triggered, this, &MainWindow::openToUrlFromAction);
                action->setData(QUrl(tr("http://aseba.wikidot.com/en:thymioapi")));
                targetSpecificHelp.append(action);
                helpMenu->insertAction(helpMenuTargetSpecificSeparator, action);
                break;

            case ASEBA_PID_CHALLENGE:
                action = new QAction(tr("Challenge tutorial..."), helpMenu);
                connect(action, &QAction::triggered, this, &MainWindow::openToUrlFromAction);
                action->setData(QUrl(tr("http://aseba.wikidot.com/en:gettingstarted")));
                targetSpecificHelp.append(action);
                helpMenu->insertAction(helpMenuTargetSpecificSeparator, action);
                break;

            case ASEBA_PID_MARXBOT:
                action = new QAction(tr("MarXbot user manual..."), helpMenu);
                connect(action, &QAction::triggered, this, &MainWindow::openToUrlFromAction);
                action->setData(QUrl(tr("http://mobots.epfl.ch/data/robots/marxbot-user-manual.pdf")));
                targetSpecificHelp.append(action);
                helpMenu->insertAction(helpMenuTargetSpecificSeparator, action);
                break;

            default: break;
        }
    }
}

void MainWindow::openToUrlFromAction() const {
    const QAction* action(reinterpret_cast<QAction*>(sender()));
    QDesktopServices::openUrl(action->data().toUrl());
}

void MainWindow::setupMenu() {
    // File menu
    QMenu* fileMenu = new QMenu(tr("&File"), this);
    menuBar()->addMenu(fileMenu);

    fileMenu->addAction(QIcon(":/images/filenew.png"), tr("&New"), this, SLOT(newFile()), QKeySequence::New);
    fileMenu->addAction(QIcon(":/images/fileopen.png"), tr("&Open..."), this, SLOT(openFile()), QKeySequence::Open);
    openRecentMenu = new QMenu(tr("Open &Recent"), fileMenu);
    regenerateOpenRecentMenu();
    fileMenu->addMenu(openRecentMenu)->setIcon(QIcon(":/images/fileopen.png"));

    fileMenu->addAction(QIcon(":/images/filesave.png"), tr("&Save"), this, SLOT(save()), QKeySequence::Save);
    fileMenu->addAction(QIcon(":/images/filesaveas.png"), tr("Save &As..."), this, SLOT(saveFile()),
                        QKeySequence::SaveAs);

    fileMenu->addSeparator();
    fileMenu->addAction(QIcon(":/images/filesaveas.png"), tr("Export &memories content..."), this,
                        SLOT(exportMemoriesContent()));
    fileMenu->addAction(QIcon(":/images/filesaveas.png"), tr("Export current program to binary..."), this, SLOT(ExportCode())),
    fileMenu->addSeparator();
#ifdef Q_WS_MAC
    fileMenu->addAction(QIcon(":/images/exit.png"), "quit", this, SLOT(close()), QKeySequence::Quit);
#else   // Q_WS_MAC
    fileMenu->addAction(QIcon(":/images/exit.png"), tr("&Quit"), this, SLOT(close()), QKeySequence::Quit);
#endif  // Q_WS_MAC

    // Edit menu
    cutAct = new QAction(QIcon(":/images/editcut.png"), tr("Cu&t"), this);
    cutAct->setShortcut(QKeySequence::Cut);
    cutAct->setEnabled(false);

    copyAct = new QAction(QIcon(":/images/editcopy.png"), tr("&Copy"), this);
    copyAct->setShortcut(QKeySequence::Copy);
    copyAct->setEnabled(false);

    pasteAct = new QAction(QIcon(":/images/editpaste.png"), tr("&Paste"), this);
    pasteAct->setShortcut(QKeySequence::Paste);
    pasteAct->setEnabled(false);

    undoAct = new QAction(QIcon(":/images/undo.png"), tr("&Undo"), this);
    undoAct->setShortcut(QKeySequence::Undo);
    undoAct->setEnabled(false);

    redoAct = new QAction(QIcon(":/images/redo.png"), tr("Re&do"), this);
    redoAct->setShortcut(QKeySequence::Redo);
    redoAct->setEnabled(false);

    findAct = new QAction(QIcon(":/images/find.png"), tr("&Find..."), this);
    findAct->setShortcut(QKeySequence::Find);
    connect(findAct, &QAction::triggered, this, &MainWindow::findTriggered);
    findAct->setEnabled(false);

    replaceAct = new QAction(QIcon(":/images/edit.png"), tr("&Replace..."), this);
    replaceAct->setShortcut(QKeySequence::Replace);
    connect(replaceAct, &QAction::triggered, this, &MainWindow::replaceTriggered);
    replaceAct->setEnabled(false);

    goToLineAct = new QAction(QIcon(":/images/goto.png"), tr("&Go To Line..."), this);
    goToLineAct->setShortcut(tr("Ctrl+G", "Edit|Go To Line"));
    goToLineAct->setEnabled(false);
    connect(goToLineAct, &QAction::triggered, this, &MainWindow::goToLine);

    commentAct = new QAction(tr("Comment the selection"), this);
    commentAct->setShortcut(tr("Ctrl+D", "Edit|Comment the selection"));
    connect(commentAct, &QAction::triggered, this, &MainWindow::commentTriggered);

    uncommentAct = new QAction(tr("Uncomment the selection"), this);
    uncommentAct->setShortcut(tr("Shift+Ctrl+D", "Edit|Uncomment the selection"));
    connect(uncommentAct, &QAction::triggered, this, &MainWindow::uncommentTriggered);

    QMenu* editMenu = new QMenu(tr("&Edit"), this);
    menuBar()->addMenu(editMenu);
    editMenu->addAction(cutAct);
    editMenu->addAction(copyAct);
    editMenu->addAction(pasteAct);
    editMenu->addSeparator();
    editMenu->addAction(QIcon(":/images/editcopy.png"), tr("Copy &all"), this, SLOT(copyAll()));
    editMenu->addSeparator();
    editMenu->addAction(undoAct);
    editMenu->addAction(redoAct);
    editMenu->addSeparator();
    editMenu->addAction(findAct);
    editMenu->addAction(replaceAct);
    editMenu->addSeparator();
    editMenu->addAction(goToLineAct);
    editMenu->addSeparator();
    editMenu->addAction(commentAct);
    editMenu->addAction(uncommentAct);

    // View menu
    showMemoryUsageAct = new QAction(tr("Show &memory usage"), this);
    showMemoryUsageAct->setCheckable(true);
    connect(showMemoryUsageAct, &QAction::toggled, this, &MainWindow::showMemoryUsage);

    showHiddenAct = new QAction(tr("S&how hidden variables and functions"), this);
    showHiddenAct->setCheckable(true);

    showLineNumbers = new QAction(tr("Show &Line Numbers"), this);
    showLineNumbers->setShortcut(tr("F11", "View|Show Line Numbers"));
    showLineNumbers->setCheckable(true);
    connect(showLineNumbers, &QAction::toggled, this, &MainWindow::showLineNumbersChanged);

    zoomInAct = new QAction(tr("&Increase font size"), this);
    zoomInAct->setShortcut(QKeySequence::ZoomIn);
    zoomInAct->setEnabled(false);
    connect(zoomInAct, &QAction::triggered, this, &MainWindow::zoomIn);

    zoomOutAct = new QAction(tr("&Decrease font size"), this);
    zoomOutAct->setShortcut(QKeySequence::ZoomOut);
    zoomOutAct->setEnabled(false);
    connect(zoomOutAct, &QAction::triggered, this, &MainWindow::zoomOut);

    QMenu* viewMenu = new QMenu(tr("&View"), this);
    viewMenu->addAction(showMemoryUsageAct);
    viewMenu->addAction(showHiddenAct);
    viewMenu->addAction(showLineNumbers);
    viewMenu->addSeparator();
    viewMenu->addAction(zoomInAct);
    viewMenu->addAction(zoomOutAct);
    viewMenu->addSeparator();
#ifdef Q_WS_MAC
    viewMenu->addAction("settings", this, SLOT(showSettings()), QKeySequence::Preferences);
#else   // Q_WS_MAC
    viewMenu->addAction(tr("&Settings"), this, SLOT(showSettings()), QKeySequence::Preferences);
#endif  // Q_WS_MAC
    menuBar()->addMenu(viewMenu);

    stopAllAct = new QAction(QIcon(":/images/stop.png"), tr("&Stop all"), this);
    stopAllAct->setShortcut(tr("F8", "Debug|Stop all"));

    runAllAct = new QAction(QIcon(":/images/play.png"), tr("Ru&n all"), this);
    runAllAct->setShortcut(tr("F9", "Debug|Run all"));

    pauseAllAct = new QAction(QIcon(":/images/pause.png"), tr("&Pause all"), this);
    pauseAllAct->setShortcut(tr("F10", "Debug|Pause all"));

    // Debug toolbar
    globalToolBar = addToolBar(tr("Debug"));
    globalToolBar->setObjectName(QStringLiteral("debug toolbar"));
    globalToolBar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    globalToolBar->addAction(stopAllAct);
    globalToolBar->addAction(runAllAct);
    globalToolBar->addAction(pauseAllAct);

    // Debug menu
    toggleBreakpointAct = new QAction(tr("Toggle breakpoint"), this);
    toggleBreakpointAct->setShortcut(tr("Ctrl+B", "Debug|Toggle breakpoint"));
    connect(toggleBreakpointAct, &QAction::triggered, this, &MainWindow::toggleBreakpoint);

    clearAllBreakpointsAct = new QAction(tr("Clear all breakpoints"), this);
    // clearAllBreakpointsAct->setShortcut();
    connect(clearAllBreakpointsAct, &QAction::triggered, this, &MainWindow::clearAllBreakpoints);

    QMenu* debugMenu = new QMenu(tr("&Debug"), this);
    menuBar()->addMenu(debugMenu);
    debugMenu->addAction(toggleBreakpointAct);
    debugMenu->addAction(clearAllBreakpointsAct);
    debugMenu->addSeparator();
    debugMenu->addAction(stopAllAct);
    debugMenu->addAction(runAllAct);
    debugMenu->addAction(pauseAllAct);

    // Tool menu
    QMenu* toolMenu = new QMenu(tr("&Tools"), this);
    menuBar()->addMenu(toolMenu);
    /*toolMenu->addAction(QIcon(":/images/view_text.png"), tr("&Show last compilation messages"),
                        this, SLOT(showCompilationMessages()),
                        QKeySequence(tr("Ctrl+M", "Tools|Show last compilation messages")));*/
    showCompilationMsg = new QAction(QIcon(":/images/view_text.png"), tr("&Show last compilation messages"), this);
    showCompilationMsg->setCheckable(true);
    toolMenu->addAction(showCompilationMsg);
    connect(showCompilationMsg, &QAction::toggled, this, &MainWindow::showCompilationMessages);
    connect(compilationMessageBox, &CompilationLogDialog::hidden, this, &MainWindow::compilationMessagesWasHidden);
    toolMenu->addSeparator();
    writeBytecodeMenu = new QMenu(tr("Write the program(s)..."), toolMenu);
    toolMenu->addMenu(writeBytecodeMenu);
    rebootMenu = new QMenu(tr("Reboot..."), toolMenu);
    toolMenu->addMenu(rebootMenu);

    // Help menu
    helpMenu = new QMenu(tr("&Help"), this);
    menuBar()->addMenu(helpMenu);
    generateHelpMenu();
    regenerateHelpMenu();

    // add dynamic stuff
    regenerateToolsMenus();

    // Load the state from the settings (thus from hard drive)
    applySettings();
}

//! Ask the user to save or discard or ignore the operation that would destroy the unmodified data.
/*!
    \return true if it is ok to discard, false if operation must abort
*/
bool MainWindow::askUserBeforeDiscarding() {
    const bool anythingModified = false;
    //    sourceModified || constantsDefinitionsModel->checkIfModified() || eventsDescriptionsModel->checkIfModified();
    if(anythingModified == false)
        return true;

    QString docName(tr("Untitled"));
    if(!actualFileName.isEmpty())
        docName = actualFileName.mid(actualFileName.lastIndexOf(QLatin1String("/")) + 1);

    QMessageBox msgBox;
    msgBox.setWindowTitle(tr("Aseba Studio - Confirmation Dialog"));
    msgBox.setText(tr("The document \"%0\" has been modified.").arg(docName));
    msgBox.setInformativeText(tr("Do you want to save your changes or discard them?"));
    msgBox.setStandardButtons(QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
    msgBox.setDefaultButton(QMessageBox::Save);

    int ret = msgBox.exec();
    switch(ret) {
        case QMessageBox::Save:
            // Save was clicked
            if(save())
                return true;
            else
                return false;
        case QMessageBox::Discard:
            // Don't Save was clicked
            return true;
        case QMessageBox::Cancel:
            // Cancel was clicked
            return false;
        default:
            // should never be reached
            assert(false);
            break;
    }

    return false;
}

void MainWindow::closeEvent(QCloseEvent* event) {
    if(askUserBeforeDiscarding()) {
        writeSettings();
        event->accept();
        emit MainWindowClosed();
    } else {
        event->ignore();
    }
}

bool MainWindow::readSettings() {
    bool result;

    QSettings settings;
    result = restoreGeometry(settings.value(QStringLiteral("MainWindow/geometry")).toByteArray());
    restoreState(settings.value(QStringLiteral("MainWindow/windowState")).toByteArray());
    return result;
}

void MainWindow::writeSettings() {
    QSettings settings;
    settings.setValue(QStringLiteral("MainWindow/geometry"), saveGeometry());
    settings.setValue(QStringLiteral("MainWindow/windowState"), saveState());
}

void MainWindow::updateWindowTitle() {
    const bool anythingModified = false;
    //   sourceModified || constantsDefinitionsModel->checkIfModified() ||
    //   eventsDescriptionsModel->checkIfModified();

    QString modifiedText;
    if(anythingModified)
        modifiedText = tr("[modified] ");

    QString docName(tr("Untitled"));
    if(!actualFileName.isEmpty())
        docName = actualFileName.mid(actualFileName.lastIndexOf(QLatin1String("/")) + 1);

    setWindowTitle(tr("%0 %1- Aseba Studio").arg(docName).arg(modifiedText));
}

void MainWindow::applySettings() {
    showMemoryUsageAct->setChecked(ConfigDialog::getShowMemoryUsage());
    showHiddenAct->setChecked(ConfigDialog::getShowHidden());
    showLineNumbers->setChecked(ConfigDialog::getShowLineNumbers());
}

void MainWindow::clearOpenedFileName(bool) {
    actualFileName.clear();
    updateWindowTitle();
}

}  // namespace Aseba
