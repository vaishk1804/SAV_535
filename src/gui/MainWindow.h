#pragma once
#include <QMainWindow>
#include "Simulator.h"

class QComboBox;
class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QPushButton;
class QSpinBox;
class QTableWidget;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);

private slots:
    void onLoadAsm();
    void onStep();
    void onStep10();
    void onRun();
    void onRunToBreakpoint();
    void onReset();
    void onModeChanged(int index);

    void onSetBreakpoint();
    void onClearBreakpoint();
    void onClearAllBreakpoints();

    void onMemoryStartChanged(int value);

private:
    void buildUi();
    void refreshView();

    Simulator sim_;

    QPushButton *loadButton_ = nullptr;
    QPushButton *stepButton_ = nullptr;
    QPushButton *step10Button_ = nullptr;
    QPushButton *runButton_ = nullptr;
    QPushButton *runBpButton_ = nullptr;
    QPushButton *resetButton_ = nullptr;

    QComboBox *modeBox_ = nullptr;

    QLineEdit *breakpointEdit_ = nullptr;
    QPushButton *setBpButton_ = nullptr;
    QPushButton *clearBpButton_ = nullptr;
    QPushButton *clearAllBpButton_ = nullptr;

    QSpinBox *memStartSpin_ = nullptr;

    QTableWidget *regTable_ = nullptr;
    QTableWidget *l1Table_ = nullptr;
    QTableWidget *l2Table_ = nullptr;
    QTableWidget *memTable_ = nullptr;
    QPlainTextEdit *pipelineView_ = nullptr;
    QLabel *statusLabel_ = nullptr;
};