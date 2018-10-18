#pragma once
#include <QtWidgets>
#include "Target.h"
#include "AeslEditor.h"
#include <aseba/qt-thymio-dm-client-lib/thymionode.h>

namespace Aseba {

class StudioAeslEditor;

//! Tab holding code (instead of plot)
class ScriptTab {
public:
    ScriptTab() {}
    virtual ~ScriptTab() = default;

protected:
    void createEditor();

    friend class MainWindow;
    StudioAeslEditor* editor;
    AeslLineNumberSidebar* linenumbers;
    AeslBreakpointSidebar* breakpoints;
    AeslHighlighter* highlighter;
};

class MainWindow;
class NodeTab : public QSplitter, public ScriptTab {
    Q_OBJECT

public:
    NodeTab(QWidget* parent = nullptr);
    ~NodeTab() override;
    unsigned productId() const {
        return 0;
    }

    void setThymio(std::shared_ptr<mobsya::ThymioNode> node);

Q_SIGNALS:
    void uploadReadynessChanged(bool);
    void compilationSucceed();
    void compilationFailed();
    void executionStarted();
    void executionPaused();
    void executionStopped();
    void executionStateChanged();

protected:
    void setupWidgets();
    void setupConnections();

public Q_SLOTS:
    void clearExecutionErrors();
    void refreshCompleterModel(LocalContext context);

    // void sortCompleterModel();

protected Q_SLOTS:
    void reset();
    void run();
    void pause();
    void step();
    void reboot();
    void synchronizeVariablesChecked(bool checked);


    void setVariableValues(unsigned, const VariablesDataVector&);
    void insertVariableName(const QModelIndex&);

    void editorContentChanged();
    void compileCodeOnTarget();
    void markTargetUnsynced();

    void showMemoryUsage(bool show);

    void cursorMoved();
    void goToError();

    void setBreakpoint(unsigned line);
    void clearBreakpoint(unsigned line);
    void breakpointClearedAll();
    void breakpointSetResult(unsigned line, bool success);

    void executionPosChanged(unsigned line);
    void executionModeChanged(Target::ExecutionMode mode);


    void updateHidden();

    void compilationCompleted();

private:
    void updateMemoryUsage(const mobsya::CompilationResult& res);
    void handleCompilationError(const mobsya::CompilationResult& res);


    void rehighlight();
    void handleCompletion();

    void reSetBreakpoints();

    // editor properties code
    bool setEditorProperty(const QString& property, const QVariant& value, unsigned line, bool removeOld = false);
    bool clearEditorProperty(const QString& property, unsigned line);
    bool clearEditorProperty(const QString& property);
    void switchEditorProperty(const QString& oldProperty, const QString& newProperty);
    void saveBytecode() const;

private:
    friend class MainWindow;
    friend class StudioAeslEditor;
    friend class NodeTabsManager;

    std::shared_ptr<mobsya::ThymioNode> m_thymio;
    mobsya::CompilationResultWatcher* m_compilation_watcher;

    QLabel* cursorPosText;
    QLabel* compilationResultImage;
    QLabel* compilationResultText;
    QLabel* memoryUsageText;

    QLabel* executionModeLabel;
    QPushButton* stopButton;
    QPushButton* pauseButton;
    QPushButton* runButton;
    QPushButton* nextButton;
    QCheckBox* synchronizeVariablesToogle;

    // keywords

    // TargetVariablesModel* vmMemoryModel;
    // TargetSubroutinesModel* vmSubroutinesModel;
    QTreeView* vmMemoryView;
    QLineEdit* vmMemoryFilter;

    // TargetFunctionsModel* vmFunctionsModel;
    QTreeView* vmFunctionsView;

    // DraggableListWidget* vmLocalEvents;

    QCompleter* completer;
    QAbstractItemModel* eventAggregator;
    QAbstractItemModel* variableAggregator;
    QSortFilterProxyModel* sortingProxy;
    // TreeChainsawFilter* functionsFlatModel;

    QToolBox* toolBox;

    QString lastCompiledSource;  //!< content of last source considered for compilation following
    QString lastLoadedSource;

    //!< a textChanged signal
    int errorPos;   //!< position of last error, -1 if compilation was success
    int currentPC;  //!< current program counter
    Target::ExecutionMode previousMode;
    bool showHidden;
};

}  // namespace Aseba