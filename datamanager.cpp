#include "datamanager.h"
#include <cmath>
#include <QRegularExpression>
#include <QMap>
#include <QDateTime>
#include <QDebug>

DataManager::DataManager() {}

// ---------------------------------------------------------
// 核心逻辑：数据加载与时间对齐
// ---------------------------------------------------------
bool DataManager::loadData(const QString &anglePath, const QString &windPath)
{
    m_rawData.clear();

    // Key: 时间戳(秒), Value: <方位角, 仰角>
    QMap<qint64, QPair<double, double>> angleMap;

    qDebug() << "--- [第一步] 读取角度文件 (格式: 2025-11-18) ---";

    QFile fileA(anglePath);
    if (fileA.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&fileA);
        // 自动识别编码（兼容 UTF-8 和 GBK）
        in.setAutoDetectUnicode(true);

        int lineCount = 0;
        while (!in.atEnd()) {
            QString line = in.readLine().trimmed();
            if (line.isEmpty() || line.contains("时间") || line.contains("方位")) continue;

            // 分割：空格、逗号、制表符
            QStringList parts = line.split(QRegularExpression("[,\\s\\t]+"), Qt::SkipEmptyParts);

            // 角度文件格式: 2025-11-18 13:01:22 0 5 (至少4列)
            if (parts.size() >= 4) {
                // 拼接日期和时间
                QString timeStr = parts[0] + " " + parts[1];
                // 格式：yyyy-MM-dd HH:mm:ss
                QDateTime dt = QDateTime::fromString(timeStr, "yyyy-MM-dd HH:mm:ss");

                if (dt.isValid()) {
                    bool okAz, okEl;
                    double az = parts[2].toDouble(&okAz);
                    double el = parts[3].toDouble(&okEl);
                    if (okAz && okEl) {
                        angleMap.insert(dt.toSecsSinceEpoch(), qMakePair(az, el));
                    }
                }
            }
            lineCount++;
        }
        fileA.close();
    }
    qDebug() << ">>> 角度数据加载完成，有效点数：" << angleMap.size();

    if (!angleMap.isEmpty()) {
        qDebug() << "角度时间范围:"
                 << QDateTime::fromSecsSinceEpoch(angleMap.firstKey()).toString()
                 << " -> "
                 << QDateTime::fromSecsSinceEpoch(angleMap.lastKey()).toString();
    } else {
        qDebug() << "错误：角度文件解析失败，请检查格式！";
        return false;
    }

    qDebug() << "--- [第二步] 读取风速文件 (格式: 20251118) 并对齐 ---";

    QFile fileW(windPath);
    if (!fileW.open(QIODevice::ReadOnly | QIODevice::Text)) return false;

    QTextStream inW(&fileW);
    inW.setAutoDetectUnicode(true);

    // 1. 解析表头 (找距离门)
    QString header = inW.readLine();
    QStringList hParts = header.split(QRegularExpression("[,\\s\\t]+"), Qt::SkipEmptyParts);
    QVector<double> dists;
    for (const QString& h : hParts) {
        if (h.contains(QRegularExpression("\\d+m"))) {
            QString n; for(QChar c:h) if(c.isDigit()) n+=c;
            if(!n.isEmpty() && !dists.contains(n.toDouble())) dists.append(n.toDouble());
        }
    }
    qDebug() << ">>> 解析出距离门数量：" << dists.size();

    int matchCount = 0;
    int lineCount = 0;

    // 2. 逐行读取风速数据
    while (!inW.atEnd()) {
        QString line = inW.readLine().trimmed();
        if (line.isEmpty() || line.contains("时间")) continue;
        lineCount++;

        QStringList parts = line.split(QRegularExpression("[,\\s\\t]+"), Qt::SkipEmptyParts);

        // 风速文件至少要有日期、时间、和一堆数据
        if (parts.size() < 3) continue;

        // 【核心修改】针对 20251118 13:01:15 格式的解析
        QString datePart = parts[0]; // 20251118
        QString timePart = parts[1]; // 13:01:15
        QString combined = datePart + " " + timePart;

        // 使用 yyyyMMdd HH:mm:ss 格式解析
        QDateTime windDt = QDateTime::fromString(combined, "yyyyMMdd HH:mm:ss");

        // 调试：如果第一行解析失败，打印出来看原因
        if (!windDt.isValid() && lineCount == 1) {
            qDebug() << "错误：风速首行时间解析失败！原始内容:" << combined;
            continue;
        }
        if (!windDt.isValid()) continue;

        // --- [第三步] 时间对齐算法 (最近邻搜索) ---
        qint64 tWind = windDt.toSecsSinceEpoch();

        // 在角度Map中查找
        auto it = angleMap.lowerBound(tWind);
        bool found = false;
        QPair<double, double> bestAngles;
        double minDiff = 10000.0;

        // 检查当前点 (>= tWind)
        if (it != angleMap.end()) {
            double diff = std::abs(it.key() - tWind);
            if (diff < minDiff) { minDiff = diff; bestAngles = it.value(); }
        }
        // 检查前一个点 (< tWind)
        if (it != angleMap.begin()) {
            auto prev = std::prev(it);
            double diff = std::abs(prev.key() - tWind);
            if (diff < minDiff) { minDiff = diff; bestAngles = prev.value(); }
        }

        // 容差判断：只要误差在 3秒 之内，就算对齐成功
        // 因为两个文件时间戳不一致，这是必须的步骤
        if (minDiff <= 3.0) {
            found = true;
        }

        if (found) {
            RadarRay ray;
            ray.timestamp = windDt;
            ray.azimuth = bestAngles.first;
            ray.elevation = bestAngles.second;

            // 自动推断数据起始列：
            // 假设 Date Time 占了 2 列，后面就是数据
            // 如果数据列数不对，这里做个保护
            int col = parts.size() - dists.size() * 2;
            if (col < 2) col = 2; // 默认跳过前两列

            for (double d : dists) {
                if (col + 1 >= parts.size()) break;
                RangeGate g;
                g.distance = d;
                g.speed = parts[col].toDouble();
                g.snr = parts[col+1].toDouble();
                g.turbulence = 0.0; // 初始化
                g.isValid = true;

                ray.gates.append(g);
                col += 2;
            }
            m_rawData.append(ray);
            matchCount++;
        }
    }
    fileW.close();

    qDebug() << ">>> 对齐完成！共生成射线数：" << matchCount;
    qDebug() << "    (如果此数字为0，说明两个文件时间差全部超过了3秒)";

    m_processedData = m_rawData;
    return !m_rawData.isEmpty();
}

// ---------------------------------------------------------
// 其他功能函数 (保持不变，确保链接通过)
// ---------------------------------------------------------
const ScanData& DataManager::getScanData() const { return m_processedData; }

void DataManager::applyFilter(double snrThreshold) {
    m_processedData = m_rawData;
    for (auto &r : m_processedData) {
        for (auto &g : r.gates) {
            if (g.snr < snrThreshold) g.isValid = false;
        }
    }
}

void DataManager::calculateTurbulence(int windowSize) {
    if (windowSize < 2) windowSize = 2;
    int halfWin = windowSize / 2;
    for (auto &ray : m_processedData) {
        int cnt = ray.gates.size();
        QVector<double> ti(cnt, 0.0);
        for (int i = 0; i < cnt; ++i) {
            if (!ray.gates[i].isValid) continue;
            int start = std::max(0, i - halfWin);
            int end = std::min(cnt - 1, i + halfWin);
            double sum = 0, ss = 0; int n = 0;
            for (int k = start; k <= end; ++k) {
                if (ray.gates[k].isValid) { sum += ray.gates[k].speed; n++; }
            }
            if (n < 2) continue;
            double mean = sum / n;
            for (int k = start; k <= end; ++k) {
                if (ray.gates[k].isValid) ss += std::pow(ray.gates[k].speed - mean, 2);
            }
            ti[i] = (std::abs(mean) > 0.01) ? (std::sqrt(ss/n) / std::abs(mean)) : 0.0;
        }
        for (int i = 0; i < cnt; ++i) ray.gates[i].turbulence = ti[i];
    }
}

bool DataManager::exportToCSV(const QString &filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) return false;
    QTextStream out(&file);
    out.setGenerateByteOrderMark(true); // UTF-8 BOM
    out << "Time,Azimuth,Elevation,Distance,Speed,SNR,TI\n";
    for (const auto& r : m_processedData) {
        QString ts = r.timestamp.toString("yyyy-MM-dd HH:mm:ss");
        for (const auto& g : r.gates) {
            if (g.isValid) {
                out << ts << "," << r.azimuth << "," << r.elevation << ","
                    << g.distance << "," << g.speed << "," << g.snr << "," << g.turbulence << "\n";
            }
        }
    }
    return true;
}
