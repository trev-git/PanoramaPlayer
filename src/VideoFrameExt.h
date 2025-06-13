#ifndef VIDEOFRAMEEXT_H
#define VIDEOFRAMEEXT_H

#include <QMetaType>
#include <QSharedDataPointer>

class QVideoFrameFormat;
class VideoFrameExtData;

class VideoFrameExt
{
public:
    // see shaders/color.frag
    enum ColorSpace {
        ColorSpaceBT601    = 1,
        ColorSpaceBT709    = 2,
        ColorSpaceAdobeRgb = 3,
        ColorSpaceBT2020   = 4
    };
    enum ColorTransfer {
        ColorTransferNOOP    = 1,
        ColorTransferST2084  = 2,
        ColorTransferSTD_B67 = 3
    };

    VideoFrameExt();
    VideoFrameExt(int planeFormat, const QVideoFrameFormat &surfaceFormat);
    VideoFrameExt(const VideoFrameExt &);
    VideoFrameExt(VideoFrameExt &&);
    VideoFrameExt &operator=(const VideoFrameExt &);
    VideoFrameExt &operator=(VideoFrameExt &&);
    ~VideoFrameExt();

    bool operator==(const VideoFrameExt &other) const;
    bool operator!=(const VideoFrameExt &other) const;

    void setPlaneFormat(int plane);
    int planeFormat() const;

    void setColorFull(bool yes);
    bool isColorFull() const;

    void setColorSpace(int space);
    int colorSpace() const;

    void setColorTransfer(int transfer);
    int colorTransfer() const;

    void setColorWhite(float white);
    float colorWhite() const; // 0.0..1.0

    friend inline QDebug& operator<<(QDebug &dbg, const VideoFrameExt &from) {
        from.dump(dbg); return dbg; }

private:
    void dump(QDebug &dbg) const;

    QSharedDataPointer<VideoFrameExtData> data;
};

Q_DECLARE_TYPEINFO(VideoFrameExt, Q_MOVABLE_TYPE);

Q_DECLARE_METATYPE(VideoFrameExt)

#endif // VIDEOFRAMEEXT_H
