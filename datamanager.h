#ifndef DATAMANAGER_H
#define DATAMANAGER_H

#include "datatypes.h"
#include <QString>
#include <QFile>
#include <QTextStream>
#include <QDebug>

class DataManager
{
public:
    DataManager();

    // 加载数据（含智能对齐与解析）
    bool loadData(const QString& anglePath, const QString& windPath);

    // 获取数据引用
    const ScanData& getScanData() const;

    // 1. SNR 阈值过滤
    void applyFilter(double snrThreshold);

    // 2. 湍流强度计算 (滑动窗口)
    void calculateTurbulence(int windowSize);

    // 3. 数据导出
    bool exportToCSV(const QString& filePath);

    void detectAndRepairOutliers(double diffThreshold);

private:
    ScanData m_rawData;       // 原始对齐数据
    ScanData m_processedData; // 经过过滤/计算后的展示数据
};

#endif // DATAMANAGER_H
