#ifndef VIDEORENDERER_H
#define VIDEORENDERER_H

#include <QObject>
#include <QPointer>
#include <QOpenGLExtraFunctions>
#include <QOpenGLShaderProgram>
#include <QVideoFrame>
#include <QMatrix4x4>
#include <QSize>

#include "VideoFrameExt.h"

class QQuickWindow;
class QOpenGLDebugLogger;

class VideoRenderer : public QObject, protected QOpenGLExtraFunctions
{
    Q_OBJECT
public:
    VideoRenderer(QQuickWindow *win, bool debugOpenGL = false); // the win is not parent!

    static bool isFrameSuppored(const QVideoFrame &frame);

    void setRotateDisplay(int direction); // -1/0/1
    void setStereoShift(qreal shift); // 0.0..1.0
    void setProjection(int angle); // vertical FOV angle 5..115 in degree
    void setOrientation(qreal pitch, qreal yaw); // circular orientation using Euler angles
    void setVideoFrame(const QVideoFrame &frame);

public slots:
    void setDebugOpenGL(bool yes);

signals:
    void errorOccurred(const QString &text);

private:
    void emitErrorOccured(const QString &text);
    void onWidthChanged(int width);
    void onHeightChanged(int height);
    void onBeforeRendering();
    void onBeforeRenderPassRecording();
    GLuint setQuadVaoBuffer();
    GLuint setCubeVaoBuffer();
    QString getShaderSource(const QString &name) const;
    bool initFunctions();
    bool frameToTexture();
    bool textureToView(float xOffs);
    void renderDisplay(bool first);

    QPointer<QQuickWindow> m_window;
    bool m_openGLES;
    bool m_anisotropic;
    bool m_initialized;

    QPointer<QOpenGLDebugLogger> m_debugLog;
    QMatrix4x4 m_projection, m_orientation;
    int m_rotateDisplay;
    qreal m_stereoShift;

    qint64 m_frameCount;
    bool m_renderFrame;
    QVideoFrame m_videoFrame;

    GLuint m_planeTexs[3], m_frameTex, m_frameFbo;
    VideoFrameExt m_frameExt;
    QSize m_frameSize;
    QOpenGLShaderProgram m_colorProg;

    GLuint m_viewTex, m_quadVao, m_cubeVao;
    QSize m_viewSize;
    QOpenGLShaderProgram m_viewProg;

    GLuint m_depthTex, m_viewFbo;
    QSize m_viewportSize;
    QOpenGLShaderProgram m_dispProg;
};

#endif // VIDEORENDERER_H
