#include "MainWindow.h"

#include <QComboBox>
#include <QFileDialog>
#include <QGridLayout>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>
#include <QWidget>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
    buildUi();
    refreshView();
}

void MainWindow::buildUi()
{
    auto *central = new QWidget(this);
    auto *root = new QVBoxLayout(central);

    auto *title = new QLabel("SAV_535 ISA Simulator");
    QFont titleFont = title->font();
    titleFont.setPointSize(16);
    titleFont.setBold(true);
    title->setFont(titleFont);
    root->addWidget(title);

    auto *top = new QHBoxLayout();
    loadButton_ = new QPushButton("Load ASM");
    stepButton_ = new QPushButton("Step");
    step10Button_ = new QPushButton("Step x10");
    runButton_ = new QPushButton("Run");
    runBpButton_ = new QPushButton("Run to BP");
    resetButton_ = new QPushButton("Reset");

    modeBox_ = new QComboBox();
    modeBox_->addItems({"No Pipe / No Cache",
                        "Cache Only",
                        "Pipe Only",
                        "Pipe + Cache"});

    top->addWidget(loadButton_);
    top->addWidget(stepButton_);
    top->addWidget(step10Button_);
    top->addWidget(runButton_);
    top->addWidget(runBpButton_);
    top->addWidget(resetButton_);
    top->addWidget(new QLabel("Mode:"));
    top->addWidget(modeBox_);
    root->addLayout(top);

    auto *bpRow = new QHBoxLayout();
    breakpointEdit_ = new QLineEdit();
    breakpointEdit_->setPlaceholderText("Breakpoint PC");
    setBpButton_ = new QPushButton("Set BP");
    clearBpButton_ = new QPushButton("Clear BP");
    clearAllBpButton_ = new QPushButton("Clear All BPs");

    memoryStartSpin_ = new QSpinBox();
    memoryStartSpin_->setRange(0, static_cast<int>(RAM_WORDS - WORDS_PER_LINE));
    memoryStartSpin_->setSingleStep(WORDS_PER_LINE);
    memoryStartSpin_->setValue(0);

    bpRow->addWidget(new QLabel("Breakpoint:"));
    bpRow->addWidget(breakpointEdit_);
    bpRow->addWidget(setBpButton_);
    bpRow->addWidget(clearBpButton_);
    bpRow->addWidget(clearAllBpButton_);
    bpRow->addSpacing(20);
    bpRow->addWidget(new QLabel("Memory start:"));
    bpRow->addWidget(memoryStartSpin_);
    root->addLayout(bpRow);

    auto *grid = new QGridLayout();

    regTable_ = new QTableWidget(16, 2);
    regTable_->setHorizontalHeaderLabels({"Reg", "Value"});
    regTable_->horizontalHeader()->setStretchLastSection(true);
    regTable_->verticalHeader()->setVisible(false);
    regTable_->setAlternatingRowColors(true);
    regTable_->verticalHeader()->setDefaultSectionSize(26);

    pipelineView_ = new QPlainTextEdit();
    pipelineView_->setReadOnly(true);
    pipelineView_->setMinimumHeight(260);
    pipelineView_->setLineWrapMode(QPlainTextEdit::NoWrap);

    l1Table_ = new QTableWidget(L1_NUM_LINES, 5);
    l1Table_->setHorizontalHeaderLabels({"Idx", "Valid", "Tag", "Dirty", "Data"});
    l1Table_->horizontalHeader()->setStretchLastSection(true);
    l1Table_->verticalHeader()->setVisible(false);
    l1Table_->setMinimumWidth(720);
    l1Table_->setAlternatingRowColors(true);
    l1Table_->verticalHeader()->setDefaultSectionSize(24);

    l2Table_ = new QTableWidget(L2_NUM_LINES, 5);
    l2Table_->setHorizontalHeaderLabels({"Idx", "Valid", "Tag", "Dirty", "Data"});
    l2Table_->horizontalHeader()->setStretchLastSection(true);
    l2Table_->verticalHeader()->setVisible(false);
    l2Table_->setAlternatingRowColors(true);
    l2Table_->verticalHeader()->setDefaultSectionSize(24);

    memTable_ = new QTableWidget(16, 2);
    memTable_->setHorizontalHeaderLabels({"Base", "Words"});
    memTable_->horizontalHeader()->setStretchLastSection(true);
    memTable_->verticalHeader()->setVisible(false);
    memTable_->setAlternatingRowColors(true);
    memTable_->verticalHeader()->setDefaultSectionSize(24);

    grid->addWidget(new QLabel("Registers"), 0, 0);
    grid->addWidget(regTable_, 1, 0);

    grid->addWidget(new QLabel("Pipeline / Status"), 0, 1);
    grid->addWidget(pipelineView_, 1, 1);

    grid->addWidget(new QLabel("L1 Cache"), 2, 0);
    grid->addWidget(l1Table_, 3, 0);

    grid->addWidget(new QLabel("L2 Cache"), 2, 1);
    grid->addWidget(l2Table_, 3, 1);

    grid->addWidget(new QLabel("Memory"), 4, 0, 1, 2);
    grid->addWidget(memTable_, 5, 0, 1, 2);

    root->addLayout(grid);

    statusLabel_ = new QLabel();
    root->addWidget(statusLabel_);

    setCentralWidget(central);
    resize(1550, 920);
    setWindowTitle("SAV_535 Simulator");

    setStyleSheet(R"(
        QMainWindow, QWidget {
            background-color: #f4efff;
            color: #24183a;
            font-size: 13px;
        }

        QLabel {
            color: #2f1f4f;
            font-weight: 600;
        }

        QPushButton {
            background-color: #7c5ac9;
            color: white;
            border: 1px solid #6747b3;
            border-radius: 8px;
            padding: 6px 12px;
            font-weight: 600;
        }

        QPushButton:hover {
            background-color: #6d4fc0;
        }

        QPushButton:pressed {
            background-color: #5f43ad;
        }

        QComboBox, QLineEdit, QSpinBox {
            background-color: white;
            color: #24183a;
            border: 1px solid #c9b8ee;
            border-radius: 6px;
            padding: 4px 6px;
        }

        QPlainTextEdit {
            background-color: #fffaff;
            color: #231733;
            border: 1px solid #ccbdf0;
            border-radius: 8px;
            padding: 8px;
            font-family: Consolas, 'Courier New', monospace;
            font-size: 13px;
        }

        QTableWidget {
            background-color: white;
            alternate-background-color: #f7f2ff;
            color: #24183a;
            gridline-color: #d9cdf4;
            border: 1px solid #ccbdf0;
            border-radius: 8px;
        }

        QHeaderView::section {
            background-color: #d8c8f7;
            color: #2a1d45;
            padding: 6px;
            border: 1px solid #c6b4ee;
            font-weight: 700;
        }
    )");

    connect(loadButton_, &QPushButton::clicked, this, &MainWindow::onLoadAsm);
    connect(stepButton_, &QPushButton::clicked, this, &MainWindow::onStep);
    connect(step10Button_, &QPushButton::clicked, this, &MainWindow::onStep10);
    connect(runButton_, &QPushButton::clicked, this, &MainWindow::onRun);
    connect(runBpButton_, &QPushButton::clicked, this, &MainWindow::onRunToBreakpoint);
    connect(resetButton_, &QPushButton::clicked, this, &MainWindow::onReset);
    connect(modeBox_, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::onModeChanged);

    connect(setBpButton_, &QPushButton::clicked, this, &MainWindow::onSetBreakpoint);
    connect(clearBpButton_, &QPushButton::clicked, this, &MainWindow::onClearBreakpoint);
    connect(clearAllBpButton_, &QPushButton::clicked, this, &MainWindow::onClearAllBreakpoints);

    connect(memoryStartSpin_, QOverload<int>::of(&QSpinBox::valueChanged), this, &MainWindow::onMemoryStartChanged);
}

void MainWindow::refreshView()
{
    auto snap = sim_.getSnapshot(static_cast<uint32_t>(memoryStartSpin_->value()), 16);

    for (int i = 0; i < 16; ++i)
    {
        regTable_->setItem(i, 0, new QTableWidgetItem(QString("R%1").arg(i)));
        regTable_->setItem(i, 1, new QTableWidgetItem(QString::number(snap.regs[i])));
    }

    QString ifLine = QString::fromStdString(snap.ifStage);
    if (snap.fetchInFlight && snap.ifStage == "<empty>")
    {
        ifLine = QString("[Fetching PC=%1, %2 cyc left]")
                     .arg(snap.fetchPC)
                     .arg(snap.fetchRemaining);
    }

    QString pipe =
        "IF  : " + ifLine + "\n" +
        "ID  : " + QString::fromStdString(snap.idStage) + "\n" +
        "EX  : " + QString::fromStdString(snap.exStage) + "\n" +
        "MEM : " + QString::fromStdString(snap.memStage) + "\n" +
        "WB  : " + QString::fromStdString(snap.wbStage) + "\n\n" +
        "Summary   : " + QString::fromStdString(snap.summary) + "\n" +
        "Flags     : " + QString::fromStdString(snap.flags) + "\n" +
        "Hierarchy : " + QString::fromStdString(snap.hierarchyState) + "\n" +
        "Sequential: " + QString::fromStdString(snap.seqState) + "\n";

    if (snap.mode == "NO_PIPE_NO_CACHE" || snap.mode == "NO_PIPE_CACHE")
    {
        pipe += "\nNote: Sequential mode active, pipeline registers are expected to be mostly inactive.";
    }

    pipelineView_->setPlainText(pipe);

    l1Table_->setRowCount(static_cast<int>(snap.l1Rows.size()));
    for (int i = 0; i < static_cast<int>(snap.l1Rows.size()); ++i)
    {
        const auto &r = snap.l1Rows[i];
        QString data = QString("%1 %2 %3 %4")
                           .arg(QString("0x%1").arg(r.data[0], 8, 16, QChar('0')))
                           .arg(QString("0x%1").arg(r.data[1], 8, 16, QChar('0')))
                           .arg(QString("0x%1").arg(r.data[2], 8, 16, QChar('0')))
                           .arg(QString("0x%1").arg(r.data[3], 8, 16, QChar('0')));

        l1Table_->setItem(i, 0, new QTableWidgetItem(QString::number(r.index)));
        l1Table_->setItem(i, 1, new QTableWidgetItem(r.valid ? "1" : "0"));
        l1Table_->setItem(i, 2, new QTableWidgetItem(QString::number(r.tag)));
        l1Table_->setItem(i, 3, new QTableWidgetItem(r.dirty ? "1" : "0"));
        l1Table_->setItem(i, 4, new QTableWidgetItem(data));
    }

    l2Table_->setRowCount(static_cast<int>(snap.l2Rows.size()));
    for (int i = 0; i < static_cast<int>(snap.l2Rows.size()); ++i)
    {
        const auto &r = snap.l2Rows[i];
        QString data = QString("%1 %2 %3 %4")
                           .arg(QString("0x%1").arg(r.data[0], 8, 16, QChar('0')))
                           .arg(QString("0x%1").arg(r.data[1], 8, 16, QChar('0')))
                           .arg(QString("0x%1").arg(r.data[2], 8, 16, QChar('0')))
                           .arg(QString("0x%1").arg(r.data[3], 8, 16, QChar('0')));

        l2Table_->setItem(i, 0, new QTableWidgetItem(QString::number(r.index)));
        l2Table_->setItem(i, 1, new QTableWidgetItem(r.valid ? "1" : "0"));
        l2Table_->setItem(i, 2, new QTableWidgetItem(QString::number(r.tag)));
        l2Table_->setItem(i, 3, new QTableWidgetItem(r.dirty ? "1" : "0"));
        l2Table_->setItem(i, 4, new QTableWidgetItem(data));
    }

    memTable_->setRowCount(static_cast<int>(snap.memoryRows.size()));
    for (int i = 0; i < static_cast<int>(snap.memoryRows.size()); ++i)
    {
        const auto &r = snap.memoryRows[i];
        QString data = QString("%1 %2 %3 %4")
                           .arg(QString("0x%1").arg(r.data[0], 8, 16, QChar('0')))
                           .arg(QString("0x%1").arg(r.data[1], 8, 16, QChar('0')))
                           .arg(QString("0x%1").arg(r.data[2], 8, 16, QChar('0')))
                           .arg(QString("0x%1").arg(r.data[3], 8, 16, QChar('0')));

        memTable_->setItem(i, 0, new QTableWidgetItem(QString::number(r.baseAddress)));
        memTable_->setItem(i, 1, new QTableWidgetItem(data));
    }

    statusLabel_->setText(
        QString("PC=%1  Cycles=%2  Halted=%3  HaltRequested=%4  Faulted=%5  Z=%6  L1 H/M=%7/%8  L2 H/M=%9/%10")
            .arg(snap.pc)
            .arg(snap.cycles)
            .arg(snap.halted ? "yes" : "no")
            .arg(snap.haltRequested ? "yes" : "no")
            .arg(snap.faulted ? "yes" : "no")
            .arg(snap.zFlag ? "1" : "0")
            .arg(snap.l1Hits)
            .arg(snap.l1Misses)
            .arg(snap.l2Hits)
            .arg(snap.l2Misses));
}

void MainWindow::onLoadAsm()
{
    const auto file = QFileDialog::getOpenFileName(this, "Load ASM", QString(), "Assembly (*.asm *.txt)");
    if (file.isEmpty())
        return;

    try
    {
        sim_.loadProgramAsm(file.toStdString());
        refreshView();
    }
    catch (const std::exception &e)
    {
        QMessageBox::critical(this, "Load failed", e.what());
    }
}

void MainWindow::onStep()
{
    sim_.step();
    refreshView();
}

void MainWindow::onStep10()
{
    for (int i = 0; i < 10; ++i)
    {
        if (sim_.step() == "HALTED" || sim_.step() == "FAULT")
            break;
    }
    refreshView();
}

void MainWindow::onRun()
{
    sim_.run();
    refreshView();
}

void MainWindow::onRunToBreakpoint()
{
    sim_.runUntilBreakpoint();
    refreshView();
}

void MainWindow::onReset()
{
    sim_.reset();
    refreshView();
}

void MainWindow::onModeChanged(int index)
{
    sim_.setMode(static_cast<ExecMode>(index));
    refreshView();
}

void MainWindow::onSetBreakpoint()
{
    bool ok = false;
    const uint32_t addr = breakpointEdit_->text().toUInt(&ok);
    if (!ok)
    {
        QMessageBox::warning(this, "Breakpoint", "Enter a valid numeric PC.");
        return;
    }
    sim_.addBreakpoint(addr);
    refreshView();
}

void MainWindow::onClearBreakpoint()
{
    bool ok = false;
    const uint32_t addr = breakpointEdit_->text().toUInt(&ok);
    if (!ok)
    {
        QMessageBox::warning(this, "Breakpoint", "Enter a valid numeric PC.");
        return;
    }
    sim_.clearBreakpoint(addr);
    refreshView();
}

void MainWindow::onClearAllBreakpoints()
{
    sim_.clearBreakpoints();
    refreshView();
}

void MainWindow::onMemoryStartChanged(int)
{
    refreshView();
}