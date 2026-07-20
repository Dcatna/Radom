#include "waterfall_widget.h"

#include <QPainter>
#include <QPaintEvent>
#include <QResizeEvent>
#include <QPalette>

#include <algorithm>
#include <cmath>

#include <cstring>

WaterfallWidget::WaterfallWidget(
    QWidget* parent
)
    : QWidget(parent)
{
    setMinimumHeight(250);
    recreateImage();
}

void WaterfallWidget::setPowerRange(
    double minimumDbfs,
    double maximumDbfs
)
{
    if (maximumDbfs <= minimumDbfs)
    {
        return;
    }

    minimumDbfs_ = minimumDbfs;
    maximumDbfs_ = maximumDbfs;
}

void WaterfallWidget::addSpectrum(
    const QVector<double>& magnitudeDbfs
)
{
    if (
        image_.isNull() ||
        magnitudeDbfs.isEmpty()
    )
    {
        return;
    }

    const int imageWidth = image_.width();
    const int imageHeight = image_.height();
    const int inputSize = magnitudeDbfs.size();

    /*
     * Move every existing row down by one.
     * Copy from bottom to top so rows are not overwritten
     * before they are moved.
     */
    for (int y = imageHeight - 1; y > 0; --y)
    {
        std::memcpy(
            image_.scanLine(y),
            image_.constScanLine(y - 1),
            static_cast<std::size_t>(
                image_.bytesPerLine()
            )
        );
    }

    /*
     * Write the newest FFT frame into row zero.
     */
    auto* topRow =
        reinterpret_cast<QRgb*>(
            image_.scanLine(0)
        );

    for (int x = 0; x < imageWidth; ++x)
    {
        const double normalizedPosition =
            imageWidth > 1
                ? static_cast<double>(x) /
                      static_cast<double>(
                          imageWidth - 1
                      )
                : 0.0;

        const int inputIndex = std::clamp(
            static_cast<int>(
                std::round(
                    normalizedPosition *
                    static_cast<double>(
                        inputSize - 1
                    )
                )
            ),
            0,
            inputSize - 1
        );

        topRow[x] = colorForPower(
            magnitudeDbfs[inputIndex]
        );
    }

    update();
}

void WaterfallWidget::clear()
{
    if (!image_.isNull())
    {
        image_.fill(Qt::black);
        update();
    }
}

void WaterfallWidget::setHorizontalMargins(
    int leftMargin,
    int rightMargin
)
{
    leftMargin_ = std::max(leftMargin, 0);
    rightMargin_ = std::max(rightMargin, 0);

    update();
}

void WaterfallWidget::paintEvent(
    QPaintEvent*
)
{
    QPainter painter(this);

    const int drawableWidth =
        width() - leftMargin_ - rightMargin_;

    if (drawableWidth <= 0 || image_.isNull())
    {
        return;
    }

    painter.drawImage(
        QRect(
            leftMargin_,
            0,
            drawableWidth,
            height()
        ),
        image_
    );
}
void WaterfallWidget::resizeEvent(
    QResizeEvent*
)
{
    recreateImage();
}

void WaterfallWidget::recreateImage()
{
    const int imageWidth =
        std::max(width(), 1);

    const int imageHeight =
        std::max(height(), 1);

    QImage newImage(
        imageWidth,
        imageHeight,
        QImage::Format_RGB32
    );

    newImage.fill(Qt::black);

    if (!image_.isNull())
    {
        QPainter painter(&newImage);

        painter.drawImage(
            newImage.rect(),
            image_
        );
    }

    image_ = std::move(newImage);
}

QRgb WaterfallWidget::colorForPower(
    double powerDbfs
) const
{
    const double normalized = std::clamp(
        (powerDbfs - minimumDbfs_) /
            (maximumDbfs_ - minimumDbfs_),
        0.0,
        1.0
    );

    /*
     * Simple black → blue → cyan → yellow → white map.
     * We can replace this with a configurable colormap later.
     */
    int red = 0;
    int green = 0;
    int blue = 0;

    if (normalized < 0.25)
    {
        const double section =
            normalized / 0.25;

        blue = static_cast<int>(
            255.0 * section
        );
    }
    else if (normalized < 0.50)
    {
        const double section =
            (normalized - 0.25) / 0.25;

        green = static_cast<int>(
            255.0 * section
        );

        blue = 255;
    }
    else if (normalized < 0.75)
    {
        const double section =
            (normalized - 0.50) / 0.25;

        red = static_cast<int>(
            255.0 * section
        );

        green = 255;

        blue = static_cast<int>(
            255.0 * (1.0 - section)
        );
    }
    else
    {
        const double section =
            (normalized - 0.75) / 0.25;

        red = 255;
        green = 255;
        blue = static_cast<int>(
            255.0 * section
        );
    }

    return qRgb(red, green, blue);
}