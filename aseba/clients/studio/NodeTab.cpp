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
    breakpoints = new AeslBreakpointSidebar(editor);
    linenumbers = new AeslLineNumberSidebar(editor);
    highlighter = new AeslHighlighter(editor, editor->document());
}

NodeTab::NodeTab(QWidget* parent)
    : QSplitter(parent)
    , ScriptTab()
    , currentPC(0)
    , previousMode(Target::EXECUTION_UNKNOWN)
    , compilationDirty(false)
    , isSynchronized(true) {
    // setup some variables
    // rehighlighting = false;
    errorPos = -1;
    allocatedVariablesCount = 0;

    /*  // create models
      vmFunctionsModel = new TargetFunctionsModel(target->getDescription(id), showHidden, this);
      vmMemoryModel = new TargetVariablesModel(this);
      variablesModel = vmMemoryModel;
      subscribeToVariableOfInterest(ASEBA_PID_VAR_NAME);
      */

    // create gui
    setupWidgets();
    setupConnections();

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
    // functionsFlatModel = new TreeChainsawFilter(this);
    // functionsFlatModel->setSourceModel(vmFunctionsModel);

    // create the model for subroutines
    // vmSubroutinesModel = new TargetSubroutinesModel(this);

    editor->setFocus();
    setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
}

NodeTab::~NodeTab() {
    // wait until thread has finished
}

void NodeTab::setThymio(std::shared_ptr<mobsya::ThymioNode> node) {
    m_thymio = node;
    if(node) {
        if(node->status() == mobsya::ThymioNode::Status::Available)
            node->lock();
        auto ptr = node.get();
        connect(ptr, &mobsya::ThymioNode::statusChanged, [node]() {
            if(node->status() == mobsya::ThymioNode::Status::Available)
                node->lock();
        });
    }
}

void NodeTab::resetClicked() {
    clearExecutionErrors();
}

void NodeTab::loadClicked() {
    /*if(errorPos == -1) {
        clearEditorProperty("executionError");
        target->uploadBytecode(id, bytecode);
        // target->getVariables(id, 0, allocatedVariablesCount);
        editor->debugging = true;
        reSetBreakpoints();
        rehighlight();
    }

    isSynchronized = true;
    mainWindow->resetStatusText();
    */
}

void NodeTab::runInterruptClicked() {
    /*    if(!runInterruptButton->property("isRunning").toBool())
            target->run(id);
        else
            target->pause(id);
    */
}

void NodeTab::nextClicked() {
    //   target->next(id);
}

void NodeTab::refreshMemoryClicked() {
    // as we explicitely clicked, refresh all variables
    //    target->getVariables(id, 0, allocatedVariablesCount);
}

/*void NodeTab::autoRefreshMemoryClicked(int state) {
    if(state == Qt::Checked) {
        refreshMemoryButton->setEnabled(false);
        refreshTimer = startTimer(200);  // 5 Hz
    } else {
        refreshMemoryButton->setEnabled(true);
        killTimer(refreshTimer);
        refreshMemoryClicked();
    }
}*/

void NodeTab::writeBytecode() {
    if(errorPos == -1) {
        loadClicked();
        // target->writeBytecode(id);
    }
}

void NodeTab::reboot() {
    markTargetUnsynced();
    // target->reboot(id);
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

void NodeTab::setVariableValues(unsigned index, const VariablesDataVector& values) {
    // target->setVariables(id, index, values);
}

void NodeTab::insertVariableName(const QModelIndex& index) {
    // only top level names have to be inserted
    if(!index.parent().isValid() && (index.column() == 0))
        editor->insertPlainText(index.data().toString());
}

void NodeTab::editorContentChanged() {
    // only recompile if source code has actually changed
    if(editor->toPlainText() == lastCompiledSource)
        return;
    lastCompiledSource = editor->toPlainText();

    // handle completion
    QTextCursor cursor(editor->textCursor());
    if(ConfigDialog::getAutoCompletion() && cursor.atBlockEnd()) {
        // language completion
        const QString& line(cursor.block().text());
        QString keyword(line);

        // make sure the string does not have any trailing space
        int nonWhitespace(0);
        while((nonWhitespace < keyword.size()) &&
              ((keyword.at(nonWhitespace) == ' ') || (keyword.at(nonWhitespace) == '\t')))
            ++nonWhitespace;
        keyword.remove(0, nonWhitespace);

        if(!keyword.trimmed().isEmpty()) {
            QString prefix;
            QString postfix;
            if(keyword == "if") {
                const QString headSpace = line.left(line.indexOf("if"));
                prefix = " ";
                postfix = " then\n" + headSpace + "\t\n" + headSpace + "end";
            } else if(keyword == "when") {
                const QString headSpace = line.left(line.indexOf("when"));
                prefix = " ";
                postfix = " do\n" + headSpace + "\t\n" + headSpace + "end";
            } else if(keyword == "for") {
                const QString headSpace = line.left(line.indexOf("for"));
                prefix = " ";
                postfix = "i in 0:0 do\n" + headSpace + "\t\n" + headSpace + "end";
            } else if(keyword == "while") {
                const QString headSpace = line.left(line.indexOf("while"));
                prefix = " ";
                postfix = " do\n" + headSpace + "\t\n" + headSpace + "end";
            } else if((keyword == "else") && cursor.block().next().isValid()) {
                const QString tab = QString("\t");
                QString headSpace = line.left(line.indexOf("else"));

                if(headSpace.size() >= tab.size()) {
                    headSpace = headSpace.left(headSpace.size() - tab.size());
                    if(cursor.block().next().text() == headSpace + "end") {
                        prefix = "\n" + headSpace + "else";
                        postfix = "\n" + headSpace + "\t";

                        cursor.select(QTextCursor::BlockUnderCursor);
                        cursor.removeSelectedText();
                    }
                }
            } else if(keyword == "elseif") {
                const QString headSpace = line.left(line.indexOf("elseif"));
                prefix = " ";
                postfix = " then";
            }

            if(!prefix.isNull() || !postfix.isNull()) {
                cursor.beginEditBlock();
                cursor.insertText(prefix);
                const int pos = cursor.position();
                cursor.insertText(postfix);
                cursor.setPosition(pos);
                cursor.endEditBlock();
                editor->setTextCursor(cursor);
            }
        }
    }
    // recompile
    recompile();
    // mainWindow->sourceChanged();
}

void NodeTab::keywordClicked(QString keyword) {
    QTextCursor cursor(editor->textCursor());

    cursor.beginEditBlock();
    cursor.insertText(keyword);
    cursor.endEditBlock();
    editorContentChanged();
}

void NodeTab::showKeywords(bool show) {
    if(show)
        keywordsToolbar->show();
    else
        keywordsToolbar->hide();
}

void NodeTab::showMemoryUsage(bool show) {
    memoryUsageText->setVisible(show);
}

void NodeTab::updateHidden() {
    /*    const QString& filterString(vmMemoryFilter->text());
        const QRegExp filterRegexp(filterString);
        // Quick hack to hide hidden variable in the treeview and not in vmMemoryModel
        // FIXME use a model proxy to perform this task
        for(int i = 0; i < vmMemoryModel->rowCount(QModelIndex()); i++) {
            const QString name(vmMemoryModel->data(vmMemoryModel->index(i, 0), Qt::DisplayRole).toString());
            bool hidden(false);
            if((!showHidden && ((name.left(1) == "_") || name.contains(QString("._")))) ||
               (!filterString.isEmpty() && name.indexOf(filterRegexp) == -1))
                hidden = true;
            vmMemoryView->setRowHidden(i, QModelIndex(), hidden);
        }
    */
}

// NodeTab::CompilationResult* compilationThread(const TargetDescription targetDescription,
//                                              const CommonDefinitions commonDefinitions, QString source, bool dump) {
/*auto* result(new NodeTab::CompilationResult(dump));

Compiler compiler;
compiler.setTargetDescription(&targetDescription);
compiler.setCommonDefinitions(&commonDefinitions);
compiler.setTranslateCallback(CompilerTranslator::translate);

std::wistringstream is(source.toStdWString());

if(dump)
    result->success = compiler.compile(is, result->bytecode, result->allocatedVariablesCount, result->error,
                                       &result->compilationMessages);
else
    result->success = compiler.compile(is, result->bytecode, result->allocatedVariablesCount, result->error);

if(result->success) {
    result->variablesMap = *compiler.getVariablesMap();
    result->subroutineTable = *compiler.getSubroutineTable();
}

return result;
*/
//}

void NodeTab::recompile() {
    /*    // compile
        if(compilationFuture.isRunning())
            compilationDirty = true;
        else {
            bool dump(mainWindow->nodes->currentWidget() == this);
            compilationFuture = QtConcurrent::run(compilationThread, *target->getDescription(id), *commonDefinitions,
                                                  editor->toPlainText(), dump);
            compilationWatcher.setFuture(compilationFuture);
            compilationDirty = false;

            // show progress icon
            compilationResultImage->setPixmap(QPixmap(QString(":/images/busy.png")));
        }
    */
}

void NodeTab::compilationCompleted() {
    /*    CompilationResult* result(compilationFuture.result());
        assert(result);

        // as long as result is dirty, continue compilation
        if(compilationDirty) {
            delete result;
            recompile();
            return;
        }

        // process results
        processCompilationResult(result);
    */
}

/*void NodeTab::processCompilationResult(CompilationResult* result) {
    // clear old user data
    // doRehighlight is required to prevent infinite recursion because there are no slot
    // to differentiate user changes from highlight changes in documents
    bool doRehighlight = clearEditorProperty("errorPos");

    if(result->dump) {
        mainWindow->compilationMessageBox->setWindowTitle(
            tr("Aseba Studio: Output of last compilation for %0").arg(target->getName(id)));

        if(result->success)
            mainWindow->compilationMessageBox->setText(tr("Compilation success.") + QString("\n\n") +
                                                       QString::fromStdWString(result->compilationMessages.str()));
        //      else
        //          mainWindow->compilationMessageBox->setText(QString::fromStdWString(result->error.toWString()) +
        //          ".\n\n" +
        // QString::fromStdWString(result->compilationMessages.str()));
    }

    // show memory usage
    if(result->success) {
        const unsigned variableCount = result->allocatedVariablesCount;
        const unsigned variableTotal = (*target->getDescription(id)).variablesSize;
        const unsigned bytecodeCount = result->bytecode.size();
        const unsigned bytecodeTotal = (*target->getDescription(id)).bytecodeSize;
        assert(variableCount);
        assert(bytecodeCount);
        const QString variableText = tr("variables: %1 on %2 (%3\\%)")
                                         .arg(variableCount)
                                         .arg(variableTotal)
                                         .arg((double)variableCount * 100. / variableTotal, 0, 'f', 1);
        const QString bytecodeText = tr("bytecode: %1 on %2 (%3\\%)")
                                         .arg(bytecodeCount)
                                         .arg(bytecodeTotal)
                                         .arg((double)bytecodeCount * 100. / bytecodeTotal, 0, 'f', 1);
        memoryUsageText->setText(trUtf8("<b>Memory usage</b> : %1, %2").arg(variableText).arg(bytecodeText));
    }

    // update state following result
    if(result->success) {
        bytecode = result->bytecode;
        allocatedVariablesCount = result->allocatedVariablesCount;
        vmMemoryModel->updateVariablesStructure(&result->variablesMap);
        // This is disabled because we do not want to override the user's choice.
        // The best would be to use this until the user has done any change, and
        // then stop using it. But as sizes are reloaded from settings, the
        // gain is not worth the implementation work.
        // vmMemoryView->resizeColumnToContents(0);
        vmSubroutinesModel->updateSubroutineTable(result->subroutineTable);

        updateHidden();
        compilationResultText->setText(tr("Compilation success."));
        compilationResultImage->setPixmap(QPixmap(QString(":/images/ok.png")));
        loadButton->setEnabled(true);
        emit uploadReadynessChanged(true);

        errorPos = -1;
    } else {
        //      compilationResultText->setText(QString::fromStdWString(result->error.toWString()));
        compilationResultImage->setPixmap(QPixmap(QString(":/images/warning.png")));
        loadButton->setEnabled(false);
        emit uploadReadynessChanged(false);

        // we have an error, set the correct user data
        if(result->error.pos.valid) {
            errorPos = result->error.pos.character;
            QTextBlock textBlock = editor->document()->findBlock(errorPos);
            int posInBlock = errorPos - textBlock.position();
            if(textBlock.userData())
                polymorphic_downcast<AeslEditorUserData*>(textBlock.userData())->properties["errorPos"] = posInBlock;
            else
                textBlock.setUserData(new AeslEditorUserData("errorPos", posInBlock));
            doRehighlight = true;
        }
    }

    // we have finished with the results
    delete result;

    // clear bearkpoints of target if currently in debugging mode
    if(editor->debugging) {
        // target->stop(id);
        markTargetUnsynced();
        doRehighlight = true;
    }

    if(doRehighlight)
        rehighlight();
*/
//}

//! When code is changed or target is rebooted, remove breakpoints from target but keep them locally
//! as pending for next code load
void NodeTab::markTargetUnsynced() {
    editor->debugging = false;
    resetButton->setEnabled(false);
    runInterruptButton->setEnabled(false);
    nextButton->setEnabled(false);
    // target->clearBreakpoints(id);
    switchEditorProperty("breakpoint", "breakpointPending");
    executionModeLabel->setText(tr("unknown"));
    // mainWindow->nodes->setExecutionMode(mainWindow->getIndexFromId(id), Target::EXECUTION_UNKNOWN);
}

void NodeTab::cursorMoved() {
    // fix tab
    cursorPosText->setText(QString("Line: %0 Col: %1")
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
    if(clearEditorProperty("executionError"))
        rehighlight();
}

void NodeTab::refreshCompleterModel(LocalContext context) {
    /*
        //		qDebug() << "New context: " << context;
        // disconnect(mainWindow->eventsDescriptionsModel, nullptr, sortingProxy, nullptr);

        switch(context) {
            case GeneralContext:  // both variables and constants
            case UnknownContext: sortingProxy->setSourceModel(variableAggregator); break;
            case LeftValueContext:  // only variables
                sortingProxy->setSourceModel(vmMemoryModel);
                break;
            case FunctionContext:  // native functions
                sortingProxy->setSourceModel(functionsFlatModel);
                break;
            case SubroutineCallContext:  // subroutines
                sortingProxy->setSourceModel(vmSubroutinesModel);
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
       */
}
/*
    void NodeTab::sortCompleterModel()
    {
        sortingProxy->sort(0);
        editor->setCompleterModel(0);
        editor->setCompleterModel(sortingProxy);
    }
*/
// void NodeTab::variablesMemoryChanged(unsigned start, const VariablesDataVector& variables) {
//    // update memory view
//    vmMemoryModel->setVariablesData(start, variables);
//}

void NodeTab::setBreakpoint(unsigned line) {
    rehighlight();
    // target->setBreakpoint(id, line);
}

void NodeTab::clearBreakpoint(unsigned line) {
    rehighlight();
    // target->clearBreakpoint(id, line);
}

void NodeTab::breakpointClearedAll() {
    rehighlight();
    //    target->clearBreakpoints(id);
}

void NodeTab::executionPosChanged(unsigned line) {
    // change active state
    currentPC = line;
    if(setEditorProperty("active", QVariant(), line, true))
        rehighlight();
}

void NodeTab::executionModeChanged(Target::ExecutionMode mode) {
    // ignore those messages if we are not in debugging mode
    if(!editor->debugging)
        return;

    resetButton->setEnabled(true);
    runInterruptButton->setEnabled(true);
    compilationResultImage->setPixmap(QPixmap(QString(":/images/ok.png")));

    /*
    // Filter spurious messages, to detect a stop at a breakpoint
    if(previousMode != mode) {
        previousMode = mode;
        if((mode == Target::EXECUTION_STEP_BY_STEP) && editor->isBreakpoint(currentPC)) {
            // we are at a breakpoint
            if(mainWindow->currentScriptTab != this) {
                // not the current tab -> hidden tab -> highlight me in red
                mainWindow->nodes->highlightTab(mainWindow->getIndexFromId(id), Qt::red);
            }
            // go to this line
            editor->setTextCursor(QTextCursor(editor->document()->findBlockByLineNumber(currentPC)));
            editor->ensureCursorVisible();
        }
    }*/

    if(mode == Target::EXECUTION_RUN) {
        executionModeLabel->setText(tr("running"));

        runInterruptButton->setText(tr("Pause"));
        runInterruptButton->setIcon(QIcon(":/images/pause.png"));

        runInterruptButton->setProperty("isRunning", true);

        nextButton->setEnabled(false);

        if(clearEditorProperty("active"))
            rehighlight();
    } else if(mode == Target::EXECUTION_STEP_BY_STEP) {
        executionModeLabel->setText(tr("step by step"));

        runInterruptButton->setText(tr("Run"));
        runInterruptButton->setIcon(QIcon(":/images/play.png"));
        runInterruptButton->setProperty("isRunning", false);

        nextButton->setEnabled(true);

        // go to this line and next line is visible
        editor->setTextCursor(QTextCursor(editor->document()->findBlockByLineNumber(currentPC + 1)));
        editor->ensureCursorVisible();
        editor->setTextCursor(QTextCursor(editor->document()->findBlockByLineNumber(currentPC)));
    } else if(mode == Target::EXECUTION_STOP) {
        executionModeLabel->setText(tr("stopped"));

        runInterruptButton->setText(tr("Run"));
        runInterruptButton->setIcon(QIcon(":/images/play.png"));
        runInterruptButton->setProperty("isRunning", false);

        nextButton->setEnabled(false);

        if(clearEditorProperty("active"))
            rehighlight();
    }

    // set the tab icon to show the current execution mode
    // mainWindow->nodes->setExecutionMode(mainWindow->getIndexFromId(id), mode);
}

void NodeTab::breakpointSetResult(unsigned line, bool success) {
    clearEditorProperty("breakpointPending", line);
    if(success)
        setEditorProperty("breakpoint", QVariant(), line);
    rehighlight();
}

void NodeTab::rehighlight() {
    // rehighlighting = true;
    highlighter->rehighlight();
}

void NodeTab::reSetBreakpoints() {
    // target->clearBreakpoints(id);
    QTextBlock block = editor->document()->begin();
    unsigned lineCounter = 0;
    while(block != editor->document()->end()) {
        auto* uData = polymorphic_downcast_or_null<AeslEditorUserData*>(block.userData());
        //   if(uData && (uData->properties.contains("breakpoint") || uData->properties.contains("breakpointPending")))
        //       target->setBreakpoint(id, lineCounter);
        block = block.next();
        lineCounter++;
    }
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
    editorAreaLayout->addWidget(breakpoints);
    editorAreaLayout->addWidget(linenumbers);
    editorAreaLayout->addWidget(editor);

    // keywords
    keywordsToolbar = new QToolBar();
    varButton = new QToolButton();
    varButton->setText("var");
    ifButton = new QToolButton();
    ifButton->setText("if");
    elseifButton = new QToolButton();
    elseifButton->setText("elseif");
    elseButton = new QToolButton();
    elseButton->setText("else");
    oneventButton = new QToolButton();
    oneventButton->setText("onevent");
    whileButton = new QToolButton();
    whileButton->setText("while");
    forButton = new QToolButton();
    forButton->setText("for");
    subroutineButton = new QToolButton();
    subroutineButton->setText("sub");
    callsubButton = new QToolButton();
    callsubButton->setText("callsub");
    keywordsToolbar->addWidget(new QLabel(tr("<b>Keywords</b>")));
    keywordsToolbar->addSeparator();
    keywordsToolbar->addWidget(varButton);
    keywordsToolbar->addWidget(ifButton);
    keywordsToolbar->addWidget(elseifButton);
    keywordsToolbar->addWidget(elseButton);
    keywordsToolbar->addWidget(oneventButton);
    keywordsToolbar->addWidget(whileButton);
    keywordsToolbar->addWidget(forButton);
    keywordsToolbar->addWidget(subroutineButton);
    keywordsToolbar->addWidget(callsubButton);

    auto* editorLayout = new QVBoxLayout;
    editorLayout->addWidget(keywordsToolbar);
    editorLayout->addLayout(editorAreaLayout);
    editorLayout->addLayout(compilationResultLayout);
    editorLayout->addWidget(memoryUsageText);

    // panel

    // buttons
    executionModeLabel = new QLabel(tr("unknown"));

    loadButton = new QPushButton(QIcon(":/images/upload.png"), tr("Load"));
    resetButton = new QPushButton(QIcon(":/images/reset.png"), tr("Reset"));
    resetButton->setEnabled(false);
    runInterruptButton = new QPushButton(QIcon(":/images/play.png"), tr("Run"));
    runInterruptButton->setEnabled(false);
    runInterruptButton->setProperty("isRunning", false);
    nextButton = new QPushButton(QIcon(":/images/step.png"), tr("Next"));
    nextButton->setEnabled(false);
    refreshMemoryButton = new QPushButton(QIcon(":/images/rescan.png"), tr("refresh"));
    autoRefreshMemoryCheck = new QCheckBox(tr("auto"));

    auto* buttonsLayout = new QGridLayout;
    buttonsLayout->addWidget(new QLabel(tr("<b>Execution</b>")), 0, 0);
    buttonsLayout->addWidget(executionModeLabel, 0, 1);
    buttonsLayout->addWidget(loadButton, 1, 0);
    buttonsLayout->addWidget(runInterruptButton, 1, 1);
    buttonsLayout->addWidget(resetButton, 2, 0);
    buttonsLayout->addWidget(nextButton, 2, 1);

    // memory
    vmMemoryView = new QTreeView;
    // vmMemoryView->setModel(vmMemoryModel);
    vmMemoryView->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    // vmMemoryView->setItemDelegate(new SpinBoxDelegate(-32768, 32767, this));
    vmMemoryView->setColumnWidth(0, 235 - QFontMetrics(QFont()).width("-88888##"));
    vmMemoryView->setColumnWidth(1, QFontMetrics(QFont()).width("-88888##"));
    vmMemoryView->setSelectionMode(QAbstractItemView::SingleSelection);
    vmMemoryView->setSelectionBehavior(QAbstractItemView::SelectItems);
    vmMemoryView->setDragDropMode(QAbstractItemView::DragOnly);
    vmMemoryView->setDragEnabled(true);
    // vmMemoryView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    // vmMemoryView->setHeaderHidden(true);

    auto* memoryLayout = new QVBoxLayout;
    auto* memorySubLayout = new QHBoxLayout;
    memorySubLayout->addWidget(new QLabel(tr("<b>Variables</b>")));
    memorySubLayout->addStretch();
    memorySubLayout->addWidget(autoRefreshMemoryCheck);
    memorySubLayout->addWidget(refreshMemoryButton);
    memoryLayout->addLayout(memorySubLayout);
    memoryLayout->addWidget(vmMemoryView);
    memorySubLayout = new QHBoxLayout;
    QLabel* filterLabel(new QLabel(tr("F&ilter:")));
    memorySubLayout->addWidget(filterLabel);
    vmMemoryFilter = new QLineEdit;
    filterLabel->setBuddy(vmMemoryFilter);
    memorySubLayout->addWidget(vmMemoryFilter);
    memoryLayout->addLayout(memorySubLayout);

    // functions
    vmFunctionsView = new QTreeView;
    vmFunctionsView->setMinimumHeight(40);
    vmFunctionsView->setMinimumSize(QSize(50, 40));
    // vmFunctionsView->setModel(vmFunctionsModel);
    vmFunctionsView->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    vmFunctionsView->setSelectionMode(QAbstractItemView::SingleSelection);
    vmFunctionsView->setSelectionBehavior(QAbstractItemView::SelectItems);
    vmFunctionsView->setDragDropMode(QAbstractItemView::DragOnly);
    vmFunctionsView->setDragEnabled(true);
    vmFunctionsView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    vmFunctionsView->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding);
    vmFunctionsView->setHeaderHidden(true);
    // local events
    /*vmLocalEvents = new DraggableListWidget;
    vmLocalEvents->setMinimumHeight(40);
    vmLocalEvents->setSelectionMode(QAbstractItemView::SingleSelection);
    vmLocalEvents->setDragDropMode(QAbstractItemView::DragOnly);
    vmLocalEvents->setDragEnabled(true);
    for(size_t i = 0; i < target->getDescription(id)->localEvents.size(); i++) {
        QListWidgetItem* item =
            new QListWidgetItem(QString::fromStdWString(target->getDescription(id)->localEvents[i].name));
        item->setToolTip(QString::fromStdWString(target->getDescription(id)->localEvents[i].description));
        vmLocalEvents->addItem(item);
    }*/

    // toolbox
    toolBox = new QToolBox;
    toolBox->addItem(vmFunctionsView, tr("Native Functions"));
    // toolBox->addItem(vmLocalEvents, tr("Local Events"));

    // panel
    auto* panelSplitter = new QSplitter(Qt::Vertical);

    QWidget* buttonsWidget = new QWidget;
    buttonsWidget->setLayout(buttonsLayout);
    panelSplitter->addWidget(buttonsWidget);
    panelSplitter->setCollapsible(0, false);

    QWidget* memoryWidget = new QWidget;
    memoryWidget->setLayout(memoryLayout);
    panelSplitter->addWidget(memoryWidget);
    panelSplitter->setStretchFactor(1, 9);

    panelSplitter->setStretchFactor(2, 4);

    addWidget(panelSplitter);
    QWidget* editorWidget = new QWidget;
    editorWidget->setLayout(editorLayout);
    addWidget(editorWidget);
    setSizes(QList<int>() << 270 << 500);
}

void NodeTab::setupConnections() {
    // compiler
    // connect(&compilationWatcher, SIGNAL(finished()), SLOT(compilationCompleted()));

    // execution
    connect(loadButton, SIGNAL(clicked()), SLOT(loadClicked()));
    connect(resetButton, SIGNAL(clicked()), SLOT(resetClicked()));
    connect(runInterruptButton, SIGNAL(clicked()), SLOT(runInterruptClicked()));
    connect(nextButton, SIGNAL(clicked()), SLOT(nextClicked()));
    connect(refreshMemoryButton, SIGNAL(clicked()), SLOT(refreshMemoryClicked()));
    connect(autoRefreshMemoryCheck, SIGNAL(stateChanged(int)), SLOT(autoRefreshMemoryClicked(int)));

    // memory
    // connect(vmMemoryModel, SIGNAL(variableValuesChanged(unsigned, const VariablesDataVector&)),
    //        SLOT(setVariableValues(unsigned, const VariablesDataVector&)));
    connect(vmMemoryFilter, SIGNAL(textChanged(const QString&)), SLOT(updateHidden()));

    // editor
    connect(editor, SIGNAL(textChanged()), SLOT(editorContentChanged()));
    connect(editor, SIGNAL(cursorPositionChanged()), SLOT(cursorMoved()));
    connect(editor, SIGNAL(breakpointSet(unsigned)), SLOT(setBreakpoint(unsigned)));
    connect(editor, SIGNAL(breakpointCleared(unsigned)), SLOT(clearBreakpoint(unsigned)));
    connect(editor, SIGNAL(breakpointClearedAll()), SLOT(breakpointClearedAll()));
    connect(editor, SIGNAL(refreshModelRequest(LocalContext)), SLOT(refreshCompleterModel(LocalContext)));

    connect(compilationResultImage, SIGNAL(clicked()), SLOT(goToError()));
    connect(compilationResultText, SIGNAL(clicked()), SLOT(goToError()));

    // keywords
    signalMapper = new QSignalMapper(this);
    signalMapper->setMapping(varButton, QString("var "));
    signalMapper->setMapping(ifButton, QString("if"));
    signalMapper->setMapping(elseifButton, QString("elseif"));
    signalMapper->setMapping(elseButton, QString("else\n\t"));
    signalMapper->setMapping(oneventButton, QString("onevent "));
    signalMapper->setMapping(whileButton, QString("while"));
    signalMapper->setMapping(forButton, QString("for"));
    signalMapper->setMapping(subroutineButton, QString("sub "));
    signalMapper->setMapping(callsubButton, QString("callsub "));

    connect(varButton, SIGNAL(clicked()), signalMapper, SLOT(map()));
    connect(ifButton, SIGNAL(clicked()), signalMapper, SLOT(map()));
    connect(elseifButton, SIGNAL(clicked()), signalMapper, SLOT(map()));
    connect(elseButton, SIGNAL(clicked()), signalMapper, SLOT(map()));
    connect(oneventButton, SIGNAL(clicked()), signalMapper, SLOT(map()));
    connect(whileButton, SIGNAL(clicked()), signalMapper, SLOT(map()));
    connect(forButton, SIGNAL(clicked()), signalMapper, SLOT(map()));
    connect(subroutineButton, SIGNAL(clicked()), signalMapper, SLOT(map()));
    connect(callsubButton, SIGNAL(clicked()), signalMapper, SLOT(map()));

    connect(signalMapper, SIGNAL(mapped(QString)), this, SLOT(keywordClicked(QString)));

    autoRefreshMemoryCheck->setChecked(true);
}

}  // namespace Aseba