#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QSpinBox>
#include <QTimer>
#include <QLabel>
#include <QSlider> // 【新增】
#include "datamanager.h"
#include "ppiwidget.h"
#include "qcustomplot.h"

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void loadFiles();
    void updateFilter(double val);
    void updateLinePlot(int rayIndex);
    void onModeChanged(int index);
    void onWindowSizeChanged(int val);
    void onExportData();
    void onRangeChanged();

private:
    void setupUi();
    void updateStatusBar();

    DataManager m_manager;
    PPIWidget *m_ppi;
    QCustomPlot *m_speedPlot;
    QCustomPlot *m_snrPlot;

    QDoubleSpinBox *m_snrBox;
    QComboBox *m_comboMode;
    QSpinBox *m_spinWinSize;

    // 【新增】距离控制相关
    QSlider *m_minSlider; // 最小距离滑条
    QSlider *m_maxSlider; // 最大距离滑条
    QSpinBox *m_minDistBox;
    QSpinBox *m_maxDistBox;

    QLabel *m_statusLabel;
    QString m_currentFileName = "未加载";

    QTimer *m_playTimer;
    int m_playIndex = 0;
    DisplayMode m_currentMode = Mode_Speed;
    QCPCurve *m_speedCurve;
    QCPCurve *m_snrCurve;
};

#endif // MAINWINDOW_H
