#include "MainWindow.h"
#include <QComboBox>
#include <QFileDialog>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QTableWidget>
#include <QVBoxLayout>
#include <QFrame>
#include <QWidget>
#include <QFont>
#include <QGraphicsDropShadowEffect>
#include <QScrollArea>

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
    modeBox_->setStyleSheet(
        "QComboBox { "
        "   padding: 8px 12px; "
        "   background-color: white; "
        "   border: 2px solid #e0e0e0; "
        "   border-radius: 8px; "
        "   min-width: 220px; "
        "   font-size: 13px; "
        "   color: #2c3e50; "
        "} "
        "QComboBox:hover { border-color: #4A90E2; } "
        "QComboBox:focus { border-color: #357ABD; } "
        "QComboBox::drop-down { border: none; width: 30px; } "
        "QComboBox QAbstractItemView { "
        "   background-color: white; "
        "   border: 2px solid #e0e0e0; "
        "   selection-background-color: #4A90E2; "
        "   selection-color: white; "
        "   padding: 4px; "
        "}");
    modeBox_->addItems({"Mode 0: No Pipeline / No Cache",
                        "Mode 1: Cache Only",
                        "Mode 2: Pipeline Only",
                        "Mode 3: Pipeline + Cache"});
    modeBox_->setCurrentIndex(3);

    auto *modeLabel = new QLabel("Execution Mode:");
    modeLabel->setStyleSheet("QLabel { color: #2c3e50; font-size: 14px; font-weight: 600; background: transparent; }");

    controlLayout->addWidget(loadButton_);
    controlLayout->addWidget(stepButton_);
    controlLayout->addWidget(step10Button_);
    controlLayout->addWidget(runButton_);
    controlLayout->addWidget(runBpButton_);
    controlLayout->addWidget(resetButton_);
    controlLayout->addStretch();
    controlLayout->addWidget(modeLabel);
    controlLayout->addWidget(modeBox_);

    controlGroup->setLayout(controlLayout);
    mainLayout->addWidget(controlGroup);

    // ========== BREAKPOINT PANEL ==========
    auto *bpGroup = new QGroupBox();
    bpGroup->setStyleSheet(
        "QGroupBox { "
        "   background-color: white; "
        "   border-radius: 12px; "
        "   padding: 20px; "
        "   border: 1px solid #e0e0e0; "
        "}");
    auto *bpLayout = new QHBoxLayout();
    bpLayout->setSpacing(12);

    auto *bpLabel = new QLabel("Breakpoints:");
    bpLabel->setStyleSheet("QLabel { color: #2c3e50; font-size: 14px; font-weight: 700; background: transparent; }");

    bpAddressEdit_ = new QLineEdit();
    bpAddressEdit_->setPlaceholderText("Enter address...");
    bpAddressEdit_->setStyleSheet(
        "QLineEdit { "
        "   padding: 8px 12px; "
        "   background-color: #f8f9fa; "
        "   border: 2px solid #e0e0e0; "
        "   border-radius: 8px; "
        "   font-size: 13px; "
        "   max-width: 160px; "
        "   color: #2c3e50; "
        "} "
        "QLineEdit:focus { "
        "   border-color: #4A90E2; "
        "   background-color: white; "
        "}");

    QString bpButtonStyle =
        "QPushButton { "
        "   padding: 8px 16px; "
        "   background: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
        "       stop:0 #51CF66, stop:1 #40C057); "
        "   color: white; "
        "   border: none; "
        "   border-radius: 6px; "
        "   font-size: 13px; "
        "   font-weight: 600; "
        "} "
        "QPushButton:hover { "
        "   background: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
        "       stop:0 #61DF76, stop:1 #51CF66); "
        "}";

    setBpButton_ = new QPushButton("➕ Set");
    clearBpButton_ = new QPushButton("➖ Clear");
    clearAllBpButton_ = new QPushButton("🗑 Clear All");

    setBpButton_->setStyleSheet(bpButtonStyle);
    clearBpButton_->setStyleSheet(bpButtonStyle);
    clearAllBpButton_->setStyleSheet(bpButtonStyle);

    bpList_ = new QListWidget();
    bpList_->setMaximumHeight(50);
    bpList_->setStyleSheet(
        "QListWidget { "
        "   background-color: #f8f9fa; "
        "   border: 2px solid #e0e0e0; "
        "   border-radius: 8px; "
        "   padding: 6px; "
        "   font-size: 13px; "
        "   color: #1d1d1f; "
        "} "
        "QListWidget::item { "
        "   color: #1d1d1f; "
        "}");

    bpLayout->addWidget(bpLabel);
    bpLayout->addWidget(bpAddressEdit_);
    bpLayout->addWidget(setBpButton_);
    bpLayout->addWidget(clearBpButton_);
    bpLayout->addWidget(clearAllBpButton_);
    bpLayout->addWidget(bpList_, 1);

    bpGroup->setLayout(bpLayout);
    mainLayout->addWidget(bpGroup);

    // ========== STATE DISPLAY GRID ==========
    auto *gridLayout = new QGridLayout();
    gridLayout->setSpacing(16);

    // Shared styles for uniform panel cards
    QString panelFrameStyle =
        "QFrame {"
        "   background-color: white;"
        "   border-radius: 12px;"
        "   border: 1px solid #e0e0e0;"
        "}";
    QString panelHeaderStyle =
        "QWidget {"
        "   background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #667eea, stop:1 #764ba2);"
        "   border-top-left-radius: 10px;"
        "   border-top-right-radius: 10px;"
        "}";
    QString panelTitleStyle =
        "QLabel { color: white; font-size: 14px; font-weight: 700; background: transparent; }";

    auto makePanelFrame = [&](const QString &title) -> std::pair<QFrame *, QVBoxLayout *>
    {
        auto *frame = new QFrame();
        frame->setStyleSheet(panelFrameStyle);
        auto *vbox = new QVBoxLayout();
        vbox->setSpacing(8);
        vbox->setContentsMargins(0, 0, 0, 16);
        auto *header = new QWidget();
        header->setStyleSheet(panelHeaderStyle);
        auto *hdr = new QHBoxLayout(header);
        hdr->setContentsMargins(16, 10, 16, 10);
        auto *lbl = new QLabel(title);
        lbl->setStyleSheet(panelTitleStyle);
        hdr->addWidget(lbl);
        hdr->addStretch();
        vbox->addWidget(header);
        return {frame, vbox};
    };

    auto wrapContent = [](QWidget *w, QVBoxLayout *vbox, int l = 16, int t = 4, int r = 16, int b = 0)
    {
        auto *wrapper = new QWidget();
        auto *wl = new QVBoxLayout(wrapper);
        wl->setContentsMargins(l, t, r, b);
        wl->addWidget(w);
        vbox->addWidget(wrapper);
    };

    QString tableStyle =
        "QTableWidget { "
        "   background-color: #fafbfc; "
        "   border: none; "
        "   gridline-color: #e1e4e8; "
        "   font-size: 13px; "
        "   color: #24292e; "
        "   selection-background-color: #0366d6; "
        "   selection-color: white; "
        "} "
        "QTableWidget::item { "
        "   padding: 6px; "
        "   color: #24292e; "
        "} "
        "QTableWidget::item:selected { "
        "   background-color: #0366d6; "
        "   color: white; "
        "} "
        "QHeaderView::section { "
        "   background: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
        "       stop:0 #f6f8fa, stop:1 #e1e4e8); "
        "   padding: 8px; "
        "   border: none; "
        "   border-bottom: 2px solid #d1d5da; "
        "   border-right: 1px solid #e1e4e8; "
        "   font-size: 12px; "
        "   font-weight: 700; "
        "   color: #586069; "
        "}";

    // Registers
    auto [regGroup, regLayout] = makePanelFrame("\U0001F4CB  Registers");
    regTable_ = new QTableWidget(16, 2);
    regTable_->setHorizontalHeaderLabels({"Register", "Value"});
    regTable_->horizontalHeader()->setStretchLastSection(true);
    regTable_->verticalHeader()->setVisible(false);
    regTable_->setAlternatingRowColors(true);
    regTable_->verticalHeader()->setDefaultSectionSize(26);

    pipelineView_ = new QPlainTextEdit();
    pipelineView_->setReadOnly(true);
    pipelineView_->setMinimumHeight(260);
    pipelineView_->setLineWrapMode(QPlainTextEdit::NoWrap);

    l1Table_ = new QTableWidget(L1_NUM_LINES, 5);
    l1Table_->setHorizontalHeaderLabels({"Index", "Valid", "Tag", "Dirty", "Data [4 words]"});
    l1Table_->horizontalHeader()->setStretchLastSection(true);
    l1Table_->verticalHeader()->setVisible(false);
    l1Table_->setMinimumWidth(720);
    l1Table_->setAlternatingRowColors(true);
    l1Table_->verticalHeader()->setDefaultSectionSize(24);

    l2Table_ = new QTableWidget(L2_NUM_LINES, 5);
    l2Table_->setHorizontalHeaderLabels({"Index", "Valid", "Tag", "Dirty", "Data [4 words]"});
    l2Table_->horizontalHeader()->setStretchLastSection(true);
    l2Table_->verticalHeader()->setVisible(false);
    l2Table_->setAlternatingRowColors(true);
    l2Table_->verticalHeader()->setDefaultSectionSize(24);

    memTable_ = new QTableWidget(16, 2);
    memTable_->setHorizontalHeaderLabels({"Address", "Data [4 words]"});
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
#ifdef Q_OS_MAC
    QFont statusFont("Menlo", 12, QFont::Medium);
#else
    QFont statusFont("Consolas", 12, QFont::Medium);
#endif
    statusLabel_->setFont(statusFont);
    statusLabel_->setStyleSheet(
        "QLabel { "
        "   background: qlineargradient(x1:0, y1:0, x2:1, y2:0, "
        "       stop:0 #2c3e50, stop:1 #34495e); "
        "   color: #ecf0f1; "
        "   padding: 14px 20px; "
        "   border-radius: 10px; "
        "   border: 2px solid #1a252f; "
        "}");
    mainLayout->addWidget(statusLabel_);

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
    connect(memStartSpin_, QOverload<int>::of(&QSpinBox::valueChanged), this, &MainWindow::onMemoryStartChanged);
}

void MainWindow::refreshView()
{
    auto snap = sim_.getSnapshot(memStartSpin_ ? memStartSpin_->value() : 0, 16);

    // Update registers
    for (int i = 0; i < 16; ++i)
    {
        QString regName;
        if (i == 0)
            regName = "R0 (zero)";
        else if (i == 14)
            regName = "R14 (SP)";
        else if (i == 15)
            regName = "R15 (RA)";
        else
            regName = QString("R%1").arg(i);

        regTable_->setItem(i, 0, new QTableWidgetItem(regName));
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

    // Update L1 cache
    for (size_t i = 0; i < snap.l1Rows.size(); ++i)
    {
        const auto &r = snap.l1Rows[i];
        QString data = QString("%1 %2 %3 %4")
                           .arg(r.data[0], 8, 16, QChar('0'))
                           .arg(r.data[1], 8, 16, QChar('0'))
                           .arg(r.data[2], 8, 16, QChar('0'))
                           .arg(r.data[3], 8, 16, QChar('0'));

        l1Table_->setItem(i, 0, new QTableWidgetItem(QString::number(r.index)));
        l1Table_->setItem(i, 1, new QTableWidgetItem(r.valid ? "✓" : "✗"));
        l1Table_->setItem(i, 2, new QTableWidgetItem(QString::number(r.tag, 16).toUpper()));
        l1Table_->setItem(i, 3, new QTableWidgetItem(r.dirty ? "✓" : "✗"));
        l1Table_->setItem(i, 4, new QTableWidgetItem(data.toUpper()));
    }

    // Update L2 cache
    for (size_t i = 0; i < snap.l2Rows.size(); ++i)
    {
        const auto &r = snap.l2Rows[i];
        QString data = QString("%1 %2 %3 %4")
                           .arg(r.data[0], 8, 16, QChar('0'))
                           .arg(r.data[1], 8, 16, QChar('0'))
                           .arg(r.data[2], 8, 16, QChar('0'))
                           .arg(r.data[3], 8, 16, QChar('0'));

        l2Table_->setItem(i, 0, new QTableWidgetItem(QString::number(r.index)));
        l2Table_->setItem(i, 1, new QTableWidgetItem(r.valid ? "✓" : "✗"));
        l2Table_->setItem(i, 2, new QTableWidgetItem(QString::number(r.tag, 16).toUpper()));
        l2Table_->setItem(i, 3, new QTableWidgetItem(r.dirty ? "✓" : "✗"));
        l2Table_->setItem(i, 4, new QTableWidgetItem(data.toUpper()));
    }

    // Update memory
    memTable_->setRowCount(static_cast<int>(snap.memoryRows.size()));
    for (size_t i = 0; i < snap.memoryRows.size(); ++i)
    {
        const auto &r = snap.memoryRows[i];
        QString data = QString("%1 %2 %3 %4")
                           .arg(r.data[0], 8, 16, QChar('0'))
                           .arg(r.data[1], 8, 16, QChar('0'))
                           .arg(r.data[2], 8, 16, QChar('0'))
                           .arg(r.data[3], 8, 16, QChar('0'));

        memTable_->setItem(i, 0, new QTableWidgetItem(QString::number(r.baseAddress)));
        memTable_->setItem(i, 1, new QTableWidgetItem(data.toUpper()));
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
    QString file = QFileDialog::getOpenFileName(
        this,
        "Load Assembly Program",
        "../demo",
        "Assembly Files (*.asm *.txt);;All Files (*)");
    if (file.isEmpty())
        return;

    try
    {
        sim_.loadProgramAsm(file.toStdString());
        refreshView();
        QMessageBox::information(this, "Success", "Program loaded successfully!");
    }
    catch (const std::exception &e)
    {
        QMessageBox::critical(this, "Load Error", QString("Failed to load program:\n%1").arg(e.what()));
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
    QString result = QString::fromStdString(sim_.run());
    refreshView();
    if (result.contains("HALTED"))
    {
        QMessageBox::information(this, "Execution Complete", "Program halted successfully!");
    }
}

void MainWindow::onRunToBreakpoint()
{
    QString msg = QString::fromStdString(sim_.runUntilBreakpoint());
    refreshView();
    if (msg.contains("breakpoint", Qt::CaseInsensitive))
    {
        QMessageBox::information(this, "Breakpoint Hit", msg);
    }
    else if (msg.contains("HALTED"))
    {
        QMessageBox::information(this, "Execution Complete", "Program halted before hitting breakpoint");
    }
}

void MainWindow::onReset()
{
    sim_.reset();
    refreshView();
    QMessageBox::information(this, "Reset", "Simulator reset to initial state");
}

void MainWindow::onModeChanged(int index)
{
    sim_.setMode(static_cast<ExecMode>(index));
    refreshView();
}

void MainWindow::onSetBreakpoint()
{
    bool ok = false;
    uint32_t addr = bpAddressEdit_->text().toUInt(&ok);
    if (ok)
    {
        sim_.addBreakpoint(addr);
        bpAddressEdit_->clear();
        refreshView();
    }
    else
    {
        QMessageBox::warning(this, "Invalid Input", "Please enter a valid address number");
    }
}

void MainWindow::onClearBreakpoint()
{
    auto selected = bpList_->selectedItems();
    if (!selected.isEmpty())
    {
        QString text = selected[0]->text();
        QStringList parts = text.split(" ");
        if (parts.size() >= 3)
        {
            uint32_t addr = parts[2].toUInt();
            sim_.clearBreakpoint(addr);
            refreshView();
        }
    }
    else
    {
        QMessageBox::information(this, "No Selection", "Please select a breakpoint to clear");
    }
}

void MainWindow::onClearAllBreakpoints()
{
    sim_.clearBreakpoints();
    refreshView();
}

void MainWindow::onMemoryStartChanged(int value)
{
    (void)value;
    refreshView();
}