#pragma once

#include "gui/radio_settings.h"

#include <QObject>
#include <QVector>

#include <atomic>

class SpectrumWorker : public QObject
{
    Q_OBJECT

public:
    explicit SpectrumWorker(
        const RadioSettings& settings,
        QObject* parent = nullptr
    );

    void requestStop();

public slots:
    void run();

signals:
    void spectrumReady(
        QVector<double> frequencyKhz,
        QVector<double> magnitudeDbfs,
        double peakFrequencyKhz,
        double peakLevelDbfs,
        double noiseFloorDbfs
    );
    
    void temperatureChanged(double temperatureCelsius);
    void statusChanged(const QString& status);
    void errorOccurred(const QString& error);
    void finished();

private:
    RadioSettings settings_;
    std::atomic<bool> stopRequested_{false};
};