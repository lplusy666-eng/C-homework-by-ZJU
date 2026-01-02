#ifndef PPIWIDGET_H
#define PPIWIDGET_H

#include <QWidget>
#include "datatypes.h"

class PPIWidget : public QWidget {
    Q_OBJECT
public:
    explicit PPIWidget(QWidget *parent = nullptr);
    void setData(const ScanData* data);
    void setDisplayMode(DisplayMode mode);
    void setPlayLimit(int limit);
     void setDistanceRange(double min, double max);

signals:
    void raySelected(int rayIndex);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
     void mouseDoubleClickEvent(QMouseEvent *event) override;

private:
    void drawLegend(QPainter &p); // 新增：绘制图例
    QColor valueToColor(double val);
    QPointF polarToScreen(double azimuth, double distance, QPointF center);

    const ScanData* m_data = nullptr;
    DisplayMode m_mode = Mode_Speed;
    int m_playLimit = -1;
    double m_scale = 0.8;
    QPointF m_offset = QPointF(0, 0);
    QPoint m_lastMousePos;
    bool m_isDragging = false;
    double m_minVisDist = 0.0;
    double m_maxVisDist = 10000.0;
};

#endif // PPIWIDGET_H
