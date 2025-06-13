#include "PanoramaView.h"
#include "VideoRenderer.h"

#include <QQuickWindow>
#include <QSGRendererInterface>
#include <QRunnable>
#include <QtDebug>

//#define TRACE_PANORAMAVIEW
#ifdef  TRACE_PANORAMAVIEW
#include <QTime>
#include <QThread>
#define TRACE()      qDebug() << QTime::currentTime().toString("hh:mm:ss.zzz") << QThread::currentThreadId() << Q_FUNC_INFO;
#define TRACE_ARG(x) qDebug() << QTime::currentTime().toString("hh:mm:ss.zzz") << QThread::currentThreadId() << Q_FUNC_INFO << x;
#else
#define TRACE()
#define TRACE_ARG(x)
#endif

// round up to 2 digit after point
static qreal roundTo2(qreal value)
{
    return int(value * 100.0 + 0.5) / 100.0;
}

PanoramaView::PanoramaView(QQuickItem *parent)
    : QQuickItem(parent)
    , m_renderer(nullptr)
    , m_debugOpenGL(false)
    , m_rotateDisplay(0)
    , m_stereoShift(defaultStereoShift)
    , m_pitchAngle(0.0)
    , m_yawAngle(0.0)
    , m_fovAngle(FovDef)
    , m_mousePress(false)
{
    TRACE_ARG(parent);
    setAcceptedMouseButtons(Qt::LeftButton);
    setFocus(true);
    connect(this, &QQuickItem::windowChanged, this, &PanoramaView::onWindowChanged);
}

bool PanoramaView::debugOpenGL() const
{
    return m_debugOpenGL;
}

void PanoramaView::setDebugOpenGL(bool yes)
{
    TRACE_ARG(yes);
    if (yes != m_debugOpenGL) {
        m_debugOpenGL = yes;
        if (m_renderer)
            QMetaObject::invokeMethod(m_renderer, "setDebugOpenGL", Q_ARG(bool, m_debugOpenGL));
        emit debugOpenGLChanged();
    }
}

int PanoramaView::rotateDisplay() const
{
    return m_rotateDisplay;
}

void PanoramaView::setRotateDisplay(int direction)
{
    TRACE_ARG(direction);
    int dir = qBound(-1, direction, 1);
    if (dir != m_rotateDisplay) {
        m_rotateDisplay = dir;
        emit rotateDisplayChanged();
        if (window()) window()->update();
    }
}

qreal PanoramaView::stereoShift() const
{
    return m_stereoShift;
}

void PanoramaView::setStereoShift(qreal stereo)
{
    TRACE_ARG(stereo);
    qreal shift = roundTo2(qBound(0.0, stereo, 1.0));
    if (shift != m_stereoShift) {
        m_stereoShift = shift;
        emit stereoShiftChanged();
        if (window()) window()->update();
    }
}

qreal PanoramaView::pitchAngle() const
{
    return m_pitchAngle;
}

void PanoramaView::setPitchAngle(qreal angle)
{
    TRACE_ARG(angle);
    qreal degree = roundTo2(qBound(-90.0, angle, 90.0));
    if (degree != m_pitchAngle) {
        m_pitchAngle = degree;
        emit pitchAngleChanged();
        if (window()) window()->update();
    }
}

qreal PanoramaView::yawAngle() const
{
    return m_yawAngle;
}

void PanoramaView::setYawAngle(qreal angle)
{
    TRACE_ARG(angle);
    qreal degree = roundTo2(qBound(-180.0, angle, 180.0));
    if (degree != m_yawAngle) {
        m_yawAngle = degree;
        emit yawAngleChanged();
        if (window()) window()->update();
    }
}

int PanoramaView::fovAngle() const
{
    return m_fovAngle;
}

void PanoramaView::setFovAngle(int angle)
{
    TRACE_ARG(angle);
    if (angle < 1) {
        angle = FovDef;
        m_mouseBase.setX(0.0);
        m_mouseBase.setY(0.0);
    }
    int degree = qBound((int)FovMin, angle, (int)FovMax);
    if (degree != m_fovAngle) {
        m_fovAngle = degree;
        emit fovAngleChanged();
        if (window()) window()->update();
    }
}

void PanoramaView::setOrientation(qreal p, qreal y)
{
    TRACE_ARG(p << y);
    qreal pitch = roundTo2(qBound(-90.0, p, 90.0));
    qreal yaw = roundTo2(qBound(-180.0, y, 180.0));
    bool pitch_changed = (pitch != m_pitchAngle);
    if (pitch_changed) m_pitchAngle = pitch;
    bool yaw_changed = (yaw != m_yawAngle);
    if (yaw_changed) m_yawAngle = yaw;
    if (pitch_changed || yaw_changed) {
        if (pitch_changed) emit pitchAngleChanged();
        if (yaw_changed) emit yawAngleChanged();
        if (window()) window()->update();
    }
}

QString PanoramaView::graphicsApi() const
{
    return m_graphicsApi;
}

static QString graphicsApiText(QSGRendererInterface::GraphicsApi gapi)
{
    switch (gapi) {
    case QSGRendererInterface::Unknown:    return QStringLiteral("Unknown");
    case QSGRendererInterface::Software:   return QStringLiteral("Software");
    case QSGRendererInterface::OpenVG:     return QStringLiteral("OpenVG");
    case QSGRendererInterface::OpenGL:     return QStringLiteral("OpenGL");
    case QSGRendererInterface::Direct3D11: return QStringLiteral("Direct3D11");
#if QT_VERSION >= QT_VERSION_CHECK(6, 6, 0)
    case QSGRendererInterface::Direct3D12: return QStringLiteral("Direct3D12");
#endif
    case QSGRendererInterface::Vulkan:     return QStringLiteral("Vulkan");
    case QSGRendererInterface::Metal:      return QStringLiteral("Metal");
    case QSGRendererInterface::Null:       return QStringLiteral("Null");
    }
    return QStringLiteral("None");
}

QString PanoramaView::errorText() const
{
    return m_errorText;
}

void PanoramaView::setErrorText(const QString &text)
{
    TRACE_ARG(text);
    if (text != m_errorText) {
        m_errorText = text;
        emit errorTextChanged();
    }
}

void PanoramaView::onWindowChanged(QQuickWindow *win)
{
    if (!win) return;
    TRACE_ARG(win->surfaceType());
    QSGRendererInterface *rif = win->rendererInterface();
    if (!rif) return;
    const auto gapi = rif->graphicsApi();
    if (gapi == QSGRendererInterface::OpenGL) {
        connect(win, &QQuickWindow::beforeSynchronizing,
                this, &PanoramaView::onBeforeSynchronizing, Qt::DirectConnection);
        connect(win, &QQuickWindow::sceneGraphInvalidated,
                this, &PanoramaView::onSceneGraphInvalidated, Qt::DirectConnection);
    } else {
        setErrorText(QStringLiteral("Current graphics API: %1, but OpenGL is required!").arg(graphicsApiText(gapi)));
    }
    QString text = graphicsApiText(gapi);
    if (text != m_graphicsApi) {
        m_graphicsApi = text;
        emit graphicsApiChanged();
    }
}

void PanoramaView::setVideoFrame(const QVideoFrame &frame)
{
    auto win = window();
    if (!win) return;
    TRACE_ARG(frame);
    if (VideoRenderer::isFrameSuppored(frame)) {
        m_videoFrame = frame;
        win->update();
    }
}

void PanoramaView::onBeforeSynchronizing()
{
    auto win = window();
    if (!win) return;
    TRACE();
    if (!m_renderer) {
        m_renderer = new VideoRenderer(win, m_debugOpenGL);
        connect(m_renderer, &VideoRenderer::errorOccurred,
                this, &PanoramaView::setErrorText, Qt::QueuedConnection);
        win->setColor(Qt::black);
    }
    m_renderer->setRotateDisplay(m_rotateDisplay);
    m_renderer->setStereoShift(m_stereoShift);
    m_renderer->setProjection(m_fovAngle);
    m_renderer->setOrientation(m_pitchAngle, m_yawAngle);
    if (m_videoFrame.isValid())
        m_renderer->setVideoFrame(m_videoFrame);
}

void PanoramaView::onSceneGraphInvalidated()
{
    TRACE();
    if (!m_renderer) return;
    delete m_renderer;
    m_renderer = nullptr;
}

class CleanupJob : public QRunnable
{
public:
    CleanupJob(VideoRenderer *renderer) : m_renderer(renderer) { }
    void run() override { delete m_renderer; }
private:
    VideoRenderer *m_renderer;
};

void PanoramaView::releaseResources()
{
    if (!m_renderer) return;
    auto win = window();
    if (!win) return;
    TRACE();
    win->scheduleRenderJob(new CleanupJob(m_renderer), QQuickWindow::BeforeSynchronizingStage);
    m_renderer = nullptr;
}

void PanoramaView::keyPressEvent(QKeyEvent *event)
{
    auto win = window();
    if (!win) return;
    TRACE_ARG(event->key());
    switch (event->key()) {
    case Qt::Key_Plus:
    case Qt::Key_Right:
        if (m_stereoShift <= 0.9) {
            m_stereoShift += 0.1;
            win->update();
        }
        break;
    case Qt::Key_Minus:
    case Qt::Key_Left:
        if (m_stereoShift >= 0.1) {
            m_stereoShift -= 0.1;
            if (m_stereoShift < 0.1) m_stereoShift = 0.0;
            win->update();
        }
        break;
    case Qt::Key_Equal:
    case Qt::Key_Asterisk:
        if (!qFuzzyCompare(m_stereoShift, defaultStereoShift)) {
            m_stereoShift = defaultStereoShift;
            win->update();
        }
        break;
    case Qt::Key_Space:
    case Qt::Key_Backspace:
        setStereoShift(defaultStereoShift);
        setFovAngle(0);
        setOrientation(0.0, 0.0);
        break;
    case Qt::Key_F11:
        if (win->visibility() != QWindow::FullScreen)
             win->setVisibility(QWindow::FullScreen);
        else win->setVisibility(QWindow::Windowed);
        break;
    case Qt::Key_F12:
        if (win->visibility() != QWindow::FullScreen)
            win->resize(QSize(1280, 720)); // HD
        break;
    default:
        event->ignore();
    }
}

void PanoramaView::mousePressEvent(QMouseEvent *event)
{
    TRACE();
    if (!m_renderer) return;
    m_mousePress = true;
    m_mousePos = event->position();
    m_mouseAngle.setX(0.0);
    m_mouseAngle.setY(0.0);
}

void PanoramaView::mouseMoveEvent(QMouseEvent *event)
{
    if (!m_mousePress) return;
    auto win = window();
    if (!win) return;
    TRACE();
    QPointF dp = event->position() - m_mousePos;
    qreal vpw = qreal(win->width()) * win->devicePixelRatio();
    qreal vph = qreal(win->height()) * win->devicePixelRatio();
    m_mouseAngle.setX(dp.x() / vpw * 180.0);
    m_mouseAngle.setY(dp.y() / vph * 90.0);
    setOrientation(m_mouseBase.y() + m_mouseAngle.y(), m_mouseBase.x() + m_mouseAngle.x());
}

void PanoramaView::mouseReleaseEvent(QMouseEvent *event)
{
    if (!m_mousePress || !m_renderer) return;
    TRACE();
    Q_UNUSED(event);
    m_mousePress = false;
    m_mouseBase += m_mouseAngle;
    m_mouseAngle.setX(0.0);
    m_mouseAngle.setY(0.0);
}

void PanoramaView::wheelEvent(QWheelEvent *event)
{
    setFovAngle(m_fovAngle - event->angleDelta().y() / 120);
}
