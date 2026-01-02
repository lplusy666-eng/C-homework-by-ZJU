#include "mainwindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QFileDialog>
#include <QSplitter>
#include <QMessageBox>
#include <QStatusBar>
#include <QFileInfo>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    setupUi();
    resize(1250, 850); // 稍微加宽一点以容纳滑条
    setWindowTitle("测风雷达数据分析平台");

    m_playTimer = new QTimer(this);
    connect(m_playTimer, &QTimer::timeout, [=](){
        m_playIndex += 2;
        if (m_playIndex <= m_manager.getScanData().size()) m_ppi->setPlayLimit(m_playIndex);
        else m_playTimer->stop();
    });
}

MainWindow::~MainWindow() {}

void MainWindow::setupUi() {
    QWidget *center = new QWidget;
    setCentralWidget(center);
    QVBoxLayout *mainLayout = new QVBoxLayout(center);
    mainLayout->setContentsMargins(0, 0, 0, 0); mainLayout->setSpacing(0);

    // --- 工具栏 ---
    QWidget *toolWidget = new QWidget;
    toolWidget->setFixedHeight(80); // 增高一点以容纳滑条布局

    // 【修改点 1】修复字体颜色的样式表
    QString style =
        "QWidget { background-color: #f5f5f5; border-bottom: 1px solid #dcdcdc; font-family: 'Microsoft YaHei'; }"
        "QLabel { border: none; background: transparent; color: black; font-weight: bold; }"
        "QPushButton { min-width: 60px; padding: 4px; color: black; border: 1px solid #aaa; border-radius: 3px; background-color: white; }"
        "QPushButton:hover { background-color: #e0e0e0; }"

        // 核心修复：针对下拉列表视图的样式
        "QComboBox { color: black; background-color: white; border: 1px solid #aaa; padding: 3px; }"
        "QComboBox QAbstractItemView { color: black; background-color: white; selection-background-color: #d0d0d0; selection-color: black; }"

        "QSpinBox, QDoubleSpinBox { color: black; background-color: white; border: 1px solid #aaa; padding: 2px; }"
        // 滑动条样式微调
        "QSlider::groove:horizontal { border: 1px solid #999999; height: 4px; background: #e0e0e0; margin: 2px 0; }"
        "QSlider::handle:horizontal { background: #5c5c5c; border: 1px solid #5c5c5c; width: 12px; height: 12px; margin: -4px 0; border-radius: 6px; }";

    toolWidget->setStyleSheet(style);

    // 全局 ToolTip 样式
    qApp->setStyleSheet("QToolTip { color: black; background-color: #ffffe0; border: 1px solid black; padding: 2px; }");

    QHBoxLayout *toolLayout = new QHBoxLayout(toolWidget);

    QPushButton *btnLoad = new QPushButton("📂 导入", this);
    m_snrBox = new QDoubleSpinBox; m_snrBox->setRange(-50, 50); m_snrBox->setValue(-20);

    m_comboMode = new QComboBox;
    m_comboMode->addItem("径向风速", QVariant(Mode_Speed));
    m_comboMode->addItem("湍流强度", QVariant(Mode_Turbulence));

    m_spinWinSize = new QSpinBox; m_spinWinSize->setRange(2, 20); m_spinWinSize->setValue(5);

    // 【修改点 2】创建距离滑条控件组
    QWidget *rangeGroup = new QWidget;
    QVBoxLayout *rangeLayout = new QVBoxLayout(rangeGroup);
    rangeLayout->setContentsMargins(0,0,0,0);
    rangeLayout->setSpacing(2);

    // 第一行：最小距离控制
    QHBoxLayout *minLayout = new QHBoxLayout;
    m_minSlider = new QSlider(Qt::Horizontal);
    m_minSlider->setRange(0, 10000); m_minSlider->setValue(0); m_minSlider->setFixedWidth(100);
    m_minDistBox = new QSpinBox; m_minDistBox->setRange(0, 10000); m_minDistBox->setValue(0); m_minDistBox->setSuffix("m"); m_minDistBox->setFixedWidth(70);
    minLayout->addWidget(new QLabel("Min:")); minLayout->addWidget(m_minSlider); minLayout->addWidget(m_minDistBox);

    // 第二行：最大距离控制
    QHBoxLayout *maxLayout = new QHBoxLayout;
    m_maxSlider = new QSlider(Qt::Horizontal);
    m_maxSlider->setRange(0, 10000); m_maxSlider->setValue(4000); m_maxSlider->setFixedWidth(100);
    m_maxDistBox = new QSpinBox; m_maxDistBox->setRange(0, 10000); m_maxDistBox->setValue(4000); m_maxDistBox->setSuffix("m"); m_maxDistBox->setFixedWidth(70);
    maxLayout->addWidget(new QLabel("Max:")); maxLayout->addWidget(m_maxSlider); maxLayout->addWidget(m_maxDistBox);

    rangeLayout->addLayout(minLayout);
    rangeLayout->addLayout(maxLayout);

    QPushButton *btnExp = new QPushButton("📊 导出");
    QPushButton *btnShot = new QPushButton("📸 截图");

    // 添加到工具栏布局
    toolLayout->addWidget(btnLoad);
    toolLayout->addSpacing(10);

    // 参数组
    QVBoxLayout *paramLayout = new QVBoxLayout;
    paramLayout->setSpacing(5);
    QHBoxLayout *p1 = new QHBoxLayout; p1->addWidget(new QLabel("SNR:")); p1->addWidget(m_snrBox);
    QHBoxLayout *p2 = new QHBoxLayout; p2->addWidget(new QLabel("模式:")); p2->addWidget(m_comboMode);
    paramLayout->addLayout(p1); paramLayout->addLayout(p2);
    toolLayout->addLayout(paramLayout);

    toolLayout->addWidget(new QLabel("窗口:")); toolLayout->addWidget(m_spinWinSize);

    toolLayout->addWidget(new QLabel("|")); // 分隔符
    toolLayout->addWidget(rangeGroup); // 加入滑条组

    toolLayout->addStretch();
    toolLayout->addWidget(btnExp);
    toolLayout->addWidget(btnShot);

    // --- 可视化区域 (保持不变) ---
    QSplitter *vSplitter = new QSplitter(Qt::Vertical);
    vSplitter->setHandleWidth(4);

    m_ppi = new PPIWidget;
    vSplitter->addWidget(m_ppi);

    QWidget *bottomArea = new QWidget;
    QHBoxLayout *hLayout = new QHBoxLayout(bottomArea);
    m_speedPlot = new QCustomPlot;
    m_snrPlot = new QCustomPlot;
    m_speedPlot->setBackground(QBrush(Qt::white)); m_snrPlot->setBackground(QBrush(Qt::white));

    m_speedCurve = new QCPCurve(m_speedPlot->xAxis, m_speedPlot->yAxis);
    m_speedCurve->setPen(QPen(Qt::blue, 2));
    m_speedPlot->xAxis->setLabel("数值"); m_speedPlot->yAxis->setLabel("距离 (m)");
    m_speedPlot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom);

    m_snrCurve = new QCPCurve(m_snrPlot->xAxis, m_snrPlot->yAxis);
    m_snrCurve->setPen(QPen(Qt::red, 2));
    m_snrPlot->xAxis->setLabel("SNR (dB)"); m_snrPlot->yAxis->setLabel("距离 (m)");
    m_snrPlot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom);

    hLayout->addWidget(m_speedPlot); hLayout->addWidget(m_snrPlot);
    vSplitter->addWidget(bottomArea);
    vSplitter->setSizes(QList<int>() << 500 << 300);

    mainLayout->addWidget(toolWidget);
    mainLayout->addWidget(vSplitter);

    statusBar()->setStyleSheet("background-color: #f0f0f0; border-top: 1px solid #ccc; color: black;");
    m_statusLabel = new QLabel("就绪");
    statusBar()->addWidget(m_statusLabel);

    // --- 信号连接 ---
    connect(btnLoad, &QPushButton::clicked, this, &MainWindow::loadFiles);
    connect(m_snrBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &MainWindow::updateFilter);
    connect(m_ppi, &PPIWidget::raySelected, this, &MainWindow::updateLinePlot);
    connect(m_comboMode, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::onModeChanged);
    connect(m_spinWinSize, QOverload<int>::of(&QSpinBox::valueChanged), this, &MainWindow::onWindowSizeChanged);
    connect(btnExp, &QPushButton::clicked, this, &MainWindow::onExportData);

    // 【修改点 3】距离控件双向绑定 (滑条 <-> SpinBox)
    // 最小距离同步
    connect(m_minSlider, &QSlider::valueChanged, m_minDistBox, &QSpinBox::setValue);
    connect(m_minDistBox, QOverload<int>::of(&QSpinBox::valueChanged), m_minSlider, &QSlider::setValue);
    connect(m_minSlider, &QSlider::valueChanged, this, &MainWindow::onRangeChanged);

    // 最大距离同步
    connect(m_maxSlider, &QSlider::valueChanged, m_maxDistBox, &QSpinBox::setValue);
    connect(m_maxDistBox, QOverload<int>::of(&QSpinBox::valueChanged), m_maxSlider, &QSlider::setValue);
    connect(m_maxSlider, &QSlider::valueChanged, this, &MainWindow::onRangeChanged);

    connect(btnShot, &QPushButton::clicked, [=](){
        QString p = QFileDialog::getSaveFileName(this, "截图", "Radar.png", "Images (*.png)");
        if(!p.isEmpty()) this->grab().save(p);
    });
}

// ... 保持 loadFiles, updateLinePlot, updateFilter, onModeChanged, onWindowSizeChanged, onExportData, updateStatusBar 等函数不变 ...
// 注意：onRangeChanged 函数需要稍微调整以适应 slider 的逻辑

void MainWindow::onRangeChanged() {
    double min = m_minDistBox->value();
    double max = m_maxDistBox->value();

    // 简单逻辑：防止 min > max
    if (min >= max) {
        if (sender() == m_minSlider || sender() == m_minDistBox) {
            m_minDistBox->setValue(max - 100);
        } else {
            m_maxDistBox->setValue(min + 100);
        }
        return;
    }

    m_ppi->setDistanceRange(min, max);
    m_speedPlot->yAxis->setRange(min, max);
    m_snrPlot->yAxis->setRange(min, max);
    m_speedPlot->replot();
    m_snrPlot->replot();
}

void MainWindow::updateStatusBar() {
    QString modeStr = (m_currentMode == Mode_Turbulence) ? "湍流强度" : "径向风速";
    QString text = QString("当前文件: %1  |  扫描模式: %2  |  数据行数: %3")
                       .arg(m_currentFileName)
                       .arg(modeStr)
                       .arg(m_manager.getScanData().size());
    m_statusLabel->setText(text);
}

void MainWindow::loadFiles() {
    QString a = QFileDialog::getOpenFileName(this, "选择角度文件", "", "CSV (*.csv)"); if(a.isEmpty()) return;
    QString w = QFileDialog::getOpenFileName(this, "选择风速文件", "", "CSV (*.csv)"); if(w.isEmpty()) return;
    m_currentFileName = QFileInfo(w).fileName();
    if (m_manager.loadData(a, w)) {
        m_manager.applyFilter(m_snrBox->value());
        m_manager.calculateTurbulence(m_spinWinSize->value());
        m_ppi->setData(&m_manager.getScanData());
        m_playIndex = 0; m_playTimer->start(25);
        updateLinePlot(m_manager.getScanData().size()/2);
        updateStatusBar();
    } else {
        QMessageBox::warning(this, "解析失败", "无法对齐时间戳");
    }
}

void MainWindow::updateLinePlot(int idx) {
    const ScanData& data = m_manager.getScanData();
    if (idx < 0 || idx >= data.size()) return;
    const RadarRay& ray = data[idx];
    QVector<double> dists, vals, snrs;
    for (const auto& g : ray.gates) {
        if (g.isValid) {
            dists << g.distance; snrs << g.snr;
            vals << (m_currentMode == Mode_Turbulence ? g.turbulence : g.speed);
        }
    }
    m_speedCurve->setData(vals, dists);
    m_speedPlot->rescaleAxes();
    m_speedPlot->yAxis->setRange(m_minDistBox->value(), m_maxDistBox->value()); // 遵循滑条范围
    m_speedPlot->replot();

    m_snrCurve->setData(snrs, dists);
    m_snrPlot->rescaleAxes();
    m_snrPlot->yAxis->setRange(m_minDistBox->value(), m_maxDistBox->value());
    m_snrPlot->replot();
}

void MainWindow::updateFilter(double val) {
    m_manager.applyFilter(val);
    m_manager.calculateTurbulence(m_spinWinSize->value());
    m_ppi->update();
    if(!m_manager.getScanData().isEmpty()) updateLinePlot(m_manager.getScanData().size()/2);
}

void MainWindow::onModeChanged(int) {
    m_currentMode = (DisplayMode)m_comboMode->currentData().toInt();
    m_ppi->setDisplayMode(m_currentMode);
    m_speedPlot->xAxis->setLabel(m_currentMode == Mode_Turbulence ? "湍流强度" : "风速 (m/s)");
    m_speedPlot->replot();
    if(!m_manager.getScanData().isEmpty()) updateLinePlot(m_manager.getScanData().size()/2);
    updateStatusBar();
}

void MainWindow::onWindowSizeChanged(int v) {
    m_manager.calculateTurbulence(v);
    m_ppi->update();
    if(!m_manager.getScanData().isEmpty()) updateLinePlot(m_manager.getScanData().size()/2);
}

void MainWindow::onExportData() {
    QString p = QFileDialog::getSaveFileName(this, "保存", "radar.csv", "CSV (*.csv)");
    if (!p.isEmpty()) { m_manager.calculateTurbulence(m_spinWinSize->value()); m_manager.exportToCSV(p); }
}
