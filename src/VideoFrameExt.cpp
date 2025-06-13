#include "VideoFrameExt.h"

#include <QVideoFrameFormat>
#include <QDebug>
#include <utility>

class VideoFrameExtData : public QSharedData
{
public:
    VideoFrameExtData()
        : m_planeFormat(0)
        , m_colorFull(false)
        , m_colorSpace(VideoFrameExt::ColorSpaceBT709)
        , m_colorTransfer(VideoFrameExt::ColorTransferNOOP)
        , m_colorWhite(1.0f)
    {}
    VideoFrameExtData(const VideoFrameExtData &other)
        : QSharedData(other)
        , m_planeFormat(other.m_planeFormat)
        , m_colorFull(other.m_colorFull)
        , m_colorSpace(other.m_colorSpace)
        , m_colorTransfer(other.m_colorTransfer)
        , m_colorWhite(other.m_colorWhite)
    {}
    ~VideoFrameExtData() {}

    int m_planeFormat;
    bool m_colorFull;
    int m_colorSpace;
    int m_colorTransfer;
    float m_colorWhite;
};

VideoFrameExt::VideoFrameExt() : data(new VideoFrameExtData) {}
VideoFrameExt::VideoFrameExt(const VideoFrameExt &rhs) : data{rhs.data} {}
VideoFrameExt::VideoFrameExt(VideoFrameExt &&rhs) : data{std::move(rhs.data)} {}
VideoFrameExt &VideoFrameExt::operator=(const VideoFrameExt &rhs) {
    if (this != &rhs) data = rhs.data;
    return *this;
}
VideoFrameExt &VideoFrameExt::operator=(VideoFrameExt &&rhs) {
    if (this != &rhs) data = std::move(rhs.data);
    return *this;
}
VideoFrameExt::~VideoFrameExt() {}

bool VideoFrameExt::operator==(const VideoFrameExt &other) const
{
    return (data->m_planeFormat   == other.data->m_planeFormat &&
            data->m_colorFull     == other.data->m_colorFull &&
            data->m_colorSpace    == other.data->m_colorSpace &&
            data->m_colorTransfer == other.data->m_colorTransfer &&
            data->m_colorWhite == other.data->m_colorWhite);
}

bool VideoFrameExt::operator!=(const VideoFrameExt &other) const
{
    return (data->m_planeFormat   != other.data->m_planeFormat ||
            data->m_colorFull     != other.data->m_colorFull ||
            data->m_colorSpace    != other.data->m_colorSpace ||
            data->m_colorTransfer != other.data->m_colorTransfer ||
            data->m_colorWhite != other.data->m_colorWhite);
}

VideoFrameExt::VideoFrameExt(int planeFmt, const QVideoFrameFormat &surfaceFormat)
    : data(new VideoFrameExtData)
{
    setPlaneFormat(planeFmt);
    switch (surfaceFormat.colorSpace()) {
    case QVideoFrameFormat::ColorSpace_BT601:
        setColorSpace(ColorSpaceBT601);
        break;
    case QVideoFrameFormat::ColorSpace_BT709:
        setColorSpace(ColorSpaceBT709);
        break;
    case QVideoFrameFormat::ColorSpace_AdobeRgb:
        setColorSpace(ColorSpaceAdobeRgb);
        break;
    case QVideoFrameFormat::ColorSpace_BT2020:
        setColorSpace(ColorSpaceBT2020);
        break;
    default:
        setColorSpace(surfaceFormat.frameHeight() > 576 ? ColorSpaceBT709 : ColorSpaceBT601);
    }

    setColorFull(surfaceFormat.colorRange() == QVideoFrameFormat::ColorRange_Full);

    // By default, it is assumed that the color has already been transmitted using the
    // color space conversion
    if (surfaceFormat.colorTransfer() == QVideoFrameFormat::ColorTransfer_ST2084) {
        float m1 = 1305.0f / 8192.0f, m2 = 2523.0f / 32.0f;
        float c1 = 107.0f / 128.0f, c2 = 2413.0f / 128.0f, c3 = 2392.0f / 128.0f;
        float psig = qPow((surfaceFormat.maxLuminance() / 100.0f) * (1.0f / 100.0f), m1);
        setColorWhite(qPow((c1 + c2 * psig) / (1.0f + c3 * psig), m2));
        setColorTransfer(ColorTransferST2084);
    } else if (surfaceFormat.colorTransfer() == QVideoFrameFormat::ColorTransfer_STD_B67) {
        float a = 0.17883277f, b = 0.28466892f, c = 0.55991073f;
        float sig = surfaceFormat.maxLuminance() / 100.0f;
        if (sig < 1.0f / 12.0f) setColorWhite(qSqrt(3.0f * sig));
        else setColorWhite(a * qLn(12.0f * sig - b) + c);
        setColorTransfer(ColorTransferSTD_B67);
    }
}

void VideoFrameExt::setPlaneFormat(int plane)
{
    data->m_planeFormat = plane;
}

int VideoFrameExt::planeFormat() const
{
    return data->m_planeFormat;
}

void VideoFrameExt::setColorFull(bool yes)
{
    data->m_colorFull = yes;
}

bool VideoFrameExt::isColorFull() const
{
    return data->m_colorFull;
}

void VideoFrameExt::setColorSpace(int space)
{
    data->m_colorSpace = space;
}

int VideoFrameExt::colorSpace() const
{
    return data->m_colorSpace;
}

void VideoFrameExt::setColorTransfer(int transfer)
{
    data->m_colorTransfer = transfer;
}

int VideoFrameExt::colorTransfer() const
{
    return data->m_colorTransfer;
}

void VideoFrameExt::setColorWhite(float white)
{
    data->m_colorWhite = qBound(0.0f, white, 1.0f);
}

float VideoFrameExt::colorWhite() const
{
    return data->m_colorWhite;
}

void VideoFrameExt::dump(QDebug &dbg) const
{
    QDebugStateSaver saver(dbg);
    dbg.noquote();
    dbg << "VideoFrameExt(planeFormat" << data->m_planeFormat
        << ", isColorFull"   << data->m_colorFull
        << ", colorSpace"    << data->m_colorSpace
        << ", colorTransfer" << data->m_colorTransfer
        << ", colorWhite"    << data->m_colorWhite
        << ")";
}
