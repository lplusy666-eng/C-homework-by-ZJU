#include "ppiwidget.h"
#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QtMath>
#include <QToolTip>

PPIWidget::PPIWidget(QWidget *parent) : QWidget(parent) {
    // 【关键1】必须开启鼠标追踪，否则不按键时无法触发悬停事件
    setMouseTracking(true);
    m_scale = 0.8;
    m_offset = QPointF(0, 0);
    // 确保能正常显示背景色
    setAttribute(Qt::WA_StyledBackground, true);
}

void PPIWidget::setData(const ScanData *data) {
    m_data = data;
    m_playLimit = -1;
    update();
}

void PPIWidget::setDisplayMode(DisplayMode mode) {
    m_mode = mode;
    update();
}

void PPIWidget::setPlayLimit(int limit) {
    m_playLimit = limit;
    update();
}

void PPIWidget::setDistanceRange(double min, double max) {
    m_minVisDist = min;
    m_maxVisDist = max;
    update();
}

// 核心坐标转换：极坐标(雷达) -> 屏幕坐标(像素)
QPointF PPIWidget::polarToScreen(double azimuth, double distance, QPointF center) {
    // 角度：雷达0度 = 屏幕东偏南15度 (Qt 0度是东)
    double angleRad = qDegreesToRadians(azimuth + 15.0);

    // 比例尺：4000米映射到窗口半径的 1/2.2
    double baseRadius = qMin(width(), height()) / 2.2;
    double pxPerM = (baseRadius / 4000.0) * m_scale;

    double r = distance * pxPerM;
    double x = center.x() + m_offset.x() + r * qCos(angleRad);
    double y = center.y() + m_offset.y() + r * qSin(angleRad);
    return QPointF(x, y);
}

QColor PPIWidget::valueToColor(double val) {
    if (m_mode == Mode_Turbulence) {
        double norm = qBound(0.0, val / 0.5, 1.0);
        return QColor::fromHsv(120 * (1.0 - norm), 255, 255, 220);
    } else {
        double norm = (val + 10.0) / 20.0;
        norm = qBound(0.0, norm, 1.0);
        return QColor::fromHsv(240 * (1.0 - norm), 255, 255, 220);
    }
}

void PPIWidget::paintEvent(QPaintEvent *) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    // 1. 背景设为白色
    p.fillRect(rect(), Qt::white);

    if (!m_data || m_data->isEmpty()) {
        p.setPen(Qt::gray);
        p.drawText(rect(), Qt::AlignCenter, "请点击[导入]并选择文件");
        return;
    }

    QPointF center = rect().center();
    int limit = (m_playLimit == -1) ? m_data->size() : qMin(m_playLimit, m_data->size());

    // 2. 绘制热力图 (使用清晰的 Polygon 方式)
    for (int i = 0; i < limit; ++i) {
        const RadarRay& ray = m_data->at(i);
        double az1 = ray.azimuth;
        double az2 = az1 + 1.2; // 略微加宽以消除缝隙

        for (int j = 0; j < ray.gates.size() - 1; ++j) {
            const RangeGate& g = ray.gates[j];

            // 距离范围过滤
            if (g.distance < m_minVisDist || g.distance > m_maxVisDist) continue;

            if (!g.isValid) {
                // 无效数据画浅灰
                p.setBrush(QColor(240, 240, 240));
            } else {
                p.setBrush(valueToColor((m_mode == Mode_Turbulence) ? g.turbulence : g.speed));
            }

            // 计算四个顶点
            QPointF p1 = polarToScreen(az1, g.distance, center);
            QPointF p2 = polarToScreen(az1, ray.gates[j+1].distance, center);
            QPointF p3 = polarToScreen(az2, ray.gates[j+1].distance, center);
            QPointF p4 = polarToScreen(az2, g.distance, center);

            QPolygonF poly;
            poly << p1 << p2 << p3 << p4;

            p.setPen(Qt::NoPen);
            p.drawPolygon(poly);
        }
    }

    // 3. 绘制距离刻度圈
    p.setPen(QPen(Qt::lightGray, 1, Qt::DashLine));
    p.setBrush(Qt::NoBrush);

    // 统一比例尺计算
    double baseRadius = qMin(width(), height()) / 2.2;
    double pxPerM = (baseRadius / 4000.0) * m_scale;

    for (int r = 1000; r <= 4000; r += 1000) {
        // 只画在范围内的圈
        if (r >= m_minVisDist && r <= m_maxVisDist) {
            double radius = r * pxPerM;
            // 圈的位置要加上偏移量
            p.drawEllipse(center + m_offset, radius, radius);
            p.drawText(center + m_offset + QPointF(radius + 5, 0), QString::number(r) + "m");
        }
    }

    // 4. 绘制图例
    drawLegend(p);
}

void PPIWidget::drawLegend(QPainter &p) {
    int w = 15, h = 160, m = 40;
    QRect r(width() - w - m, height() - h - m, w, h);

    p.setPen(Qt::NoPen);
    p.setBrush(QColor(255, 255, 255, 220));
    p.drawRect(r.adjusted(-10, -30, 50, 10));

    for (int i = 0; i < h; ++i) {
        double v = (m_mode == Mode_Turbulence) ? (double)(h-i)/h*0.5 : (double)(h-i)/h*20-10;
        p.setPen(valueToColor(v));
        p.drawLine(r.left(), r.top() + i, r.right(), r.top() + i);
    }
    p.setPen(Qt::black);
    p.drawRect(r);
    p.drawText(r.right() + 5, r.top() + 10, (m_mode == Mode_Turbulence ? "0.5" : "+10"));
    p.drawText(r.right() + 5, r.bottom(), (m_mode == Mode_Turbulence ? "0.0" : "-10"));
    p.drawText(r.left() - 5, r.top() - 10, (m_mode == Mode_Turbulence ? "湍流" : "风速"));
}

// --- 交互事件实现 ---

void PPIWidget::mousePressEvent(QMouseEvent *e) {
    if (e->button() == Qt::LeftButton) {
        m_isDragging = true;
        m_lastMousePos = e->pos();
        setCursor(Qt::ClosedHandCursor);
    }
    // 发送信号给折线图
    if (m_data && !m_data->isEmpty()) emit raySelected(m_data->size()/2);
}

void PPIWidget::mouseReleaseEvent(QMouseEvent *e) {
    if (e->button() == Qt::LeftButton) {
        m_isDragging = false;
        unsetCursor();
    }
}

void PPIWidget::mouseDoubleClickEvent(QMouseEvent *) {
    // 双击复位功能
    m_scale = 0.8;
    m_offset = {0, 0};
    update();
}

void PPIWidget::wheelEvent(QWheelEvent *e) {
    QPointF mousePos = e->position();
    QPointF center = rect().center();
    QPointF pRel = mousePos - center;

    double factor = (e->angleDelta().y() > 0) ? 1.15 : 0.85;
    double oldS = m_scale;
    m_scale = qBound(0.1, m_scale * factor, 30.0);
    double actualF = m_scale / oldS;

    // 中心缩放补偿
    m_offset = pRel - (pRel - m_offset) * actualF;
    update();
}

// 【核心修复】鼠标移动事件：反算坐标显示 ToolTip
void PPIWidget::mouseMoveEvent(QMouseEvent *e) {
    // 1. 处理拖拽平移
    if (m_isDragging) {
        QPoint delta = e->pos() - m_lastMousePos;
        m_offset += QPointF(delta.x(), delta.y());
        m_lastMousePos = e->pos();
        update();
        return;
    }

    // 2. 处理悬停提示
    if (!m_data || m_data->isEmpty()) return;

    QPointF center = rect().center();

    // 关键步骤A：反算鼠标相对于圆心的偏移量（必须减去平移量 m_offset）
    double dx = e->position().x() - center.x() - m_offset.x();
    double dy = e->position().y() - center.y() - m_offset.y();

    // 关键步骤B：计算比例尺（必须与 paintEvent 中的公式一模一样）
    double baseRadius = qMin(width(), height()) / 2.2;
    double pxPerM = (baseRadius / 4000.0) * m_scale;

    // 反算出实际距离
    double dist_m = std::sqrt(dx*dx + dy*dy) / pxPerM;

    // 关键步骤C：反算出方位角
    // 绘图公式：angle = az + 15
    // 逆向公式：az = angle - 15
    double angleRad = std::atan2(dy, dx);
    double angleDeg = qRadiansToDegrees(angleRad);
    double az = angleDeg - 15.0;

    // 归一化到 0~360
    while (az < 0) az += 360;
    while (az >= 360) az -= 360;

    // 3. 在数据中查找
    QString info;
    const RadarRay* bestRay = nullptr;
    double minAzDiff = 100.0;

    // 找最近的射线
    for (const auto& ray : *m_data) {
        double diff = std::abs(ray.azimuth - az);
        if (diff > 180) diff = 360 - diff;
        if (diff < minAzDiff) {
            minAzDiff = diff;
            bestRay = &ray;
        }
    }

    // 阈值判定：鼠标必须指在有效数据范围内 (角度偏差<2度，距离<4000)
    if (bestRay && minAzDiff < 2.0 && dist_m <= 4000) {
        // 找最近的距离门
        for (const auto& gate : bestRay->gates) {
            // 距离容差设为 30米
            if (std::abs(gate.distance - dist_m) < 30.0) {
                info = QString("方位: %1°\n距离: %2 m\n风速: %3 m/s\nSNR: %4\n湍流: %5")
                           .arg(bestRay->azimuth, 0, 'f', 1)
                           .arg(gate.distance, 0, 'f', 0)
                           .arg(gate.speed, 0, 'f', 2)
                           .arg(gate.snr, 0, 'f', 1)
                           .arg(gate.turbulence, 0, 'f', 3);
                break;
            }
        }
    }

    if (!info.isEmpty()) {
        QToolTip::showText(e->globalPosition().toPoint(), info, this);
    } else {
        QToolTip::hideText(); // 移出有效区隐藏
    }
}
