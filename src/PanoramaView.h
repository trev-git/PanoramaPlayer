#ifndef PANORAMAVIEW_H
#define PANORAMAVIEW_H

#include <QObject>
#include <QQmlEngine>
#include <QQuickItem>
#include <QVideoFrame>

class VideoRenderer;

class PanoramaView : public QQuickItem
{
    Q_OBJECT
    Q_PROPERTY(bool    debugOpenGL READ debugOpenGL   WRITE setDebugOpenGL   NOTIFY debugOpenGLChanged FINAL)
    Q_PROPERTY(int   rotateDisplay READ rotateDisplay WRITE setRotateDisplay NOTIFY rotateDisplayChanged FINAL)
    Q_PROPERTY(qreal   stereoShift READ stereoShift   WRITE setStereoShift   NOTIFY stereoShiftChanged FINAL)
    Q_PROPERTY(qreal    pitchAngle READ pitchAngle    WRITE setPitchAngle    NOTIFY pitchAngleChanged FINAL)
    Q_PROPERTY(qreal      yawAngle READ yawAngle      WRITE setYawAngle      NOTIFY yawAngleChanged FINAL)
    Q_PROPERTY(int        fovAngle READ fovAngle      WRITE setFovAngle      NOTIFY fovAngleChanged FINAL)
    Q_PROPERTY(QString graphicsApi READ graphicsApi   NOTIFY graphicsApiChanged FINAL)
    Q_PROPERTY(QString   errorText READ errorText     NOTIFY errorTextChanged FINAL)
    QML_ELEMENT

public:
    static constexpr qreal const defaultStereoShift = 0.5;

    explicit PanoramaView(QQuickItem *parent = nullptr);

    enum FovAngle { // vertical FOV angle in degree
        FovMin = 35,
        FovDef = 95,
        FovMax = 115
    };
    Q_ENUM(FovAngle)

    bool debugOpenGL() const;
    void setDebugOpenGL(bool yes);

    int rotateDisplay() const;
    void setRotateDisplay(int direction); // -1/0/1

    qreal stereoShift() const;
    void setStereoShift(qreal stereo); // 0.0..1.0

    qreal pitchAngle() const;
    void setPitchAngle(qreal angle); // -90.0..90.0

    qreal yawAngle() const;
    void setYawAngle(qreal angle); // -180.0..180.0

    int fovAngle() const;
    void setFovAngle(int angle); // FovMin..FovMax in degree, use 0 to reset to default

    QString graphicsApi() const;
    QString errorText() const;

    void setOrientation(qreal pitch, qreal yaw);
    void setVideoFrame(const QVideoFrame &frame);

signals:
    void debugOpenGLChanged();
    void rotateDisplayChanged();
    void stereoShiftChanged();
    void pitchAngleChanged();
    void yawAngleChanged();
    void fovAngleChanged();
    void graphicsApiChanged();
    void errorTextChanged();

protected:
    void releaseResources() override;
    void keyPressEvent(QKeyEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;

private:
    void setErrorText(const QString &text);
    void onWindowChanged(QQuickWindow *window);
    void onBeforeSynchronizing();
    void onSceneGraphInvalidated();

    VideoRenderer *m_renderer;
    bool m_debugOpenGL;
    int m_rotateDisplay;
    qreal m_stereoShift;
    qreal m_pitchAngle;
    qreal m_yawAngle;
    int m_fovAngle;
    QString m_graphicsApi;
    QVideoFrame m_videoFrame;
    QString m_errorText;

    bool m_mousePress;
    QPointF m_mousePos;
    QPointF m_mouseBase;
    QPointF m_mouseAngle;
};

#endif // PANORAMAVIEW_H
