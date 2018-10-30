#include "NodeTab.h"
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
#include "StudioAeslEditor.h"
#include "CustomDelegate.h"
#include "CustomWidgets.h"
#include "ModelAggregator.h"
#include "ConfigDialog.h"


namespace Aseba {

void ScriptTab::createEditor() {
    // editor widget
    editor = new StudioAeslEditor(this);
    m_breakpointsSidebar = new AeslBreakpointSidebar(editor);
    linenumbers = new AeslLineNumberSidebar(editor);
    highlighter = new AeslHighlighter(editor, editor->document());
}

NodeTab::NodeTab(QWidget* parent)
    : QSplitter(parent)
    , ScriptTab()
    , m_compilation_watcher(new mobsya::CompilationRequestWatcher(this))
    , m_breakpoints_watcher(new mobsya::BreakpointsRequestWatcher(this))
    , m_aseba_vm_description_watcher(new mobsya::AsebaVMDescriptionRequestWatcher(this))
    , currentPC(-1) {
    errorPos = -1;


    m_vm_variables_filter_model.setSourceModel(&m_vm_variables_model);
    m_vm_variables_filter_model.setFilterCaseSensitivity(Qt::CaseInsensitive);

    connect(m_compilation_watcher, &mobsya::CompilationRequestWatcher::finished, this, &NodeTab::compilationCompleted);
    connect(m_breakpoints_watcher, &mobsya::BreakpointsRequestWatcher::finished, this, &NodeTab::breakpointsChanged);
    connect(m_aseba_vm_description_watcher, &mobsya::AsebaVMDescriptionRequestWatcher::finished, this,
            &NodeTab::onAsebaVMDescriptionChanged);
    connect(&m_vm_variables_model, &VariablesModel::variableChanged, this, &NodeTab::setVariable);


    /*  // create models
      vmFunctionsModel = new TargetFunctionsModel(target->getDescription(id), showHidden, this);
      vmMemoryModel = new TargetVariablesModel(this);
      variablesModel = vmMemoryModel;
      subscribeToVariableOfInterest(ASEBA_PID_VAR_NAME);
      */

    // create gui
    setupWidgets();
    setupConnections();


    connect(&m_constants_model, &ConstantsModel::constantModified, [this](const QString& name, const QVariant& value) {
        this->setVariable(name, {value, true});
    });
    m_constantsWidget->setModel(&m_constants_model);
    m_eventsWidget->setModel(&m_events_model);

    // create aggregated models
    // local and global events
    auto* aggregator = new ModelAggregator(this);
    // aggregator->addModel(vmLocalEvents->model());
    // aggregator->addModel(mainWindow->eventsDescriptionsModel);
    eventAggregator = aggregator;
    // variables and constants
    aggregator = new ModelAggregator(this);
    // aggregator->addModel(vmMemoryModel);
    // aggregator->addModel(mainWindow->constantsDefinitionsModel);
    variableAggregator = aggregator;

    // create the sorting proxy
    sortingProxy = new QSortFilterProxyModel(this);
    sortingProxy->setDynamicSortFilter(true);
    sortingProxy->setSortCaseSensitivity(Qt::CaseInsensitive);
    sortingProxy->setSortRole(Qt::DisplayRole);

    // create the chainsaw filter for native functions
    functionsFlatModel = new TreeChainsawFilter(this);
    functionsFlatModel->setSourceModel(&vmFunctionsModel);

    // create the model for subroutines
    // vmSubroutinesModel = new TargetSubroutinesModel(this);

    editor->setFocus();
    setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
}

NodeTab::~NodeTab() {}

void NodeTab::setThymio(std::shared_ptr<mobsya::ThymioNode> node) {
    m_thymio = node;
    if(node) {
        if(node->status() == mobsya::ThymioNode::Status::Available)
            node->lock();
        auto ptr = node.get();
        node->setWatchVariablesEnabled(synchronizeVariablesToogle->isChecked());
        node->setWatchEventsEnabled(true);
        node->setWatchVMExecutionStateEnabled(true);
        m_vm_variables_model.clear();

        connect(node.get(), &mobsya::ThymioNode::vmExecutionStarted, this, &NodeTab::executionStarted);
        connect(node.get(), &mobsya::ThymioNode::vmExecutionPaused, this, &NodeTab::executionPaused);
        connect(node.get(), &mobsya::ThymioNode::vmExecutionPaused, this, &NodeTab::onExecutionPosChanged);
        connect(node.get(), &mobsya::ThymioNode::vmExecutionStopped, this, &NodeTab::executionStopped);
        connect(node.get(), &mobsya::ThymioNode::vmExecutionStateChanged, this, &NodeTab::executionStateChanged);
        connect(node.get(), &mobsya::ThymioNode::vmExecutionStateChanged, this, &NodeTab::onExecutionStateChanged);
        connect(node.get(), &mobsya::ThymioNode::variablesChanged, this, &NodeTab::onVariablesChanged);
        connect(node.get(), &mobsya::ThymioNode::eventsTableChanged, this, &NodeTab::onGlobalEventsTableChanged);
        connect(node.get(), &mobsya::ThymioNode::events, this, &NodeTab::onEvents);


        connect(ptr, &mobsya::ThymioNode::statusChanged, [node]() {
            if(node->status() == mobsya::ThymioNode::Status::Available)
                node->lock();
        });

        updateAsebaVMDescription();
    }
}

const std::shared_ptr<const mobsya::ThymioNode> NodeTab::thymio() const {
    return m_thymio;
}

void NodeTab::reset() {
    clearExecutionErrors();
    if(m_thymio) {
        m_thymio->load_aseba_code({});
        m_thymio->stop();
    }
    lastLoadedSource.clear();
    editor->debugging = false;
    currentPC = -1;
    resetVariables();
}

void NodeTab::resetVariables() {
    m_vm_variables_model.clear();
    if(m_thymio) {
        m_thymio->setWatchVariablesEnabled(false);
        if(synchronizeVariablesToogle->isChecked())
            m_thymio->setWatchVariablesEnabled(true);
    }
}

void NodeTab::run() {
    if(!m_thymio) {
        return;
    }

    resetVariables();

    editor->debugging = false;

    auto set_breakpoints_and_run = [this] {
        auto bpwatcher = new mobsya::BreakpointsRequestWatcher(this);
        connect(bpwatcher, &mobsya::BreakpointsRequestWatcher::finished, [this, bpwatcher] {
            bpwatcher->deleteLater();
            auto bps = [bpwatcher]() -> QVector<unsigned> {
                if(!bpwatcher->success())
                    return {};
                auto bps = bpwatcher->getResult().breakpoints();
                std::sort(bps.begin(), bps.end());
                return bps;
            }();
            if(bps.empty() || bps.first() != 1)
                m_thymio->run();
            else {
                m_thymio->stepToNextLine();
            }
        });
        auto bps = breakpoints();
        auto req = m_thymio->setBreakPoints(bps);
        bpwatcher->setRequest(req);
        m_breakpoints_watcher->setRequest(req);
    };

    auto code = editor->toPlainText();
    if(m_thymio->vmExecutionState() == mobsya::ThymioNode::VMExecutionState::Paused) {
        m_thymio->run();
        return;
    }
    auto watcher = new mobsya::CompilationRequestWatcher(this);
    connect(watcher, &mobsya::CompilationRequestWatcher::finished, [code, this, watcher, set_breakpoints_and_run] {
        watcher->deleteLater();
        if(watcher->success() && watcher->getResult().success()) {
            lastLoadedSource = code;
            currentPC = -1;
            set_breakpoints_and_run();
        }
    });
    auto req = m_thymio->load_aseba_code(code.toUtf8());
    watcher->setRequest(req);
    m_compilation_watcher->setRequest(req);
}

void NodeTab::pause() {
    editor->debugging = true;
    m_thymio->pause();
}

void NodeTab::compileCodeOnTarget() {
    if(!m_thymio)
        return;

    auto code = editor->toPlainText();
    m_compilation_watcher->setRequest(m_thymio->compile_aseba_code(code.toUtf8()));
}

void NodeTab::step() {
    editor->debugging = true;
    m_thymio->stepToNextLine();
}

void NodeTab::reboot() {
    m_thymio->reboot();
}

void NodeTab::compilationCompleted() {
    if(m_compilation_watcher->isCanceled()) {
        Q_EMIT compilationFailed();
        return;
    }
    if(!m_compilation_watcher->success()) {
        // Deal with network error
        Q_EMIT compilationFailed();
        return;
    }

    auto res = m_compilation_watcher->getResult();


    clearEditorProperty(QStringLiteral("errorPos"));

    updateMemoryUsage(res);
    handleCompilationError(res);


    if(res.success()) {
        Q_EMIT compilationSucceed();
        errorPos = -1;
    } else {
        Q_EMIT compilationFailed();
        emit uploadReadynessChanged(false);
    }

    rehighlight();
}

void NodeTab::updateMemoryUsage(const mobsya::CompilationResult& res) {
    if(res.success()) {
        const QString variableText =
            tr("variables: %1/%2 (%3 %)")
                .arg(res.variables_size())
                .arg(res.variables_total_size())
                .arg((double)res.variables_size() * 100. / res.variables_total_size(), 0, 'f', 1);
        const QString bytecodeText =
            tr("bytecode: %1/%2 (%3 %)")
                .arg(res.bytecode_size())
                .arg(res.variables_total_size())
                .arg((double)res.bytecode_size() * 100. / res.bytecode_total_size(), 0, 'f', 1);
        memoryUsageText->setText(trUtf8("<b>Memory usage</b> : %1, %2").arg(variableText).arg(bytecodeText));
    }
    memoryUsageText->setVisible(res.success());
}

void NodeTab::handleCompilationError(const mobsya::CompilationResult& res) {
    compilationResultText->setVisible(!res.success());
    compilationResultImage->setVisible(!res.success());
    if(res.success()) {
        return;
    }
    compilationResultText->setText(res.error().errorMessage());
    compilationResultImage->setPixmap(QPixmap(QStringLiteral(":/images/warning.png")));

    if(res.error().charater()) {
        errorPos = res.error().charater();
        QTextBlock textBlock = editor->document()->findBlock(errorPos);
        int posInBlock = errorPos - textBlock.position();
        if(textBlock.userData())
            polymorphic_downcast<AeslEditorUserData*>(textBlock.userData())->properties[QStringLiteral("errorPos")] =
                posInBlock;
        else
            textBlock.setUserData(new AeslEditorUserData(QStringLiteral("errorPos"), posInBlock));
    }
}

void NodeTab::setBreakpoint(unsigned line) {
    line = line + 1;
    if(!m_breakpoints.contains(line)) {
        m_breakpoints.append(line);
    }
    updateBreakpoints();
}

void NodeTab::clearBreakpoint(unsigned line) {
    line = line + 1;
    m_breakpoints.removeOne(line);
    updateBreakpoints();
}

void NodeTab::breakpointClearedAll() {
    m_breakpoints.clear();
    updateBreakpoints();
}

void NodeTab::updateBreakpoints() {
    rehighlight();
    m_breakpoints = breakpoints();
    auto code = editor->toPlainText();
    if(code.isEmpty() || code != lastLoadedSource) {
        return;
    }

    if(m_thymio->vmExecutionState() != mobsya::ThymioNode::VMExecutionState::Stopped)
        m_breakpoints_watcher->setRequest(m_thymio->setBreakPoints(m_breakpoints));

    rehighlight();
}

QVector<unsigned> NodeTab::breakpoints() const {
    auto bps = editor->breakpoints();
    std::transform(bps.begin(), bps.end(), bps.begin(), [](auto bp) { return bp + 1; });
    return bps;
}

void NodeTab::breakpointsChanged() {
    QVector<unsigned> set;
    if(m_breakpoints_watcher->success())
        set = m_breakpoints_watcher->getResult().breakpoints();
    for(auto bp : m_breakpoints) {
        clearEditorProperty(QStringLiteral("breakpointPending"), bp - 1);
        if(set.contains(bp))
            setEditorProperty(QStringLiteral("breakpoint"), QVariant(), bp - 1);
    }
    m_breakpoints = set;
    std::sort(m_breakpoints.begin(), m_breakpoints.end());
    rehighlight();
}

void NodeTab::onExecutionPosChanged(unsigned line) {
    currentPC = line;

    if(line == 0)
        return;

    line = line - 1;

    // go to this line and next line is visible
    editor->setTextCursor(QTextCursor(editor->document()->findBlockByLineNumber(currentPC + 1)));
    editor->ensureCursorVisible();
    editor->setTextCursor(QTextCursor(editor->document()->findBlockByLineNumber(currentPC)));


    if(setEditorProperty(QStringLiteral("active"), QVariant(), line, true))
        rehighlight();
}

void NodeTab::onExecutionStateChanged() {
    editor->debugging = currentPC > 0;
    if(m_thymio->vmExecutionState() == mobsya::ThymioNode::VMExecutionState::Stopped) {
        currentPC = -1;
        if(clearEditorProperty(QStringLiteral("active")))
            rehighlight();
    }
    if(m_thymio->vmExecutionState() != mobsya::ThymioNode::VMExecutionState::Paused) {

        if(clearEditorProperty(QStringLiteral("active")))
            rehighlight();
    }
}


void NodeTab::updateAsebaVMDescription() {
    if(m_thymio)
        m_aseba_vm_description_watcher->setRequest(m_thymio->fetchAsebaVMDescription());
}

void NodeTab::onAsebaVMDescriptionChanged() {
    if(!m_aseba_vm_description_watcher->success())
        return;
    auto desc = m_aseba_vm_description_watcher->getResult();
    vmFunctionsModel.recreateTreeFromDescription(desc.functions());
    vmLocalEvents->clear();
    for(const auto& e : desc.events()) {
        auto item = new QListWidgetItem(e.name());
        item->setToolTip(e.description());
        vmLocalEvents->addItem(item);
    }
}


void NodeTab::onVariablesChanged(const mobsya::ThymioNode::VariableMap& vars) {
    bool recompile = false;
    for(auto it = vars.begin(); it != vars.end(); ++it) {
        if(it->isConstant()) {
            it->value().isNull() ? m_constants_model.removeVariable(it.key()) :
                                   m_constants_model.addVariable(it.key(), it.value().value());
            recompile = true;
        } else {
            it->value().isNull() ? m_vm_variables_model.removeVariable(it.key()) :
                                   m_vm_variables_model.setVariable(it.key(), it.value());
        }
    }

    if(recompile) {
        this->compileCodeOnTarget();
    }
}

void NodeTab::setVariable(const QString& k, const mobsya::ThymioVariable& value) {
    if(!m_thymio)
        return;

    m_thymio->setWatchVariablesEnabled(true);
    synchronizeVariablesToogle->setChecked(true);

    mobsya::ThymioNode::VariableMap map;
    map.insert(k, value);
    m_thymio->setVariabes(map);
}

void NodeTab::onGlobalEventsTableChanged(const QVector<mobsya::EventDescription>& events) {
    m_events_model.clear();
    for(const auto& e : events) {
        m_events_model.addVariable(e.name(), e.size());
    }
    this->compileCodeOnTarget();
}


void NodeTab::addEvent(const QString& event, int size) {
    if(!m_thymio)
        return;
    m_thymio->addEvent(mobsya::EventDescription(event, size));
}

void NodeTab::removeEvent(const QString& name) {
    if(!m_thymio)
        return;
    m_thymio->removeEvent(name);
}

void NodeTab::emitEvent(const QString& name, const QVariant& value) {
    if(!m_thymio)
        return;
    m_thymio->emitEvent(name, value);
}

void NodeTab::onEvents(const mobsya::ThymioNode::VariableMap& events) {
    m_eventsWidget->onEvents(events);
}


static void write16(QIODevice& dev, const uint16_t v) {
    dev.write((const char*)&v, 2);
}

static void write16(QIODevice& dev, const VariablesDataVector& data, const char* varName) {
    if(data.empty()) {
        std::cerr << "Warning, cannot find " << varName << " required to save bytecode, using 0" << std::endl;
        write16(dev, 0);
    } else
        dev.write((const char*)&data[0], 2);
}

static uint16_t crcXModem(const uint16_t oldCrc, const QString& s) {
    return crcXModem(oldCrc, s.toStdWString());
}

void NodeTab::saveBytecode() const {
    /* const QString& nodeName(target->getName(id));
     QString bytecodeFileName =
         QFileDialog::getSaveFileName(mainWindow, tr("Save the binary code of %0").arg(nodeName),
                                      QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation),
                                      "Aseba Binary Object (*.abo);;All Files (*)");

     QFile file(bytecodeFileName);
     if(!file.open(QFile::WriteOnly | QFile::Truncate))
         return;

     // See AS001 at https://aseba.wikidot.com/asebaspecifications

     // header
     const char* magic = "ABO";
     file.write(magic, 4);
     write16(file, 0);  // binary format version
     write16(file, target->getDescription(id)->protocolVersion);
     write16(file, vmMemoryModel->getVariableValue("_productId"), "product identifier (_productId)");
     write16(file, vmMemoryModel->getVariableValue("_fwversion"), "firmware version (_fwversion)");
     write16(file, id);
     write16(file, crcXModem(0, nodeName));
     write16(file, target->getDescription(id)->crc());

     // bytecode
     write16(file, bytecode.size());
     uint16_t crc(0);
     for(size_t i = 0; i < bytecode.size(); ++i) {
         const uint16_t bc(bytecode[i]);
         write16(file, bc);
         crc = crcXModem(crc, bc);
     }
     write16(file, crc);
     */
}

void NodeTab::editorContentChanged() {
    // only recompile if source code has actually changed
    if(editor->toPlainText() == lastCompiledSource)
        return;
    lastCompiledSource = editor->toPlainText();
    // recompile
    compileCodeOnTarget();
    // mainWindow->sourceChanged();
}

void NodeTab::showMemoryUsage(bool show) {
    memoryUsageText->setVisible(show);
}

void NodeTab::cursorMoved() {
    // fix tab
    cursorPosText->setText(QStringLiteral("Line: %0 Col: %1")
                               .arg(editor->textCursor().blockNumber() + 1)
                               .arg(editor->textCursor().columnNumber() + 1));
}

void NodeTab::goToError() {
    if(errorPos >= 0) {
        QTextCursor cursor = editor->textCursor();
        cursor.setPosition(errorPos);
        editor->setTextCursor(cursor);
        editor->ensureCursorVisible();
    }
}

void NodeTab::clearExecutionErrors() {
    // remove execution error
    if(clearEditorProperty(QStringLiteral("executionError")))
        rehighlight();
}

void NodeTab::refreshCompleterModel(LocalContext context) {

    //		qDebug() << "New context: " << context;
    // disconnect(mainWindow->eventsDescriptionsModel, nullptr, sortingProxy, nullptr);

    switch(context) {
        case GeneralContext:  // both variables and constants
        case UnknownContext: sortingProxy->setSourceModel(variableAggregator); break;
        case LeftValueContext:  // only variables
            // sortingProxy->setSourceModel(vmMemoryModel);
            break;
        case FunctionContext:  // native functions
            sortingProxy->setSourceModel(functionsFlatModel);
            break;
        case SubroutineCallContext:  // subroutines
            // sortingProxy->setSourceModel(vmSubroutinesModel);
            break;
        case EventContext:  // events
            sortingProxy->setSourceModel(eventAggregator);
            break;
        case VarDefContext:
        default:  // disable auto-completion in this case
            editor->setCompleterModel(nullptr);
            return;
    }
    sortingProxy->sort(0);
    editor->setCompleterModel(sortingProxy);
}

/*void NodeTab::sortCompleterModel() {
    sortingProxy->sort(0);
    editor->setCompleterModel(0);
    editor->setCompleterModel(sortingProxy);
}*/

// void NodeTab::variablesMemoryChanged(unsigned start, const VariablesDataVector& variables) {
//    // update memory view
//    vmMemoryModel->setVariablesData(start, variables);
//}


void NodeTab::rehighlight() {
    // rehighlighting = true;
    highlighter->rehighlight();
}


bool NodeTab::setEditorProperty(const QString& property, const QVariant& value, unsigned line, bool removeOld) {
    bool changed = false;

    QTextBlock block = editor->document()->begin();
    unsigned lineCounter = 0;
    while(block != editor->document()->end()) {
        auto* uData = polymorphic_downcast_or_null<AeslEditorUserData*>(block.userData());
        if(lineCounter == line) {
            // set propety
            if(uData) {
                if(!uData->properties.contains(property) || (uData->properties[property] != value)) {
                    uData->properties[property] = value;
                    changed = true;
                }
            } else {
                block.setUserData(new AeslEditorUserData(property, value));
                changed = true;
            }
        } else {
            // remove if exists
            if(uData && removeOld && uData->properties.contains(property)) {
                uData->properties.remove(property);
                if(uData->properties.isEmpty()) {
                    // garbage collect UserData
                    block.setUserData(nullptr);
                }
                changed = true;
            }
        }

        block = block.next();
        lineCounter++;
    }

    return changed;
}

bool NodeTab::clearEditorProperty(const QString& property, unsigned line) {
    bool changed = false;

    // find line, remove property
    QTextBlock block = editor->document()->begin();
    unsigned lineCounter = 0;
    while(block != editor->document()->end()) {
        if(lineCounter == line) {
            auto* uData = polymorphic_downcast_or_null<AeslEditorUserData*>(block.userData());
            if(uData && uData->properties.contains(property)) {
                uData->properties.remove(property);
                if(uData->properties.isEmpty()) {
                    // garbage collect UserData
                    block.setUserData(nullptr);
                }
                changed = true;
            }
        }
        block = block.next();
        lineCounter++;
    }

    return changed;
}

bool NodeTab::clearEditorProperty(const QString& property) {
    bool changed = false;

    // go through all blocks, remove property if found
    QTextBlock block = editor->document()->begin();
    while(block != editor->document()->end()) {
        auto* uData = polymorphic_downcast_or_null<AeslEditorUserData*>(block.userData());
        if(uData && uData->properties.contains(property)) {
            uData->properties.remove(property);
            if(uData->properties.isEmpty()) {
                // garbage collect UserData
                block.setUserData(nullptr);
            }
            changed = true;
        }
        block = block.next();
    }

    return changed;
}

void NodeTab::switchEditorProperty(const QString& oldProperty, const QString& newProperty) {
    QTextBlock block = editor->document()->begin();
    while(block != editor->document()->end()) {
        auto* uData = polymorphic_downcast_or_null<AeslEditorUserData*>(block.userData());
        if(uData && uData->properties.contains(oldProperty)) {
            uData->properties.remove(oldProperty);
            uData->properties[newProperty] = QVariant();
        }
        block = block.next();
    }
}


void NodeTab::setupWidgets() {
    createEditor();

    // editor related notification widgets
    cursorPosText = new QLabel;
    compilationResultImage = new ClickableLabel;
    compilationResultText = new ClickableLabel;
    compilationResultText->setWordWrap(true);
    compilationResultText->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    auto* compilationResultLayout = new QHBoxLayout;
    compilationResultLayout->addWidget(cursorPosText);
    compilationResultLayout->addWidget(compilationResultText, 1000);
    compilationResultLayout->addWidget(compilationResultImage);

    // memory usage notification area
    memoryUsageText = new QLabel();
    memoryUsageText->setAlignment(Qt::AlignLeft);
    memoryUsageText->setWordWrap(true);

    // editor area
    auto* editorAreaLayout = new QHBoxLayout;
    editorAreaLayout->setSpacing(0);
    editorAreaLayout->addWidget(m_breakpointsSidebar);
    editorAreaLayout->addWidget(linenumbers);
    editorAreaLayout->addWidget(editor);

    auto topLayout = new QHBoxLayout;
    auto editorLayout = new QVBoxLayout;

    // panel

    // buttons
    executionModeLabel = new QLabel(tr("unknown"));
    stopButton = new QPushButton(QIcon(":/images/stop.png"), tr("Stop"));
    // resetButton->setEnabled(false);

    runButton = new QPushButton(QIcon(":/images/play.png"), tr("Run"));

    pauseButton = new QPushButton(QIcon(":/images/pause.png"), tr("Pause"));

    nextButton = new QPushButton(QIcon(":/images/step.png"), tr("Next"));
    nextButton->setEnabled(false);

    synchronizeVariablesToogle = new QCheckBox(tr("Synchronize"));

    topLayout->addStretch();
    topLayout->addWidget(stopButton);
    topLayout->addWidget(runButton);
    topLayout->addWidget(pauseButton);
    topLayout->addWidget(nextButton);


    editorLayout->addLayout(topLayout);
    editorLayout->addLayout(editorAreaLayout);
    editorLayout->addLayout(compilationResultLayout);
    editorLayout->addWidget(memoryUsageText);

    auto* buttonsLayout = new QGridLayout;
    buttonsLayout->addWidget(new QLabel(tr("<b>Execution</b>")), 0, 0);
    buttonsLayout->addWidget(executionModeLabel, 0, 1);


    // memory
    m_vm_variables_view = new QTreeView;
    m_vm_variables_view->setModel(&m_vm_variables_filter_model);
    m_vm_variables_view->setItemDelegate(new SpinBoxDelegate(-32768, 32767, this));
    m_vm_variables_view->setSelectionMode(QAbstractItemView::SingleSelection);
    m_vm_variables_view->setSelectionBehavior(QAbstractItemView::SelectItems);
    m_vm_variables_view->setEditTriggers(QAbstractItemView::SelectedClicked | QAbstractItemView::DoubleClicked);
    m_vm_variables_view->setDragDropMode(QAbstractItemView::DragOnly);
    m_vm_variables_view->setDragEnabled(true);
    m_vm_variables_view->setHeaderHidden(true);
    m_vm_variables_view->header()->setSectionResizeMode(QHeaderView::ResizeToContents);

    auto* localVariablesAndEventLayout = new QVBoxLayout;
    auto* memorySubLayout = new QHBoxLayout;
    memorySubLayout->addWidget(new QLabel(tr("<b>Variables</b>")));
    memorySubLayout->addStretch();
    memorySubLayout->addWidget(synchronizeVariablesToogle);
    localVariablesAndEventLayout->addLayout(memorySubLayout);
    localVariablesAndEventLayout->addWidget(m_vm_variables_view);
    memorySubLayout = new QHBoxLayout;
    QLabel* filterLabel(new QLabel(tr("F&ilter:")));
    memorySubLayout->addWidget(filterLabel);
    m_vm_variables_filter_input = new QLineEdit;
    filterLabel->setBuddy(m_vm_variables_filter_input);
    memorySubLayout->addWidget(m_vm_variables_filter_input);
    localVariablesAndEventLayout->addLayout(memorySubLayout);
    connect(m_vm_variables_filter_input, &QLineEdit::textChanged, &m_vm_variables_filter_model,
            [this](const QString& text) { m_vm_variables_filter_model.setFilterWildcard("*" + text + "*"); });

    // functions
    vmFunctionsView = new QTreeView;
    vmFunctionsView->setMinimumHeight(40);
    vmFunctionsView->setMinimumSize(QSize(50, 40));
    vmFunctionsView->setModel(&vmFunctionsModel);
    vmFunctionsView->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    vmFunctionsView->setSelectionMode(QAbstractItemView::SingleSelection);
    vmFunctionsView->setSelectionBehavior(QAbstractItemView::SelectItems);
    vmFunctionsView->setDragDropMode(QAbstractItemView::DragOnly);
    vmFunctionsView->setDragEnabled(true);
    vmFunctionsView->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding);
    vmFunctionsView->setHeaderHidden(true);
    // local events
    vmLocalEvents = new DraggableListWidget;
    vmLocalEvents->setMinimumHeight(40);
    vmLocalEvents->setSelectionMode(QAbstractItemView::SingleSelection);
    vmLocalEvents->setDragDropMode(QAbstractItemView::DragOnly);
    vmLocalEvents->setDragEnabled(true);
    // toolbox
    toolBox = new QToolBox;
    toolBox->addItem(vmFunctionsView, tr("Native Functions"));
    toolBox->addItem(vmLocalEvents, tr("Local Events"));

    localVariablesAndEventLayout->addWidget(toolBox);

    // panel
    auto* panelSplitter = new QSplitter(Qt::Vertical);

    QWidget* buttonsWidget = new QWidget;
    buttonsWidget->setLayout(buttonsLayout);
    panelSplitter->addWidget(buttonsWidget);
    panelSplitter->setCollapsible(0, false);

    QWidget* localVariablesAndEvent = new QWidget;
    localVariablesAndEvent->setLayout(localVariablesAndEventLayout);
    panelSplitter->addWidget(localVariablesAndEvent);
    panelSplitter->setStretchFactor(1, 9);
    panelSplitter->setStretchFactor(2, 4);

    addWidget(panelSplitter);
    QWidget* editorWidget = new QWidget;
    editorWidget->setLayout(editorLayout);
    addWidget(editorWidget);
    setSizes(QList<int>() << 270 << 500);


    QVBoxLayout* rightPaneLayout = new QVBoxLayout;

    m_constantsWidget = new ConstantsWidget();
    m_eventsWidget = new EventsWidget;
    rightPaneLayout->addWidget(m_constantsWidget);
    rightPaneLayout->addWidget(m_eventsWidget);

    QWidget* rightPane = new QWidget();
    rightPane->setLayout(rightPaneLayout);
    addWidget(rightPane);
}

void NodeTab::setupConnections() {
    // execution
    connect(stopButton, &QAbstractButton::clicked, this, &NodeTab::reset);
    connect(runButton, &QAbstractButton::clicked, this, &NodeTab::run);
    connect(pauseButton, &QAbstractButton::clicked, this, &NodeTab::pause);
    connect(nextButton, &QAbstractButton::clicked, this, &NodeTab::step);
    connect(synchronizeVariablesToogle, &QCheckBox::toggled, this, &NodeTab::synchronizeVariablesChecked);


    connect(this, &NodeTab::executionStateChanged, [this] {
        runButton->setEnabled(true);
        runButton->setVisible(true);
        pauseButton->setEnabled(true);
        pauseButton->setVisible(true);
        nextButton->setEnabled(true);
        nextButton->setVisible(true);
        stopButton->setVisible(true);
        stopButton->setEnabled(true);

        auto state = m_thymio->vmExecutionState();
        switch(state) {
            case mobsya::ThymioNode::VMExecutionState::Running: {
                runButton->setEnabled(false);
                nextButton->setEnabled(false);
                break;
            };
            case mobsya::ThymioNode::VMExecutionState::Paused: {
                pauseButton->setEnabled(false);
                break;
            };
            case mobsya::ThymioNode::VMExecutionState::Stopped: {
                pauseButton->hide();
                nextButton->hide();
                stopButton->setEnabled(false);
                break;
            };
            default: break;
        }
    });

    connect(this, &NodeTab::compilationFailed, [this] { runButton->setEnabled(false); });
    connect(this, &NodeTab::compilationSucceed, [this] {
        runButton->setEnabled(m_thymio->vmExecutionState() != mobsya::ThymioNode::VMExecutionState::Running);
    });


    connect(m_constantsWidget, &ConstantsWidget::constantModified, [this](const QString& name, const QVariant& value) {
        this->setVariable(name, {value, true});
    });
    connect(m_eventsWidget, &EventsWidget::eventAdded, this, &NodeTab::addEvent);
    connect(m_eventsWidget, &EventsWidget::eventRemoved, this, &NodeTab::removeEvent);
    connect(m_eventsWidget, &EventsWidget::eventEmitted, this, &NodeTab::emitEvent);

    // memory
    // connect(vmMemoryModel, SIGNAL(variableValuesChanged(unsigned, const VariablesDataVector&)),
    //        SLOT(setVariableValues(unsigned, const VariablesDataVector&)));

    // editor
    connect(editor, &QTextEdit::textChanged, this, &NodeTab::editorContentChanged);
    connect(editor, &QTextEdit::cursorPositionChanged, this, &NodeTab::cursorMoved);
    connect(editor, &AeslEditor::breakpointSet, this, &NodeTab::setBreakpoint);
    connect(editor, &AeslEditor::breakpointCleared, this, &NodeTab::clearBreakpoint);
    connect(editor, &AeslEditor::breakpointClearedAll, this, &NodeTab::breakpointClearedAll);
    connect(editor, &AeslEditor::refreshModelRequest, this, &NodeTab::refreshCompleterModel);

    connect(compilationResultImage, SIGNAL(clicked()), SLOT(goToError()));
    connect(compilationResultText, SIGNAL(clicked()), SLOT(goToError()));

    synchronizeVariablesToogle->setChecked(true);
}

void NodeTab::synchronizeVariablesChecked(bool checked) {
    if(m_thymio) {
        m_thymio->setWatchVariablesEnabled(checked);
    }
}

void NodeTab::showHidden(bool show) {
    m_vm_variables_filter_model.showHidden(show);
}


}  // namespace Aseba