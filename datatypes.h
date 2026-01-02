#ifndef DATATYPES_H
#define DATATYPES_H

#include <QVector>
#include <QDateTime>

struct RangeGate {
    double distance;   // 距离 (m)
    double speed;      // 径向风速 (m/s)
    double snr;        // 信噪比 (dB)
    double turbulence; // 湍流强度
    bool isValid;      // 是否有效
};

struct RadarRay {
    QDateTime timestamp;
    double azimuth;
    double elevation;
    QVector<RangeGate> gates;
};

typedef QVector<RadarRay> ScanData;

enum DisplayMode {
    Mode_Speed,
    Mode_Turbulence
};

#endif // DATATYPES_H
