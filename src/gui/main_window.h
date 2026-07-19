#pragma once

#include <QMainWindow>

class QLabel;
class QPushButton;
class QCloseEvent;
class QDoubleSpinBox;
class QThread;

namespace QtCharts
{
class QLineSeries;
class QChart;
class QChartView;
class QValueAxis;
}

class SpectrumWorker;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

protected:
    void closeEvent(QCloseEvent* event) override;

private slots:
    void updateSpectrum(
        const QVector<double>& frequencyKhz,
        const QVector<double>& magnitudeDbfs,
        double peakFrequencyKhz,
        double peakLevelDbfs,
        double noiseFloorDbfs
    );

    void showError(const QString& error);
    void startStreaming();
    void stopStreaming();

private:
    QtCharts::QLineSeries* spectrumSeries_{nullptr};
    //QtCharts::QLineSeries* expectedToneSeries_{nullptr};

    QtCharts::QChart* chart_{nullptr};
    QtCharts::QChartView* chartView_{nullptr};

    QtCharts::QValueAxis* frequencyAxis_{nullptr};
    QtCharts::QValueAxis* magnitudeAxis_{nullptr};

    QLabel* statusLabel_{nullptr};
    QLabel* peakLabel_{nullptr};
    QLabel* noiseLabel_{nullptr};
    QLabel* snrLabel_{nullptr};
    QLabel* tempLabel_{nullptr};

    QDoubleSpinBox* centerFrequencyInput_{nullptr};
    QDoubleSpinBox* toneFrequencyInput_{nullptr};
    QDoubleSpinBox* txGainInput_{nullptr};
    QDoubleSpinBox* rxGainInput_{nullptr};

    QPushButton* startButton_{nullptr};
    QPushButton* stopButton_{nullptr};

    QThread* workerThread_{nullptr};
    SpectrumWorker* worker_{nullptr};

    bool stopping_{false};
};