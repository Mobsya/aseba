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

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QString>
#include <QMessageBox>
#include <QItemSelection>
#include <QTableView>
#include <QSplitter>
#include <QTextEdit>
#include <QMultiMap>
#include <QTabWidget>
#include <QCloseEvent>
#include <QFuture>
#include <QFutureWatcher>
#include <QToolButton>
#include <QToolBar>
#include <QCompleter>
#include <QSortFilterProxyModel>
#include <QSignalMapper>

#include "StudioAeslEditor.h"
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include "ConfigDialog.h"

#include <aseba/qt-thymio-dm-client-lib/thymionode.h>
#include <aseba/qt-thymio-dm-client-lib/thymiodevicemanagerclient.h>

class QLabel;
class QSpinBox;
class QGroupBox;
class QPushButton;
class QListWidget;
class QListWidgetItem;
class QTreeView;
class QTranslator;
// class QTextBrowser;
class QToolBox;
class QCheckBox;

namespace Aseba {
/** \addtogroup studio */
/*@{*/

class TargetVariablesModel;
class TargetFunctionsModel;
class TargetMemoryModel;
class MaskableNamedValuesVectorModel;
class FlatVariablesModel;
class ConstantsModel;
class StudioAeslEditor;
class AeslLineNumberSidebar;
class AeslBreakpointSidebar;
class AeslHighlighter;
class EventViewer;
class NodeTabsManager;
class DraggableListWidget;
class FindDialog;
class NodeTab;
class MainWindow;

class CompilationLogDialog : public QDialog {
    Q_OBJECT

public:
    CompilationLogDialog(QWidget* parent = nullptr);
    void setText(const QString& text) {
        te->setText(text);
    }
signals:
    void hidden();

protected:
    void hideEvent(QHideEvent* event) override;
    QTextEdit* te;
};

//! Studio main window
class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(const mobsya::ThymioDeviceManagerClient& client, const QVector<QUuid>& targetUuids,
               QWidget* parent = nullptr);
    ~MainWindow() override;

Q_SIGNALS:
    void MainWindowClosed();

private Q_SLOTS:
    void tabAdded(int index);

    void about();
    bool newFile();
    int openFile(const QString& path = QString());
    void openRecentFile();
    void ExportCode();

    bool save();
    bool saveFile(const QString& previousFileName = QString());
    void exportMemoriesContent();
    void copyAll();
    void findTriggered();
    void replaceTriggered();
    void commentTriggered();
    void uncommentTriggered();
    void showLineNumbersChanged(bool state);
    void goToLine();
    void zoomIn();
    void zoomOut();
    void showSettings();

    void toggleBreakpoint();
    void clearAllBreakpoints();

    void resetAll();
    void runAll();
    void pauseAll();
    void stopAll();

    void clearAllExecutionError();

    void tabChanged(int);
    void eventContextMenuRequested(const QPoint& pos);
    void showCompilationMessages(bool doShown);
    void compilationMessagesWasHidden();
    void showMemoryUsage(bool show);

    void recompileAll();
    void writeAllBytecodes();
    void rebootAllNodes();

    void sourceChanged();
    void updateWindowTitle();

    void openToUrlFromAction() const;

public Q_SLOTS:
    void applySettings();
    void connectToDevice(QUuid);

private:
    // utility functions
    int getIndexFromId(unsigned node) const;
    NodeTab* getTabFromId(unsigned node) const;
    NodeTab* getTabFromName(const QString& name, unsigned preferedId = 0, bool* isPrefered = nullptr,
                            QSet<int>* filledList = nullptr) const;
    void clearDocumentSpecificTabs();
    bool askUserBeforeDiscarding();

    // gui initialisation code
    void regenerateOpenRecentMenu();
    void updateRecentFiles(const QString& fileName, bool to_delete = false);
    void regenerateToolsMenus();
    void generateHelpMenu();
    void regenerateHelpMenu();
    void setupWidgets();
    void setupConnections();
    void setupMenu();
    void closeEvent(QCloseEvent* event) override;
    bool readSettings();
    void writeSettings();
    void clearOpenedFileName(bool isModified);

    // tabs and nodes
    friend class NodeTab;
    friend class StudioAeslEditor;
    NodeTabsManager* nodes;
    ScriptTab* currentScriptTab;

    // events
    QPushButton* addEventNameButton;
    QPushButton* removeEventNameButton;
    QPushButton* sendEventButton;

    QListWidget* logger;
    QPushButton* clearLogger;
    QLabel* statusText;  // Jiwon
    // FixedWidthTableView* eventsDescriptionsView;

    // global buttons
    QAction* stopAllAct;
    QAction* runAllAct;
    QAction* pauseAllAct;
    QToolBar* globalToolBar;

    // open recent actions
    QMenu* openRecentMenu;

    // tools
    QMenu* writeBytecodeMenu;
    QAction* writeAllBytecodesAct;
    QMenu* rebootMenu;
    QMenu* helpMenu;
    using ActionList = QList<QAction*>;
    QAction* helpMenuTargetSpecificSeparator;
    ActionList targetSpecificHelp;

    // Menu action that need dynamic reconnection
    QAction* cutAct;
    QAction* copyAct;
    QAction* pasteAct;
    QAction* undoAct;
    QAction* redoAct;
    QAction* findAct;
    QAction* replaceAct;
    QAction* commentAct;
    QAction* uncommentAct;
    QAction* showLineNumbers;
    QAction* goToLineAct;
    QAction* zoomInAct;
    QAction* zoomOutAct;
    QAction* toggleBreakpointAct;
    QAction* clearAllBreakpointsAct;
    QAction* showHiddenAct;
    QAction* showCompilationMsg;
    QAction* showMemoryUsageAct;

    // gui helper stuff
    CompilationLogDialog* compilationMessageBox;  //!< box to show last compilation messages
    FindDialog* findDialog;                       //!< find dialog
    QString actualFileName;                       //!< name of opened file, "" if new
};

/*@}*/
}  // namespace Aseba

#endif
