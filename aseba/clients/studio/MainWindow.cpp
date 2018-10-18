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
#include "EventViewer.h"
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
#include <sstream>
#include <iostream>
#include <cassert>
#include <QTabWidget>
#include <QDesktopServices>
#include <QtConcurrentRun>
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
    font.setFamily("");
    font.setStyleHint(QFont::TypeWriter);
    font.setFixedPitch(true);
    font.setPointSize(10);

    te->setFont(font);
    te->setTabStopWidth(QFontMetrics(font).width(' ') * 4);
    te->setReadOnly(true);

    setWindowTitle(tr("Aseba Studio: Output of last compilation"));

    resize(600, 560);
}

void CompilationLogDialog::hideEvent(QHideEvent* event) {
    if(!isVisible())
        emit hidden();
}

NewNamedValueDialog::NewNamedValueDialog(QString* name, int* value, int min, int max) : name(name), value(value) {
    // create the widgets
    label1 = new QLabel(tr("Name", "Name of the named value (can be a constant, event,...)"));
    line1 = new QLineEdit(*name);
    label2 = new QLabel(tr("Default description", "When no description is given for the named value"));
    line2 = new QSpinBox();
    line2->setRange(min, max);
    line2->setValue(*value);
    QLabel* lineHelp = new QLabel(QString("(%1 ... %2)").arg(min).arg(max));
    QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);

    // create the layout
    auto* mainLayout = new QVBoxLayout();
    mainLayout->addWidget(label1);
    mainLayout->addWidget(line1);
    mainLayout->addWidget(label2);
    mainLayout->addWidget(line2);
    mainLayout->addWidget(lineHelp);
    mainLayout->addWidget(buttonBox);
    setLayout(mainLayout);

    // set modal
    setModal(true);

    // connections
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(okSlot()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(cancelSlot()));
}

bool NewNamedValueDialog::getNamedValue(QString* name, int* value, int min, int max, QString title, QString valueName,
                                        QString valueDescription) {
    NewNamedValueDialog dialog(name, value, min, max);
    dialog.setWindowTitle(title);
    dialog.label1->setText(valueName);
    dialog.label2->setText(valueDescription);
    dialog.resize(500, 0);  // make it wide enough

    int ret = dialog.exec();

    if(ret)
        return true;
    else
        return false;
}

void NewNamedValueDialog::okSlot() {
    *name = line1->text();
    *value = line2->value();
    accept();
}

void NewNamedValueDialog::cancelSlot() {
    *name = "";
    *value = -1;
    reject();
}

MainWindow::MainWindow(const mobsya::ThymioDeviceManagerClient& client, const QVector<QUuid>& targetUuids,
                       QWidget* parent)
    : QMainWindow(parent) {

    nodes = new NodeTabsManager(client);

    // create target

    // create models
    // eventsDescriptionsModel =
    //    new MaskableNamedValuesVectorModel(&commonDefinitions.events, tr("Event number %0"), this);
    // eventsDescriptionsModel->setExtraMimeType("application/aseba-events");
    // constantsDefinitionsModel = new ConstantsModel(&commonDefinitions.constants, this);
    // constantsDefinitionsModel->setExtraMimeType("application/aseba-constants");
    // constantsDefinitionsModel->setEditable(true);

    // create help viwer
    helpViewer.setupWidgets();
    helpViewer.setupConnections();

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
        // clear content
        clearDocumentSpecificTabs();
        // we must only have NodeTab* left, clear content of editors in tabs
        for(int i = 0; i < nodes->count(); i++) {
            auto* tab = dynamic_cast<NodeTab*>(nodes->widget(i));
            Q_ASSERT(tab);
            tab->editor->clear();
        }
        /*constantsDefinitionsModel->clear();
        constantsDefinitionsModel->clearWasModified();
        eventsDescriptionsModel->clear();
        eventsDescriptionsModel->clearWasModified();*/

        // reset opened file name
        clearOpenedFileName(false);
        return true;
    }
    return false;
}

void MainWindow::openFile(const QString& path) {
    // make sure we do not loose changes
    if(askUserBeforeDiscarding() == false)
        return;

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
            QStringList recentFiles = settings.value("recent files").toStringList();
            if(recentFiles.size() > 0) {
                dir = recentFiles[0];
            } else {
                const QStringList stdLocations(QStandardPaths::standardLocations(QStandardPaths::DocumentsLocation));
                dir = !stdLocations.empty() ? stdLocations[0] : "";
            }
        }

        fileName = QFileDialog::getOpenFileName(this, tr("Open Script"), dir, "Aseba scripts (*.aesl)");
    }

    QFile file(fileName);
    if(!file.open(QFile::ReadOnly))
        return;

    // load the document
    QDomDocument document("aesl-source");
    QString errorMsg;
    int errorLine;
    int errorColumn;
    if(document.setContent(&file, false, &errorMsg, &errorLine, &errorColumn)) {
        // remove event and constant definitions
        // eventsDescriptionsModel->clear();
        // constantsDefinitionsModel->clear();
        // delete all absent node tabs
        clearDocumentSpecificTabs();
        // we must only have NodeTab* left, clear content of editors in tabs
        for(int i = 0; i < nodes->count(); i++) {
            auto* tab = dynamic_cast<NodeTab*>(nodes->widget(i));
            Q_ASSERT(tab);
            tab->editor->clear();
        }

        // build list of tabs filled from file to be loaded
        QSet<int> filledList;
        QDomNode domNode = document.documentElement().firstChild();
        while(!domNode.isNull()) {
            if(domNode.isElement()) {
                QDomElement element = domNode.toElement();
                /*if(element.tagName() == "node") {
                    bool prefered;
                    NodeTab* tab = getTabFromName(element.attribute("name"),
                                                  element.attribute("nodeId", nullptr).toUInt(), &prefered);
                    if(prefered) {
                        const int index(nodes->indexOf(tab));
                        assert(index >= 0);
                        filledList.insert(index);
                    }
                }*/
            }
            domNode = domNode.nextSibling();
        }

        // load file
        int noNodeCount = 0;
        actualFileName = fileName;
        domNode = document.documentElement().firstChild();
        while(!domNode.isNull()) {
            if(domNode.isElement()) {
                QDomElement element = domNode.toElement();
                if(element.tagName() == "node") {
                    // load plugins xml data

                    // get text
                    QString text;
                    for(QDomNode n = element.firstChild(); !n.isNull(); n = n.nextSibling()) {
                        QDomText t = n.toText();
                        if(!t.isNull())
                            text += t.data();
                    }

                    // reconstruct nodes
                    bool prefered;
                    const QString nodeName(element.attribute("name"));
                    const unsigned nodeId(element.attribute("nodeId", nullptr).toUInt());
                    NodeTab* tab = nullptr;  // getTabFromName(nodeName, nodeId, &prefered, &filledList);
                    if(tab) {
                        // matching tab name
                        if(prefered) {
                            // the node is the prefered one, fill now
                            tab->editor->setPlainText(text);
                            // note that the node is already marked in filledList
                        } else {
                            const int index(nodes->indexOf(tab));
                            // the node is not filled, fill now
                            tab->editor->setPlainText(text);
                            filledList.insert(index);
                        }
                    } else {
                        // no matching name or no free slot, create an absent tab
                        // nodes->addTab(new AbsentNodeTab(nodeId, nodeName, text, savedPlugins),
                        //              nodeName + tr(" (not available)"));
                        noNodeCount++;
                    }
                } /*else if(element.tagName() == "event") {
                    const QString eventName(element.attribute("name"));
                    const unsigned eventSize(element.attribute("size").toUInt());
                    eventsDescriptionsModel->addNamedValue(
                        NamedValue(eventName.toStdWString(), std::min(unsigned(ASEBA_MAX_EVENT_ARG_SIZE), eventSize)));
                } else if(element.tagName() == "constant") {
                    constantsDefinitionsModel->addNamedValue(
                        NamedValue(element.attribute("name").toStdWString(), element.attribute("value").toInt()));
                } else if(element.tagName() == "keywords") {
                    if(element.attribute("flag") == "true")
                        showKeywordsAct->setChecked(true);
                    else
                        showKeywordsAct->setChecked(false);
                }*/
            }
            domNode = domNode.nextSibling();
        }

        // check if there was some matching problem
        if(noNodeCount)
            QMessageBox::warning(this, tr("Loading"),
                                 tr("%0 scripts have no corresponding nodes in the current network "
                                    "and have not been loaded.")
                                     .arg(noNodeCount));

        // update recent files
        updateRecentFiles(fileName);
        regenerateOpenRecentMenu();

        updateWindowTitle();
    } else {
        QMessageBox::warning(
            this, tr("Loading"),
            tr("Error in XML source file: %0 at line %1, column %2").arg(errorMsg).arg(errorLine).arg(errorColumn));
    }

    file.close();
}

void MainWindow::openRecentFile() {
    auto* entry = dynamic_cast<QAction*>(sender());
    openFile(entry->text());
}

bool MainWindow::save() {
    return saveFile(actualFileName);
}

bool MainWindow::saveFile(const QString& previousFileName) {
    /*    QString fileName = previousFileName;

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

        // initiate DOM tree
        QDomDocument document("aesl-source");
        QDomElement root = document.createElement("network");
        document.appendChild(root);

        root.appendChild(document.createTextNode("\n\n\n"));
        root.appendChild(document.createComment("list of global events"));

        // events
        for(size_t i = 0; i < commonDefinitions.events.size(); i++) {
            QDomElement element = document.createElement("event");
            element.setAttribute("name", QString::fromStdWString(commonDefinitions.events[i].name));
            element.setAttribute("size", QString::number(commonDefinitions.events[i].value));
            root.appendChild(element);
        }

        root.appendChild(document.createTextNode("\n\n\n"));
        root.appendChild(document.createComment("list of constants"));

        // constants
        for(size_t i = 0; i < commonDefinitions.constants.size(); i++) {
            QDomElement element = document.createElement("constant");
            element.setAttribute("name", QString::fromStdWString(commonDefinitions.constants[i].name));
            element.setAttribute("value", QString::number(commonDefinitions.constants[i].value));
            root.appendChild(element);
        }

        // keywords
        root.appendChild(document.createTextNode("\n\n\n"));
        root.appendChild(document.createComment("show keywords state"));

        QDomElement keywords = document.createElement("keywords");
        if(showKeywordsAct->isChecked())
            keywords.setAttribute("flag", "true");
        else
            keywords.setAttribute("flag", "false");
        root.appendChild(keywords);

        // source code
        for(int i = 0; i < nodes->count(); i++) {
            const auto* tab = dynamic_cast<const ScriptTab*>(nodes->widget(i));
            if(tab) {
                QString nodeName;

                const auto* nodeTab = dynamic_cast<const NodeTab*>(tab);
                if(nodeTab)
                    nodeName = target->getName(nodeTab->nodeId());

                const auto* absentNodeTab = dynamic_cast<const AbsentNodeTab*>(tab);
                if(absentNodeTab)
                    nodeName = absentNodeTab->name;

                const QString& nodeContent = tab->editor->toPlainText();
                ScriptTab::SavedPlugins savedPlugins(tab->savePlugins());
                // is there something to save?
                if(!nodeContent.isEmpty() || !savedPlugins.isEmpty()) {
                    root.appendChild(document.createTextNode("\n\n\n"));
                    root.appendChild(document.createComment(QString("node %0").arg(nodeName)));

                    QDomElement element = document.createElement("node");
                    element.setAttribute("name", nodeName);
                    element.setAttribute("nodeId", tab->nodeId());
                    QDomText text = document.createTextNode(nodeContent);
                    element.appendChild(text);
                    if(!savedPlugins.isEmpty()) {
                        QDomElement plugins = document.createElement("toolsPlugins");
                        for(ScriptTab::SavedPlugins::const_iterator it(savedPlugins.begin()); it != savedPlugins.end();
                            ++it) {
                            const NodeToolInterface::SavedContent content(*it);
                            QDomElement plugin(document.createElement(content.first));
                            plugin.appendChild(document.importNode(content.second.documentElement(), true));
                            plugins.appendChild(plugin);
                        }
                        element.appendChild(plugins);
                    }
                    root.appendChild(element);
                }
            }
        }
        root.appendChild(document.createTextNode("\n\n\n"));

        QTextStream out(&file);
        document.save(out, 0);

        sourceModified = false;
        constantsDefinitionsModel->clearWasModified();
        eventsDescriptionsModel->clearWasModified();
        updateWindowTitle();

        return true;
    */
}

void MainWindow::exportMemoriesContent() {
    /*    QString exportFileName = QFileDialog::getSaveFileName(
            this, tr("Export memory content"), QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation),
            "All Files (*);;CSV files (*.csv);;Text files (*.txt)");

        QFile file(exportFileName);
        if(!file.open(QFile::WriteOnly | QFile::Truncate))
            return;

        QTextStream out(&file);

        for(int i = 0; i < nodes->count(); i++) {
            auto* tab = dynamic_cast<NodeTab*>(nodes->widget(i));
            if(tab) {
                const QString nodeName(target->getName(tab->nodeId()));
                const QList<TargetVariablesModel::Variable>& variables(tab->vmMemoryModel->getVariables());

                for(int j = 0; j < variables.size(); ++j) {
                    const TargetVariablesModel::Variable& variable(variables[j]);
                    out << nodeName << "." << variable.name << " ";
                    for(size_t k = 0; k < variable.value.size(); ++k) {
                        out << variable.value[k] << " ";
                    }
                    out << "\n";
                }
            }
        }
    */
}

void MainWindow::copyAll() {
    /*    QString toCopy;
        for(int i = 0; i < nodes->count(); i++) {
            const NodeTab* nodeTab = dynamic_cast<NodeTab*>(nodes->widget(i));
            if(nodeTab) {
                toCopy += QString("# node %0\n").arg(target->getName(nodeTab->nodeId()));
                toCopy += nodeTab->editor->toPlainText();
                toCopy += "\n\n";
            }
            const AbsentNodeTab* absentNodeTab = dynamic_cast<AbsentNodeTab*>(nodes->widget(i));
            if(absentNodeTab) {
                toCopy += QString("# absent node named %0\n").arg(absentNodeTab->name);
                toCopy += absentNodeTab->editor->toPlainText();
                toCopy += "\n\n";
            }
        }
        QApplication::clipboard()->setText(toCopy);
    */
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
    for(int i = 0; i < nodes->count(); i++) {
        auto* tab = dynamic_cast<NodeTab*>(nodes->widget(i));
        Q_ASSERT(tab);
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
    for(int i = 0; i < nodes->count(); i++) {
        auto* tab = dynamic_cast<NodeTab*>(nodes->widget(i));
        if(tab)
            tab->reset();
    }
}

void MainWindow::runAll() {
    /*    for(int i = 0; i < nodes->count(); i++) {
            auto* tab = dynamic_cast<NodeTab*>(nodes->widget(i));
            if(tab)
                target->run(tab->nodeId());
        }
    */
}

void MainWindow::pauseAll() {
    /*    for(int i = 0; i < nodes->count(); i++) {
            auto* tab = dynamic_cast<NodeTab*>(nodes->widget(i));
            if(tab)
                target->pause(tab->nodeId());
        }
    */
}

void MainWindow::stopAll() {
    /*    for(int i = 0; i < nodes->count(); i++) {
            auto* tab = dynamic_cast<NodeTab*>(nodes->widget(i));
            if(tab)
                target->stop(tab->nodeId());
        }
    */
}

void MainWindow::showHidden(bool show) {
    /*
        for(int i = 0; i < nodes->count(); i++) {
            auto* tab = dynamic_cast<NodeTab*>(nodes->widget(i));
            if(tab) {
                tab->vmFunctionsModel->recreateTreeFromDescription(show);
                tab->showHidden = show;
                tab->updateHidden();
            }
        }
        ConfigDialog::setShowHidden(show);
    */
}

void MainWindow::clearAllExecutionError() {
    for(int i = 0; i < nodes->count(); i++) {
        auto* tab = dynamic_cast<NodeTab*>(nodes->widget(i));
        if(tab)
            tab->clearExecutionErrors();
    }
    logger->setStyleSheet("");
}

void MainWindow::uploadReadynessChanged() {
    /* bool ready = true;
     for(int i = 0; i < nodes->count(); i++) {
         auto* tab = dynamic_cast<NodeTab*>(nodes->widget(i));
         if(tab) {
             if(!tab->loadButton->isEnabled()) {
                 ready = false;
                 break;
             }
         }
     }

     loadAllAct->setEnabled(ready);
     writeAllBytecodesAct->setEnabled(ready);
     */
}

void MainWindow::sendEvent() {
    /*    QModelIndex currentRow = eventsDescriptionsView->selectionModel()->currentIndex();
        Q_ASSERT(currentRow.isValid());

        const unsigned eventId = currentRow.row();
        const QString eventName = QString::fromStdWString(commonDefinitions.events[eventId].name);
        const int argsCount = commonDefinitions.events[eventId].value;
        VariablesDataVector data(argsCount);

        if(argsCount > 0) {
            QString argList;
            while(true) {
                bool ok;
                argList =
                    QInputDialog::getText(this, tr("Specify event arguments"),
                                          tr("Please specify the %0 arguments of event
       %1").arg(argsCount).arg(eventName), QLineEdit::Normal, argList, &ok); if(ok) { QStringList args =
       argList.split(QRegExp("[\\s,]+"), QString::SkipEmptyParts); if(args.size() != argsCount) {
                        QMessageBox::warning(this, tr("Wrong number of arguments"),
                                             tr("You gave %0 arguments where event %1 requires %2")
                                                 .arg(args.size())
                                                 .arg(eventName)
                                                 .arg(argsCount));
                        continue;
                    }
                    for(int i = 0; i < args.size(); i++) {
                        data[i] = args.at(i).toShort(&ok);
                        if(!ok) {
                            QMessageBox::warning(this, tr("Invalid value"),
                                                 tr("Invalid value for argument %0 of event %1").arg(i).arg(eventName));
                            break;
                        }
                    }
                    if(ok)
                        break;
                } else
                    return;
            }
        }

        target->sendEvent(eventId, data);
        userEvent(eventId, data);
    */
}

void MainWindow::sendEventIf(const QModelIndex& index) {
    if(index.column() == 0)
        sendEvent();
}

void MainWindow::toggleEventVisibleButton(const QModelIndex& index) {
    // if(index.column() == 2)
    //    eventsDescriptionsModel->toggle(index);
}

void MainWindow::plotEvent() {
#ifdef HAVE_QWT
    QModelIndex currentRow = eventsDescriptionsView->selectionModel()->currentIndex();
    Q_ASSERT(currentRow.isValid());
    const unsigned eventId = currentRow.row();
    plotEvent(eventId);
#endif  // HAVE_QWT
}

void MainWindow::eventContextMenuRequested(const QPoint& pos) {
#ifdef HAVE_QWT
    const QModelIndex index(eventsDescriptionsView->indexAt(pos));
    if(index.isValid() && (index.column() == 0)) {
        const QString eventName(eventsDescriptionsModel->data(index).toString());
        QMenu menu;
        menu.addAction(tr("Plot event %1").arg(eventName));
        const QAction* ret = menu.exec(eventsDescriptionsView->mapToGlobal(pos));
        if(ret) {
            const unsigned eventId = index.row();
            plotEvent(eventId);
        }
    }
#endif  // HAVE_QWT
}

void MainWindow::plotEvent(const unsigned eventId) {
#ifdef HAVE_QWT
    const unsigned eventVariablesCount(
        eventsDescriptionsModel->data(eventsDescriptionsModel->index(eventId, 1)).toUInt());
    const QString eventName(eventsDescriptionsModel->data(eventsDescriptionsModel->index(eventId, 0)).toString());
    const QString tabTitle(tr("plot of %1").arg(eventName));
    nodes->addTab(new EventViewer(eventId, eventName, eventVariablesCount, &eventsViewers), tabTitle, true);
#endif  // HAVE_QWT
}

void MainWindow::logEntryDoubleClicked(QListWidgetItem* item) {
    if(item->data(Qt::UserRole).type() == QVariant::Point) {
        int node = item->data(Qt::UserRole).toPoint().x();
        int line = item->data(Qt::UserRole).toPoint().y();

        NodeTab* tab = nullptr;  // getTabFromId(node);
        Q_ASSERT(tab);
        nodes->setCurrentWidget(tab);
        tab->editor->setTextCursor(QTextCursor(tab->editor->document()->findBlockByLineNumber(line)));
        tab->editor->setFocus();
    }
}

void MainWindow::tabChanged(int index) {
    findDialog->hide();
    auto* tab = dynamic_cast<ScriptTab*>(nodes->widget(index));
    if(currentScriptTab && tab != currentScriptTab) {
        disconnect(cutAct, SIGNAL(triggered()), currentScriptTab->editor, SLOT(cut()));
        disconnect(copyAct, SIGNAL(triggered()), currentScriptTab->editor, SLOT(copy()));
        disconnect(pasteAct, SIGNAL(triggered()), currentScriptTab->editor, SLOT(paste()));
        disconnect(undoAct, SIGNAL(triggered()), currentScriptTab->editor, SLOT(undo()));
        disconnect(redoAct, SIGNAL(triggered()), currentScriptTab->editor, SLOT(redo()));
        disconnect(currentScriptTab->editor, SIGNAL(copyAvailable(bool)), cutAct, SLOT(setEnabled(bool)));
        disconnect(currentScriptTab->editor, SIGNAL(copyAvailable(bool)), copyAct, SLOT(setEnabled(bool)));
        disconnect(currentScriptTab->editor, SIGNAL(undoAvailable(bool)), undoAct, SLOT(setEnabled(bool)));
        disconnect(currentScriptTab->editor, SIGNAL(redoAvailable(bool)), redoAct, SLOT(setEnabled(bool)));
    }
    currentScriptTab = tab;
    if(tab) {
        connect(cutAct, SIGNAL(triggered()), currentScriptTab->editor, SLOT(cut()));
        connect(copyAct, SIGNAL(triggered()), currentScriptTab->editor, SLOT(copy()));
        connect(pasteAct, SIGNAL(triggered()), currentScriptTab->editor, SLOT(paste()));
        connect(undoAct, SIGNAL(triggered()), currentScriptTab->editor, SLOT(undo()));
        connect(redoAct, SIGNAL(triggered()), currentScriptTab->editor, SLOT(redo()));
        connect(currentScriptTab->editor, SIGNAL(copyAvailable(bool)), cutAct, SLOT(setEnabled(bool)));
        connect(currentScriptTab->editor, SIGNAL(copyAvailable(bool)), copyAct, SLOT(setEnabled(bool)));
        connect(currentScriptTab->editor, SIGNAL(undoAvailable(bool)), undoAct, SLOT(setEnabled(bool)));
        connect(currentScriptTab->editor, SIGNAL(redoAvailable(bool)), redoAct, SLOT(setEnabled(bool)));

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

void MainWindow::compilationMessagesWasHidden() {
    showCompilationMsg->setChecked(false);
}

void MainWindow::showMemoryUsage(bool show) {
    for(int i = 0; i < nodes->count(); i++) {
        auto* tab = dynamic_cast<NodeTab*>(nodes->widget(i));
        if(tab)
            tab->showMemoryUsage(show);
    }
    ConfigDialog::setShowMemoryUsage(show);
}

void MainWindow::addEventNameClicked() {
    QString eventName;
    int eventNbArgs = 0;

    /* // prompt the user for the named value
     const bool ok = NewNamedValueDialog::getNamedValue(&eventName, &eventNbArgs, 0, ASEBA_MAX_EVENT_ARG_COUNT,
                                                        tr("Add a new event"), tr("Name:"),
                                                        tr("Number of arguments", "For the newly created event"));

     eventName = eventName.trimmed();
     if(ok && !eventName.isEmpty()) {
         if(commonDefinitions.events.contains(eventName.toStdWString())) {
             QMessageBox::warning(this, tr("Event already exists"), tr("Event %0 already exists.").arg(eventName));
         } else if(!QRegExp(R"(\w(\w|\.)*)").exactMatch(eventName) || eventName[0].isDigit()) {
             QMessageBox::warning(this, tr("Invalid event name"),
                                  tr("Event %0 has an invalid name. Valid names start with an "
                                     "alphabetical character or an \"_\", and continue with any "
                                     "number of alphanumeric characters, \"_\" and \".\"")
                                      .arg(eventName));
         } else {
             eventsDescriptionsModel->addNamedValue(NamedValue(eventName.toStdWString(), eventNbArgs));
         }
     }*/
}

void MainWindow::removeEventNameClicked() {
    /*    QModelIndex currentRow = eventsDescriptionsView->selectionModel()->currentIndex();
        Q_ASSERT(currentRow.isValid());
        // eventsDescriptionsModel->delNamedValue(currentRow.row());

        for(int i = 0; i < nodes->count(); i++) {
            auto* tab = dynamic_cast<NodeTab*>(nodes->widget(i));
            if(tab)
                tab->isSynchronized = false;
        }*/
}

void MainWindow::eventsUpdated(bool indexChanged) {
    if(indexChanged) {
        // statusText->setText(tr("Desynchronised! Please reload."));
        // statusText->show();
    }
    recompileAll();
    updateWindowTitle();
}

void MainWindow::eventsUpdatedDirty() {
    eventsUpdated(true);
}

void MainWindow::eventsDescriptionsSelectionChanged() {
    // bool isSelected = eventsDescriptionsView->selectionModel()->currentIndex().isValid();
    // removeEventNameButton->setEnabled(isSelected);
    // sendEventButton->setEnabled(isSelected);
#ifdef HAVE_QWT
    plotEventButton->setEnabled(isSelected);
#endif  // HAVE_QWT
}

void MainWindow::resetStatusText() {
    /*bool flag = true;

    for(int i = 0; i < nodes->count(); i++) {
        auto* tab = dynamic_cast<NodeTab*>(nodes->widget(i));
        if(tab) {
            if(!tab->isSynchronized) {
                flag = false;
                break;
            }
        }
    }

    if(flag) {
        statusText->clear();
        statusText->hide();
    }*/
}

void MainWindow::addConstantClicked() {
    bool ok;
    QString constantName;
    int constantValue = 0;

    // prompt the user for the named value
    ok = NewNamedValueDialog::getNamedValue(&constantName, &constantValue, -32768, 32767, tr("Add a new constant"),
                                            tr("Name:"), tr("Value", "Value assigned to the constant"));

    if(ok && !constantName.isEmpty()) {
        /*if(constantsDefinitionsModel->validateName(constantName)) {
            constantsDefinitionsModel->addNamedValue(NamedValue(constantName.toStdWString(), constantValue));
            recompileAll();
            updateWindowTitle();
        }/*/
    }
}

void MainWindow::removeConstantClicked() {
    // QModelIndex currentRow = constantsView->selectionModel()->currentIndex();
    // Q_ASSERT(currentRow.isValid());
    // constantsDefinitionsModel->delNamedValue(currentRow.row());

    recompileAll();
    updateWindowTitle();
}

void MainWindow::constantsSelectionChanged() {
    // bool isSelected = constantsView->selectionModel()->currentIndex().isValid();
    // removeConstantButton->setEnabled(isSelected);
}

void MainWindow::recompileAll() {
    for(int i = 0; i < nodes->count(); i++) {
        auto* tab = dynamic_cast<NodeTab*>(nodes->widget(i));
        if(tab)
            tab->compileCodeOnTarget();
    }
}

void MainWindow::writeAllBytecodes() {
    for(int i = 0; i < nodes->count(); i++) {
        auto* tab = dynamic_cast<NodeTab*>(nodes->widget(i));
        // if(tab)
        //    tab->wr();
    }
}

void MainWindow::rebootAllNodes() {
    for(int i = 0; i < nodes->count(); i++) {
        auto* tab = dynamic_cast<NodeTab*>(nodes->widget(i));
        if(tab)
            tab->reboot();
    }
}

void MainWindow::sourceChanged() {
    updateWindowTitle();
}

void MainWindow::showUserManual() {
    helpViewer.showHelp(HelpViewer::USERMANUAL);
}

void MainWindow::clearDocumentSpecificTabs() {
    /*bool changed = false;
    do {
        changed = false;
        for(int i = 0; i < nodes->count(); i++) {
            QWidget* tab = nodes->widget(i);

#ifdef HAVE_QWT
            if(dynamic_cast<AbsentNodeTab*>(tab) || dynamic_cast<EventViewer*>(tab))
#else   // HAVE_QWT
            if(dynamic_cast<AbsentNodeTab*>(tab))
#endif  // HAVE_QWT
            {
                nodes->removeAndDeleteTab(i);
                changed = true;
                break;
            }
        }
    } while(changed);*/
}

void MainWindow::setupWidgets() {
    currentScriptTab = nullptr;
    nodes->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));

    auto* splitter = new QSplitter();
    splitter->addWidget(nodes);
    setCentralWidget(splitter);

    addConstantButton = new QPushButton(QPixmap(QString(":/images/add.png")), "");
    removeConstantButton = new QPushButton(QPixmap(QString(":/images/remove.png")), "");
    addConstantButton->setToolTip(tr("Add a new constant"));
    removeConstantButton->setToolTip(tr("Remove this constant"));
    removeConstantButton->setEnabled(false);

    /*constantsView = new FixedWidthTableView;
    constantsView->setShowGrid(false);
    constantsView->verticalHeader()->hide();
    constantsView->horizontalHeader()->hide();
    // constantsView->setModel(constantsDefinitionsModel);
    constantsView->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    constantsView->setSelectionMode(QAbstractItemView::SingleSelection);
    constantsView->setSelectionBehavior(QAbstractItemView::SelectRows);
    constantsView->setDragDropMode(QAbstractItemView::InternalMove);
    constantsView->setDragEnabled(true);
    constantsView->setDropIndicatorShown(true);
    constantsView->setItemDelegateForColumn(1, new SpinBoxDelegate(-32768, 32767, this));
    constantsView->setMinimumHeight(100);
    // constantsView->setSecondColumnLongestContent("-88888##");
    // constantsView->resizeRowsToContents();


    auto* constantsLayout = new QGridLayout;
    constantsLayout->addWidget(new QLabel(tr("<b>Constants</b>")), 0, 0);
    constantsLayout->setColumnStretch(0, 1);
    constantsLayout->addWidget(addConstantButton, 0, 1);
    constantsLayout->setColumnStretch(1, 0);
    constantsLayout->addWidget(removeConstantButton, 0, 2);
    constantsLayout->setColumnStretch(2, 0);
    constantsLayout->addWidget(constantsView, 1, 0, 1, 3);
    // setColumnStretch

    /*QHBoxLayout* constantsAddRemoveLayout = new QHBoxLayout;;
    constantsAddRemoveLayout->addStretch();
    addConstantButton = new QPushButton(QPixmap(QString(":/images/add.png")), "");
    constantsAddRemoveLayout->addWidget(addConstantButton);
    removeConstantButton = new QPushButton(QPixmap(QString(":/images/remove.png")), "");
    removeConstantButton->setEnabled(false);
    constantsAddRemoveLayout->addWidget(removeConstantButton);

    eventsDockLayout->addLayout(constantsAddRemoveLayout);
    eventsDockLayout->addWidget(constantsView, 1);*/


    /*eventsDockLayout->addWidget(new QLabel(tr("<b>Events</b>")));

    QHBoxLayout* eventsAddRemoveLayout = new QHBoxLayout;;
    eventsAddRemoveLayout->addStretch();
    addEventNameButton = new QPushButton(QPixmap(QString(":/images/add.png")), "");
    eventsAddRemoveLayout->addWidget(addEventNameButton);
    removeEventNameButton = new QPushButton(QPixmap(QString(":/images/remove.png")), "");
    removeEventNameButton->setEnabled(false);
    eventsAddRemoveLayout->addWidget(removeEventNameButton);
    sendEventButton = new QPushButton(QPixmap(QString(":/images/newmsg.png")), "");
    sendEventButton->setEnabled(false);
    eventsAddRemoveLayout->addWidget(sendEventButton);

    eventsDockLayout->addLayout(eventsAddRemoveLayout);

    eventsDockLayout->addWidget(eventsDescriptionsView, 1);*/


    addEventNameButton = new QPushButton(QPixmap(QString(":/images/add.png")), "");
    removeEventNameButton = new QPushButton(QPixmap(QString(":/images/remove.png")), "");
    removeEventNameButton->setEnabled(false);
    sendEventButton = new QPushButton(QPixmap(QString(":/images/newmsg.png")), "");
    sendEventButton->setEnabled(false);

    addEventNameButton->setToolTip(tr("Add a new event"));
    removeEventNameButton->setToolTip(tr("Remove this event"));
    sendEventButton->setToolTip(tr("Send this event"));

#ifdef HAVE_QWT
    plotEventButton = new QPushButton(QPixmap(QString(":/images/plot.png")), "");
    plotEventButton->setEnabled(false);
    plotEventButton->setToolTip(tr("Plot this event"));
#endif  // HAVE_QWT

    /*eventsDescriptionsView = new FixedWidthTableView;
    eventsDescriptionsView->setShowGrid(false);
    eventsDescriptionsView->verticalHeader()->hide();
    eventsDescriptionsView->horizontalHeader()->hide();
    // eventsDescriptionsView->setModel(eventsDescriptionsModel);
    eventsDescriptionsView->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    eventsDescriptionsView->setSelectionMode(QAbstractItemView::SingleSelection);
    eventsDescriptionsView->setSelectionBehavior(QAbstractItemView::SelectRows);
    eventsDescriptionsView->setDragDropMode(QAbstractItemView::InternalMove);
    eventsDescriptionsView->setDragEnabled(true);
    eventsDescriptionsView->setDropIndicatorShown(true);
    eventsDescriptionsView->setItemDelegateForColumn(1, new SpinBoxDelegate(0, ASEBA_MAX_EVENT_ARG_COUNT, this));
    eventsDescriptionsView->setMinimumHeight(100);
    // eventsDescriptionsView->setSecondColumnLongestContent("255###");
    eventsDescriptionsView->resizeRowsToContents();
    eventsDescriptionsView->setContextMenuPolicy(Qt::CustomContextMenu);*/

    /*auto* eventsLayout = new QGridLayout;
    eventsLayout->addWidget(new QLabel(tr("<b>Global Events</b>")), 0, 0, 1, 4);
    eventsLayout->addWidget(addEventNameButton, 1, 0);
    // eventsLayout->setColumnStretch(2, 0);
    eventsLayout->addWidget(removeEventNameButton, 1, 1);
    // eventsLayout->setColumnStretch(3, 0);
    // eventsLayout->setColumnStretch(0, 1);
    eventsLayout->addWidget(sendEventButton, 1, 2);
// eventsLayout->setColumnStretch(1, 0);
#ifdef HAVE_QWT
    eventsLayout->addWidget(plotEventButton, 1, 3);
#endif  // HAVE_QWT
    eventsLayout->addWidget(eventsDescriptionsView, 2, 0, 1, 4);*/

    /*logger = new QListWidget;
    logger->setMinimumSize(80,100);
    logger->setSelectionMode(QAbstractItemView::NoSelection);
    eventsDockLayout->addWidget(logger, 3);
    clearLogger = new QPushButton(tr("Clear"));
    eventsDockLayout->addWidget(clearLogger);*/

    logger = new QListWidget;
    logger->setMinimumSize(80, 100);
    logger->setSelectionMode(QAbstractItemView::NoSelection);
    clearLogger = new QPushButton(tr("Clear"));
    // statusText = new QLabel("Test");
    // statusText->hide();

    /*auto* loggerLayout = new QVBoxLayout;
    loggerLayout->addWidget(statusText);
    loggerLayout->addWidget(logger);
    loggerLayout->addWidget(clearLogger);
    */

    // panel
    // auto* rightPanelSplitter = new QSplitter(Qt::Vertical);

    // QWidget* constantsWidget = new QWidget;
    // constantsWidget->setLayout(constantsLayout);
    // rightPanelSplitter->addWidget(constantsWidget);

    // QWidget* eventsWidget = new QWidget;
    // eventsWidget->setLayout(eventsLayout);
    // rightPanelSplitter->addWidget(eventsWidget);

    // QWidget* loggerWidget = new QWidget;
    // loggerWidget->setLayout(loggerLayout);
    // rightPanelSplitter->addWidget(loggerWidget);

    // main window

    // splitter->addWidget(rightPanelSplitter);
    splitter->setSizes(QList<int>() << 800 << 200);

    // dialog box
    compilationMessageBox = new CompilationLogDialog(this);
    connect(this, SIGNAL(MainWindowClosed()), compilationMessageBox, SLOT(close()));
    findDialog = new FindDialog(this);
    connect(this, SIGNAL(MainWindowClosed()), findDialog, SLOT(close()));

    connect(this, SIGNAL(MainWindowClosed()), &helpViewer, SLOT(close()));
}

void MainWindow::setupConnections() {
    // general connections
    connect(nodes, SIGNAL(currentChanged(int)), SLOT(tabChanged(int)));
    connect(logger, SIGNAL(itemDoubleClicked(QListWidgetItem*)), SLOT(logEntryDoubleClicked(QListWidgetItem*)));
    connect(ConfigDialog::getInstance(), SIGNAL(settingsChanged()), SLOT(applySettings()));

    // global actions
    connect(loadAllAct, SIGNAL(triggered()), SLOT(loadAll()));
    connect(resetAllAct, SIGNAL(triggered()), SLOT(resetAll()));
    connect(runAllAct, SIGNAL(triggered()), SLOT(runAll()));
    connect(pauseAllAct, SIGNAL(triggered()), SLOT(pauseAll()));

    // events
    connect(addEventNameButton, SIGNAL(clicked()), SLOT(addEventNameClicked()));
    connect(removeEventNameButton, SIGNAL(clicked()), SLOT(removeEventNameClicked()));
    connect(sendEventButton, SIGNAL(clicked()), SLOT(sendEvent()));
#ifdef HAVE_QWT
    connect(plotEventButton, SIGNAL(clicked()), SLOT(plotEvent()));
#endif  // HAVE_QWT
    /*connect(eventsDescriptionsView->selectionModel(),
            SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)),
            SLOT(eventsDescriptionsSelectionChanged()));
    connect(eventsDescriptionsView, SIGNAL(doubleClicked(const QModelIndex&)), SLOT(sendEventIf(const QModelIndex&)));
    connect(eventsDescriptionsView, SIGNAL(clicked(const QModelIndex&)),
            SLOT(toggleEventVisibleButton(const QModelIndex&)));
    /*connect(eventsDescriptionsModel, SIGNAL(dataChanged(const QModelIndex&, const QModelIndex&)),
            SLOT(eventsUpdated()));
    connect(eventsDescriptionsModel, SIGNAL(publicRowsInserted()), SLOT(eventsUpdated()));
    connect(eventsDescriptionsModel, SIGNAL(publicRowsRemoved()), SLOT(eventsUpdatedDirty()));
    connect(eventsDescriptionsView, SIGNAL(customContextMenuRequested(const QPoint&)),
            SLOT(eventContextMenuRequested(const QPoint&)));*/

    // logger
    connect(clearLogger, SIGNAL(clicked()), logger, SLOT(clear()));
    connect(clearLogger, SIGNAL(clicked()), SLOT(clearAllExecutionError()));

    // constants
    connect(addConstantButton, SIGNAL(clicked()), SLOT(addConstantClicked()));
    connect(removeConstantButton, SIGNAL(clicked()), SLOT(removeConstantClicked()));
    // connect(constantsView->selectionModel(), SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)),
    //        SLOT(constantsSelectionChanged()));
    // connect(constantsDefinitionsModel, SIGNAL(dataChanged(const QModelIndex&, const QModelIndex&)),
    //         SLOT(recompileAll()));
    // connect(constantsDefinitionsModel, SIGNAL(dataChanged(const QModelIndex&, const QModelIndex&)),
    //       SLOT(updateWindowTitle()));
    /*
        // target events
        connect(target, SIGNAL(nodeConnected(unsigned)), SLOT(nodeConnected(unsigned)));
        connect(target, SIGNAL(nodeDisconnected(unsigned)), SLOT(nodeDisconnected(unsigned)));

        connect(target, SIGNAL(userEvent(unsigned, const VariablesDataVector&)),
                SLOT(userEvent(unsigned, const VariablesDataVector&)));
        connect(target, SIGNAL(userEventsDropped(unsigned)), SLOT(userEventsDropped(unsigned)));
        connect(target, SIGNAL(arrayAccessOutOfBounds(unsigned, unsigned, unsigned, unsigned)),
                SLOT(arrayAccessOutOfBounds(unsigned, unsigned, unsigned, unsigned)));
        connect(target, SIGNAL(divisionByZero(unsigned, unsigned)), SLOT(divisionByZero(unsigned, unsigned)));
        connect(target, SIGNAL(eventExecutionKilled(unsigned, unsigned)), SLOT(eventExecutionKilled(unsigned,
       unsigned))); connect(target, SIGNAL(nodeSpecificError(unsigned, unsigned, QString)),
                SLOT(nodeSpecificError(unsigned, unsigned, QString)));

        connect(target, SIGNAL(executionPosChanged(unsigned, unsigned)), SLOT(executionPosChanged(unsigned,
       unsigned))); connect(target, SIGNAL(executionModeChanged(unsigned, Target::ExecutionMode)),
                SLOT(executionModeChanged(unsigned, Target::ExecutionMode)));
        connect(target, SIGNAL(variablesMemoryEstimatedDirty(unsigned)),
       SLOT(variablesMemoryEstimatedDirty(unsigned)));

        connect(target, SIGNAL(variablesMemoryChanged(unsigned, unsigned, const VariablesDataVector&)),
                SLOT(variablesMemoryChanged(unsigned, unsigned, const VariablesDataVector&)));

        connect(target, SIGNAL(breakpointSetResult(unsigned, unsigned, bool)),
                SLOT(breakpointSetResult(unsigned, unsigned, bool)));
    */
}

void MainWindow::regenerateOpenRecentMenu() {
    openRecentMenu->clear();

    // Add all other actions excepted the one we are processing
    QSettings settings;
    QStringList recentFiles = settings.value("recent files").toStringList();
    for(int i = 0; i < recentFiles.size(); i++) {
        const QString& fileName(recentFiles.at(i));
        openRecentMenu->addAction(fileName, this, SLOT(openRecentFile()));
    }
}

void MainWindow::updateRecentFiles(const QString& fileName) {
    QSettings settings;
    QStringList recentFiles = settings.value("recent files").toStringList();
    if(recentFiles.contains(fileName))
        recentFiles.removeAt(recentFiles.indexOf(fileName));
    recentFiles.push_front(fileName);
    const int maxRecentFiles = 8;
    if(recentFiles.size() > maxRecentFiles)
        recentFiles.pop_back();
    settings.setValue("recent files", recentFiles);
}

void MainWindow::regenerateToolsMenus() {
    /*    writeBytecodeMenu->clear();
        rebootMenu->clear();

        unsigned activeVMCount(0);
        for(int i = 0; i < nodes->count(); i++) {
            auto* tab = dynamic_cast<NodeTab*>(nodes->widget(i));
            if(tab) {
                QAction* act = writeBytecodeMenu->addAction(tr("...inside %0").arg(target->getName(tab->nodeId())), tab,
                                                            SLOT(writeBytecode()));
                connect(tab, SIGNAL(uploadReadynessChanged(bool)), act, SLOT(setEnabled(bool)));

                rebootMenu->addAction(tr("...%0").arg(target->getName(tab->nodeId())), tab, SLOT(reboot()));

                connect(tab, SIGNAL(uploadReadynessChanged(bool)), act, SLOT(setEnabled(bool)));

                ++activeVMCount;
            }
        }

        writeBytecodeMenu->addSeparator();
        writeAllBytecodesAct = writeBytecodeMenu->addAction(tr("...inside all nodes"), this, SLOT(writeAllBytecodes()));

        rebootMenu->addSeparator();
        rebootMenu->addAction(tr("...all nodes"), this, SLOT(rebootAllNodes()));

        globalToolBar->setVisible(activeVMCount > 1);
    */
}

void MainWindow::generateHelpMenu() {
    helpMenu->addAction(tr("&User Manual..."), this, SLOT(showUserManual()), QKeySequence::HelpContents);
    helpMenu->addSeparator();

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
    for(int i = 0; i < nodes->count(); i++) {
        auto* tab = dynamic_cast<NodeTab*>(nodes->widget(i));
        if(tab)
            productIds.insert(tab->productId());
    }
    for(auto it(productIds.begin()); it != productIds.end(); ++it) {
        QAction* action;
        switch(*it) {
            case ASEBA_PID_THYMIO2:
                action = new QAction(tr("Thymio programming tutorial..."), helpMenu);
                connect(action, SIGNAL(triggered()), SLOT(openToUrlFromAction()));
                action->setData(QUrl(tr("http://aseba.wikidot.com/en:thymiotutoriel")));
                targetSpecificHelp.append(action);
                helpMenu->insertAction(helpMenuTargetSpecificSeparator, action);
                action = new QAction(tr("Thymio programming interface..."), helpMenu);
                connect(action, SIGNAL(triggered()), SLOT(openToUrlFromAction()));
                action->setData(QUrl(tr("http://aseba.wikidot.com/en:thymioapi")));
                targetSpecificHelp.append(action);
                helpMenu->insertAction(helpMenuTargetSpecificSeparator, action);
                break;

            case ASEBA_PID_CHALLENGE:
                action = new QAction(tr("Challenge tutorial..."), helpMenu);
                connect(action, SIGNAL(triggered()), SLOT(openToUrlFromAction()));
                action->setData(QUrl(tr("http://aseba.wikidot.com/en:gettingstarted")));
                targetSpecificHelp.append(action);
                helpMenu->insertAction(helpMenuTargetSpecificSeparator, action);
                break;

            case ASEBA_PID_MARXBOT:
                action = new QAction(tr("MarXbot user manual..."), helpMenu);
                connect(action, SIGNAL(triggered()), SLOT(openToUrlFromAction()));
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

    fileMenu->addAction(QIcon(":/images/filesave.png"), tr("&Save..."), this, SLOT(save()), QKeySequence::Save);
    fileMenu->addAction(QIcon(":/images/filesaveas.png"), tr("Save &As..."), this, SLOT(saveFile()),
                        QKeySequence::SaveAs);

    fileMenu->addSeparator();
    fileMenu->addAction(QIcon(":/images/filesaveas.png"), tr("Export &memories content..."), this,
                        SLOT(exportMemoriesContent()));

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
    connect(findAct, SIGNAL(triggered()), SLOT(findTriggered()));
    findAct->setEnabled(false);

    replaceAct = new QAction(QIcon(":/images/edit.png"), tr("&Replace..."), this);
    replaceAct->setShortcut(QKeySequence::Replace);
    connect(replaceAct, SIGNAL(triggered()), SLOT(replaceTriggered()));
    replaceAct->setEnabled(false);

    goToLineAct = new QAction(QIcon(":/images/goto.png"), tr("&Go To Line..."), this);
    goToLineAct->setShortcut(tr("Ctrl+G", "Edit|Go To Line"));
    goToLineAct->setEnabled(false);
    connect(goToLineAct, SIGNAL(triggered()), SLOT(goToLine()));

    commentAct = new QAction(tr("Comment the selection"), this);
    commentAct->setShortcut(tr("Ctrl+D", "Edit|Comment the selection"));
    connect(commentAct, SIGNAL(triggered()), SLOT(commentTriggered()));

    uncommentAct = new QAction(tr("Uncomment the selection"), this);
    uncommentAct->setShortcut(tr("Shift+Ctrl+D", "Edit|Uncomment the selection"));
    connect(uncommentAct, SIGNAL(triggered()), SLOT(uncommentTriggered()));

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
    connect(showMemoryUsageAct, SIGNAL(toggled(bool)), SLOT(showMemoryUsage(bool)));

    showHiddenAct = new QAction(tr("S&how hidden variables and functions"), this);
    showHiddenAct->setCheckable(true);
    connect(showHiddenAct, SIGNAL(toggled(bool)), SLOT(showHidden(bool)));

    showLineNumbers = new QAction(tr("Show &Line Numbers"), this);
    showLineNumbers->setShortcut(tr("F11", "View|Show Line Numbers"));
    showLineNumbers->setCheckable(true);
    connect(showLineNumbers, SIGNAL(toggled(bool)), SLOT(showLineNumbersChanged(bool)));

    zoomInAct = new QAction(tr("&Increase font size"), this);
    zoomInAct->setShortcut(QKeySequence::ZoomIn);
    zoomInAct->setEnabled(false);
    connect(zoomInAct, SIGNAL(triggered()), SLOT(zoomIn()));

    zoomOutAct = new QAction(tr("&Decrease font size"), this);
    zoomOutAct->setShortcut(QKeySequence::ZoomOut);
    zoomOutAct->setEnabled(false);
    connect(zoomOutAct, SIGNAL(triggered()), SLOT(zoomOut()));

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

    // Debug actions
    loadAllAct = new QAction(QIcon(":/images/upload.png"), tr("&Load all"), this);
    loadAllAct->setShortcut(tr("F7", "Load|Load all"));

    resetAllAct = new QAction(QIcon(":/images/reset.png"), tr("&Reset all"), this);
    resetAllAct->setShortcut(tr("F8", "Debug|Reset all"));

    runAllAct = new QAction(QIcon(":/images/play.png"), tr("Ru&n all"), this);
    runAllAct->setShortcut(tr("F9", "Debug|Run all"));

    pauseAllAct = new QAction(QIcon(":/images/pause.png"), tr("&Pause all"), this);
    pauseAllAct->setShortcut(tr("F10", "Debug|Pause all"));

    // Debug toolbar
    globalToolBar = addToolBar(tr("Debug"));
    globalToolBar->setObjectName("debug toolbar");
    globalToolBar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    globalToolBar->addAction(loadAllAct);
    globalToolBar->addAction(resetAllAct);
    globalToolBar->addAction(runAllAct);
    globalToolBar->addAction(pauseAllAct);

    // Debug menu
    toggleBreakpointAct = new QAction(tr("Toggle breakpoint"), this);
    toggleBreakpointAct->setShortcut(tr("Ctrl+B", "Debug|Toggle breakpoint"));
    connect(toggleBreakpointAct, SIGNAL(triggered()), SLOT(toggleBreakpoint()));

    clearAllBreakpointsAct = new QAction(tr("Clear all breakpoints"), this);
    // clearAllBreakpointsAct->setShortcut();
    connect(clearAllBreakpointsAct, SIGNAL(triggered()), SLOT(clearAllBreakpoints()));

    QMenu* debugMenu = new QMenu(tr("&Debug"), this);
    menuBar()->addMenu(debugMenu);
    debugMenu->addAction(toggleBreakpointAct);
    debugMenu->addAction(clearAllBreakpointsAct);
    debugMenu->addSeparator();
    debugMenu->addAction(loadAllAct);
    debugMenu->addAction(resetAllAct);
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
    connect(showCompilationMsg, SIGNAL(toggled(bool)), SLOT(showCompilationMessages(bool)));
    connect(compilationMessageBox, SIGNAL(hidden()), SLOT(compilationMessagesWasHidden()));
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
        docName = actualFileName.mid(actualFileName.lastIndexOf("/") + 1);

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
    result = restoreGeometry(settings.value("MainWindow/geometry").toByteArray());
    restoreState(settings.value("MainWindow/windowState").toByteArray());
    return result;
}

void MainWindow::writeSettings() {
    QSettings settings;
    settings.setValue("MainWindow/geometry", saveGeometry());
    settings.setValue("MainWindow/windowState", saveState());
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
        docName = actualFileName.mid(actualFileName.lastIndexOf("/") + 1);

    setWindowTitle(tr("%0 %1- Aseba Studio").arg(docName).arg(modifiedText));
}

void MainWindow::applySettings() {
    showMemoryUsageAct->setChecked(ConfigDialog::getShowMemoryUsage());
    showHiddenAct->setChecked(ConfigDialog::getShowHidden());
    showLineNumbers->setChecked(ConfigDialog::getShowLineNumbers());
}

void MainWindow::clearOpenedFileName(bool isModified) {
    actualFileName.clear();
    updateWindowTitle();
}

/*@}*/
}  // namespace Aseba
