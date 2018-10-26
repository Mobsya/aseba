#pragma once
#include <QtWidgets>
#include "TargetModels.h"
#include "AeslEditor.h"
#include "NamedValuesVectorModel.h"
#include "TargetFunctionsModel.h"
#include "VariablesModel.h"

#include "ModelAggregator.h"
#include "CustomWidgets.h"
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
    AeslBreakpointSidebar* m_breakpointsSidebar;
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
    void updateAsebaVMDescription();
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
    void onVariablesChanged(const mobsya::ThymioNode::VariableMap& vars);
    void setVariable(const QString& k, const mobsya::ThymioVariable& value);

    void resetVariables();


    void setVariableValues(unsigned, const VariablesDataVector&);

    void editorContentChanged();
    void compileCodeOnTarget();
    void markTargetUnsynced();

    void showMemoryUsage(bool show);

    void cursorMoved();
    void goToError();

    void setBreakpoint(unsigned line);
    void clearBreakpoint(unsigned line);
    void breakpointClearedAll();
    void breakpointsChanged();

    void onExecutionPosChanged(unsigned line);
    void onExecutionStateChanged();
    void onAsebaVMDescriptionChanged();

    void compilationCompleted();

    void showHidden(bool show);

private:
    QVector<unsigned> breakpoints() const;
    void updateBreakpoints();
    void updateMemoryUsage(const mobsya::CompilationResult& res);
    void handleCompilationError(const mobsya::CompilationResult& res);


    void rehighlight();

    // editor properties code
    bool setEditorProperty(const QString& property, const QVariant& value, unsigned line, bool removeOld = false);
    bool clearEditorProperty(const QString& property, unsigned line);
    bool clearEditorProperty(const QString& property);
    void switchEditorProperty(const QString& oldProperty, const QString& newProperty);
    void saveBytecode() const;

private:
    QVector<unsigned> m_breakpoints;


    friend class MainWindow;
    friend class StudioAeslEditor;
    friend class NodeTabsManager;

    std::shared_ptr<mobsya::ThymioNode> m_thymio;
    mobsya::CompilationRequestWatcher* m_compilation_watcher;
    mobsya::BreakpointsRequestWatcher* m_breakpoints_watcher;
    mobsya::AsebaVMDescriptionRequestWatcher* m_aseba_vm_description_watcher;

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

    // TargetSubroutinesModel* vmSubroutinesModel;

    VariablesModel m_vm_variables_model;
    VariablesFilterModel m_vm_variables_filter_model;
    QTreeView* m_vm_variables_view;
    QLineEdit* m_vm_variables_filter_input;

    QTreeView* vmFunctionsView;
    DraggableListWidget* vmLocalEvents;
    TargetFunctionsModel vmFunctionsModel;

    QCompleter* completer;
    QAbstractItemModel* eventAggregator;
    QAbstractItemModel* variableAggregator;
    QSortFilterProxyModel* sortingProxy;
    TreeChainsawFilter* functionsFlatModel;

    QToolBox* toolBox;

    QString lastCompiledSource;  //!< content of last source considered for compilation following
    QString lastLoadedSource;

    //!< a textChanged signal
    int errorPos;   //!< position of last error, -1 if compilation was success
    int currentPC;  //!< current program counter
};

}  // namespace Aseba