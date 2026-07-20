#pragma once

#include <QImage>
#include <QVector>
#include <QWidget>

class WaterfallWidget : public QWidget
{
    Q_OBJECT

public:
    explicit WaterfallWidget(
        QWidget* parent = nullptr
    );

    void addSpectrum(
        const QVector<double>& magnitudeDbfs
    );

    void clear();

    void setPowerRange(
        double minimumDbfs,
        double maximumDbfs
    );

    void setHorizontalMargins(
        int leftMargin,
        int rightMargin
    );

protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private:
    QRgb colorForPower(double powerDbfs) const;
    void recreateImage();

    QImage image_;

    double minimumDbfs_{-120.0};
    double maximumDbfs_{-60.0};

    int leftMargin_{0};
    int rightMargin_{0};
};