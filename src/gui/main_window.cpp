#include "main_window.h"

#include "radio_settings.h"
#include "spectrum_worker.h"
#include "waterfall_widget.h"

#include <QtCharts/QChart>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>
#include <QComboBox>

#include <QCloseEvent>
#include <QDoubleSpinBox>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPainter>
#include <QPointF>
#include <QPushButton>
#include <QThread>
#include <QVector>
#include <QWidget>

#include <algorithm>
#include <cmath>

QT_CHARTS_USE_NAMESPACE


MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {

    setWindowTitle("Radom — Live Spectrum");
    resize(1200, 800);

    auto* centralWidget = new QWidget(this);
    auto* mainLayout = new QGridLayout(centralWidget);

    setCentralWidget(centralWidget);

    // Radio Controls
    centerFrequencyInput_ = new QDoubleSpinBox(this);
    centerFrequencyInput_->setRange(10.0, 3500.0);
    centerFrequencyInput_->setDecimals(3);
    centerFrequencyInput_->setSingleStep(1.0);
    centerFrequencyInput_->setSuffix(" MHz");
    centerFrequencyInput_->setValue(915.0);

    toneFrequencyInput_ = new QDoubleSpinBox(this);
    toneFrequencyInput_->setRange(-2400.0, 2400.0);
    toneFrequencyInput_->setDecimals(1);
    toneFrequencyInput_->setSingleStep(10.0);
    toneFrequencyInput_->setSuffix(" kHz");
    toneFrequencyInput_->setValue(400.0);

    txGainInput_ = new QDoubleSpinBox(this);
    txGainInput_->setRange(0.0, 60.0);
    txGainInput_->setDecimals(1);
    txGainInput_->setSingleStep(1.0);
    txGainInput_->setSuffix(" dB");
    txGainInput_->setValue(10.0);

    rxGainInput_ = new QDoubleSpinBox(this);
    rxGainInput_->setRange(0.0, 70.0);
    rxGainInput_->setDecimals(1);
    rxGainInput_->setSingleStep(1.0);
    rxGainInput_->setSuffix(" dB");
    rxGainInput_->setValue(30.0);

    startButton_ = new QPushButton("Start", this);
    stopButton_ = new QPushButton("Stop", this);

    waveformInput_ = new QComboBox(this);

    waveformInput_->addItem("Tone");
    waveformInput_->addItem("Continuous chirp");
    waveformInput_->addItem("Pulsed chirp");

    

    stopButton_->setEnabled(false);

    auto* controlsLayout = new QHBoxLayout();

    controlsLayout->addWidget(
        new QLabel("Center frequency:", this)
    );
    controlsLayout->addWidget(centerFrequencyInput_);

    controlsLayout->addSpacing(12);

    controlsLayout->addWidget(new QLabel("Tone offset:", this));
    controlsLayout->addWidget(toneFrequencyInput_);

    controlsLayout->addSpacing(12);

    controlsLayout->addWidget(new QLabel("TX gain:", this));
    controlsLayout->addWidget(txGainInput_);

    controlsLayout->addSpacing(12);

    controlsLayout->addWidget(new QLabel("RX gain:", this));
    controlsLayout->addWidget(rxGainInput_);

    controlsLayout->addSpacing(12);
    controlsLayout->addWidget(new QLabel("Waveform: ", this));
    controlsLayout->addWidget(waveformInput_);

    controlsLayout->addStretch();

    controlsLayout->addWidget(startButton_);
    controlsLayout->addWidget(stopButton_);
    

    // Spectrum Chart
    spectrumSeries_ = new QLineSeries(this);
    spectrumSeries_->setName("RX Spectrum");

    chart_ = new QChart();
    chart_->setTitle(
        "LimeSDR Mini Live TX-to-RX Spectrum"
    );

    chart_->addSeries(spectrumSeries_);

    waterfallWidget_ = new WaterfallWidget(this);

    waterfallWidget_->setPowerRange(
        -120.0,
        -60.0
    );

    frequencyAxis_ = new QValueAxis(this);
    frequencyAxis_->setTitleText(
        "Baseband frequency (kHz)"
    );
    frequencyAxis_->setRange(-1000.0, 1000.0);
    frequencyAxis_->setTickCount(9);
    frequencyAxis_->setLabelFormat("%.0f");

    magnitudeAxis_ = new QValueAxis(this);
    magnitudeAxis_->setTitleText(
        "Magnitude (dBFS)"
    );
    magnitudeAxis_->setRange(-130.0, -50.0);
    magnitudeAxis_->setTickCount(9);
    magnitudeAxis_->setLabelFormat("%.0f");

    chart_->addAxis(
        frequencyAxis_,
        Qt::AlignBottom
    );

    chart_->addAxis(
        magnitudeAxis_,
        Qt::AlignLeft
    );

    spectrumSeries_->attachAxis(
        frequencyAxis_
    );

    spectrumSeries_->attachAxis(
        magnitudeAxis_
    );

    chartView_ = new QChartView(
        chart_,
        this
    );

    chartView_->setRenderHint(
        QPainter::Antialiasing
    );

    // Status Labels
    statusLabel_ = new QLabel(
        "Stopped",
        this
    );

    peakLabel_ = new QLabel(
        "Peak: --",
        this
    );

    noiseLabel_ = new QLabel(
        "Noise: --",
        this
    );

    snrLabel_ = new QLabel(
        "Peak above noise: --",
        this
    );

    tempLabel_ = new QLabel(
        "Current Temp: -- °C",
        this
    );



    // Main Labels
    mainLayout->addLayout(
        controlsLayout,
        0,
        0,
        1,
        5
    );

    mainLayout->addWidget(
        chartView_,
        1,
        0,
        1,
        5
    );

    mainLayout->addWidget(
        statusLabel_,
        2,
        0
    );

    mainLayout->addWidget(
        peakLabel_,
        2,
        1
    );

    mainLayout->addWidget(
        noiseLabel_,
        2,
        2
    );

    mainLayout->addWidget(
        snrLabel_,
        2,
        3
    );

    mainLayout->addWidget(
        tempLabel_,
        2,
        4
    );

    mainLayout->addWidget(
        waterfallWidget_,
        3,
        0,
        1,
        5
    );


    mainLayout->setRowStretch(1, 2);  // spectrum
    mainLayout->setRowStretch(2, 0);  // status labels
    mainLayout->setRowStretch(3, 1);  // waterfall

    mainLayout->setVerticalSpacing(6);
    mainLayout->setContentsMargins(6, 6, 6, 6);

    // Signal Connections
    connect(
        startButton_,
        &QPushButton::clicked,
        this,
        &MainWindow::startStreaming
    );

    connect(
        stopButton_,
        &QPushButton::clicked,
        this,
        &MainWindow::stopStreaming
    );
}


MainWindow::~MainWindow()
{
    if (worker_ != nullptr)
    {
        worker_->requestStop();
    }

    if (workerThread_ != nullptr)
    {
        if (workerThread_->isRunning())
        {
            workerThread_->quit();
            workerThread_->wait(3000);
        }
    }
}


void MainWindow::startStreaming()
{
    if (workerThread_ != nullptr)
    {
        return;
    }

    RadioSettings settings;

    settings.centerFrequencyHz =
        centerFrequencyInput_->value() * 1e6;

    settings.toneFrequencyHz =
        toneFrequencyInput_->value() * 1e3;

    settings.txGainDb =
        txGainInput_->value();

    settings.rxGainDb =
        rxGainInput_->value();


    const QString waveform = waveformInput_->currentText();
    if (waveform == "Pulsed chirp") {
        settings.mode = 0;
    } else if (waveform == "Tone") {
        settings.mode = 1;
    } else {
        settings.mode = 2;
    }

    /*
     * The tone frequency must remain within the sample-rate
     * Nyquist range.
     */
    if (
        std::abs(settings.toneFrequencyHz) >=
        settings.sampleRateHz / 2.0
    )
    {
        QMessageBox::warning(
            this,
            "Invalid tone frequency",
            "The tone offset must remain inside "
            "the ±2.5 MHz Nyquist range."
        );

        return;
    }

    const double toneKhz =
        settings.toneFrequencyHz / 1e3;

    waterfallWidget_->clear();

    /*
     * Create a fresh worker and thread each time Start is
     * pressed.
     */
    workerThread_ = new QThread(this);

    worker_ = new SpectrumWorker(
        settings
    );

    worker_->moveToThread(
        workerThread_
    );

    connect(
        workerThread_,
        &QThread::started,
        worker_,
        &SpectrumWorker::run
    );

    connect(
        worker_,
        &SpectrumWorker::spectrumReady,
        this,
        &MainWindow::updateSpectrum
    );

    connect(
        worker_,
        &SpectrumWorker::statusChanged,
        statusLabel_,
        &QLabel::setText
    );

    connect(
        worker_,
        &SpectrumWorker::errorOccurred,
        this,
        &MainWindow::showError
    );

    connect(
        worker_,
        &SpectrumWorker::finished,
        workerThread_,
        &QThread::quit
    );

    connect(
        worker_,
        &SpectrumWorker::finished,
        worker_,
        &QObject::deleteLater
    );

    connect(
        workerThread_,
        &QThread::finished,
        this,
        [this]()
        {
            worker_ = nullptr;

            if (workerThread_ != nullptr)
            {
                workerThread_->deleteLater();
                workerThread_ = nullptr;
            }

            startButton_->setEnabled(true);
            stopButton_->setEnabled(false);

            centerFrequencyInput_->setEnabled(true);
            toneFrequencyInput_->setEnabled(true);
            txGainInput_->setEnabled(true);
            rxGainInput_->setEnabled(true);

            statusLabel_->setText("Stopped");
            stopping_ = false;
        }
    );

    connect(
        worker_,
        &SpectrumWorker::temperatureChanged,
        this,
        [this](double temperatureCelsius)
        {
            tempLabel_->setText(
                QString("Temperature: %1 °C")
                    .arg(
                        temperatureCelsius,
                        0,
                        'f',
                        1
                    )
            );
        }
    );

    startButton_->setEnabled(false);
    stopButton_->setEnabled(true);

    /*
     * For now, settings can only be changed while the radio
     * is stopped. Later we can add live retuning.
     */
    centerFrequencyInput_->setEnabled(false);
    toneFrequencyInput_->setEnabled(false);
    txGainInput_->setEnabled(false);
    rxGainInput_->setEnabled(false);

    peakLabel_->setText("Peak: --");
    noiseLabel_->setText("Noise: --");
    snrLabel_->setText("Peak above noise: --");
    tempLabel_->setText("Temperature: -- °C");

    statusLabel_->setText("Starting...");
    stopping_ = false;

    workerThread_->start();
}


void MainWindow::stopStreaming()
{
    if (
        workerThread_ == nullptr ||
        stopping_
    )
    {
        return;
    }

    stopping_ = true;

    stopButton_->setEnabled(false);
    statusLabel_->setText("Stopping...");

    /*
     * requestStop() only changes an atomic flag, so it can be
     * safely called from the GUI thread.
     */
    if (worker_ != nullptr)
    {
        worker_->requestStop();
    }

    /*
     * Do not call workerThread_->quit() here immediately.
     * SpectrumWorker::run() owns the streaming loop and emits
     * finished after it exits cleanly.
     */
}


void MainWindow::updateSpectrum(
    const QVector<double>& frequencyKhz,
    const QVector<double>& magnitudeDbfs,
    double peakFrequencyKhz,
    double peakLevelDbfs,
    double noiseFloorDbfs
)
{
    const int count = std::min(
        frequencyKhz.size(),
        magnitudeDbfs.size()
    );

    QVector<QPointF> points;
    points.reserve(count);

    for (
        int index = 0;
        index < count;
        ++index
    )
    {
        points.append(
            QPointF(
                frequencyKhz[index],
                magnitudeDbfs[index]
            )
        );
    }

    spectrumSeries_->replace(points);

    const QRectF plotArea = chart_->plotArea();

    const int leftMargin = static_cast<int>(
        std::round(plotArea.left())
    );

    const int rightMargin = static_cast<int>(
        std::round(
            chartView_->width() -
            plotArea.right()
        )
    );

    waterfallWidget_->setHorizontalMargins(
        leftMargin,
        rightMargin
    );

    waterfallWidget_->addSpectrum(
        magnitudeDbfs
    );

    const double peakAboveNoise =
        peakLevelDbfs - noiseFloorDbfs;

    peakLabel_->setText(
        QString(
            "Peak: %1 kHz at %2 dBFS"
        )
            .arg(
                peakFrequencyKhz,
                0,
                'f',
                1
            )
            .arg(
                peakLevelDbfs,
                0,
                'f',
                1
            )
    );

    noiseLabel_->setText(
        QString("Noise: %1 dBFS")
            .arg(
                noiseFloorDbfs,
                0,
                'f',
                1
            )
    );

    snrLabel_->setText(
        QString(
            "Peak above noise: %1 dB"
        )
            .arg(
                peakAboveNoise,
                0,
                'f',
                1
            )
    );
}


void MainWindow::showError(
    const QString& error
)
{
    statusLabel_->setText(
        "Radio error"
    );

    QMessageBox::critical(
        this,
        "Radom error",
        error
    );

    /*
     * The worker should emit finished after reporting the
     * error. Keep the controls disabled until the thread has
     * actually exited.
     */
}


void MainWindow::closeEvent(
    QCloseEvent* event
)
{
    if (worker_ != nullptr)
    {
        worker_->requestStop();
    }

    if (workerThread_ != nullptr)
    {
        if (workerThread_->isRunning())
        {
            workerThread_->quit();

            if (!workerThread_->wait(3000))
            {
                /*
                 * The Lime stream can take up to its current
                 * read timeout to return.
                 */
                workerThread_->wait();
            }
        }
    }

    event->accept();
}