#include "VideoRenderer.h"

#include <QSGRendererInterface>
#include <QQuickWindow>
#include <QSurfaceFormat>
#include <QOpenGLContext>
#include <QOpenGLDebugLogger>
#include <QQuaternion>
#include <QTimer>
#include <QFile>

#include <GL/glcorearb.h>

//#define TRACE_VIDEORENDERER
#ifdef  TRACE_VIDEORENDERER
#include <QTime>
#include <QThread>
#define TRACE()      qDebug() << QTime::currentTime().toString("hh:mm:ss.zzz") << QThread::currentThreadId() << Q_FUNC_INFO;
#define TRACE_ARG(x) qDebug() << QTime::currentTime().toString("hh:mm:ss.zzz") << QThread::currentThreadId() << Q_FUNC_INFO << x;
#else
#define TRACE()
#define TRACE_ARG(x)
#endif

VideoRenderer::VideoRenderer(QQuickWindow *win, bool debugOpenGL)
    : m_window(win)
    , m_openGLES(false)
    , m_anisotropic(false)
    , m_initialized(false)
    , m_rotateDisplay(0)
    , m_stereoShift(0.0)
    , m_frameCount(0)
    , m_renderFrame(false)
    , m_viewSize(3840, 2160) // UHD 4k by default
{
    Q_ASSERT(m_window);

    const auto ctx = QOpenGLContext::currentContext();
    if (!ctx || !ctx->isValid()) {
        emitErrorOccured(QStringLiteral("Invalid OpenGL context"));
        return;
    }
    const auto fmt = ctx->format();
    if ((fmt.renderableType() != QSurfaceFormat::OpenGL &&
         fmt.renderableType() != QSurfaceFormat::OpenGLES) || fmt.majorVersion() < 3) {
        emitErrorOccured(QStringLiteral("Requires at least OpenGL[ES] version 3"));
        return;
    }
    initializeOpenGLFunctions();

    m_viewportSize = m_window->size() * m_window->devicePixelRatio();
    m_openGLES = (fmt.renderableType() == QSurfaceFormat::OpenGLES ||
                  QOpenGLContext::openGLModuleType() == QOpenGLContext::LibGLES);
    m_anisotropic = (ctx->hasExtension("GL_ARB_texture_filter_anisotropic") ||
                     ctx->hasExtension("GL_EXT_texture_filter_anisotropic"));
    TRACE_ARG("Viewport" << m_viewportSize << "OpenGLES" << m_openGLES << "Anisotropic" << m_anisotropic);

    connect(m_window, &QQuickWindow::widthChanged, this, &VideoRenderer::onWidthChanged);
    connect(m_window, &QQuickWindow::heightChanged, this, &VideoRenderer::onHeightChanged);
    connect(m_window, &QQuickWindow::beforeRendering, this, &VideoRenderer::onBeforeRendering, Qt::DirectConnection);
    connect(m_window, &QQuickWindow::beforeRenderPassRecording, this, &VideoRenderer::onBeforeRenderPassRecording, Qt::DirectConnection);

    if (debugOpenGL) setDebugOpenGL(true);
}

void VideoRenderer::emitErrorOccured(const QString &text)
{
    QTimer::singleShot(0, this, [this, text]() { emit errorOccurred(text); });
}

void VideoRenderer::onWidthChanged(int width)
{
    TRACE_ARG(width);
    m_viewportSize.setWidth(width * m_window->devicePixelRatio());
}

void VideoRenderer::onHeightChanged(int height)
{
    TRACE_ARG(height);
    m_viewportSize.setHeight(height * m_window->devicePixelRatio());
}

//static
bool VideoRenderer::isFrameSuppored(const QVideoFrame &frame)
{
    if (frame.width() < 1 || frame.height() < 1) // last closing frame?
        return false;

    if (!frame.isValid() || frame.width() < 640 || frame.height() < 480) {
        qInfo() << Q_FUNC_INFO << "Bogus frame discarded" << frame.width() << 'x' << frame.height();
        return false;
    }
    switch (frame.pixelFormat()) {
    case QVideoFrameFormat::Format_YUV420P:
    case QVideoFrameFormat::Format_YUV422P:
    case QVideoFrameFormat::Format_NV12:
    case QVideoFrameFormat::Format_YV12:
        break;
    default:
        return false;
    }
    return true;
}

void VideoRenderer::setDebugOpenGL(bool yes)
{
    TRACE_ARG(yes);
    if (!yes) {
        if (!m_debugLog.isNull())
            m_debugLog->deleteLater();
        return;
    }
    if (m_debugLog.isNull()) {
        m_debugLog = new QOpenGLDebugLogger(this);
        connect(m_debugLog, &QOpenGLDebugLogger::messageLogged, this, [](const QOpenGLDebugMessage &msg) {
            qDebug() << msg;
        });
        m_debugLog->initialize();
        m_debugLog->startLogging();
    }
    GLint maxTexSize, maxFBWidth, maxFBHeight;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTexSize);
    glGetIntegerv(GL_MAX_FRAMEBUFFER_WIDTH, &maxFBWidth);
    glGetIntegerv(GL_MAX_FRAMEBUFFER_HEIGHT, &maxFBHeight);
    qDebug() << QString("In use: %1 v%2, GLSL v%3").arg(m_openGLES ? "OpenGLES" : "OpenGL")
                .arg(reinterpret_cast<const char*>(glGetString(GL_VERSION)))
                .arg(reinterpret_cast<const char*>(glGetString(GL_SHADING_LANGUAGE_VERSION)))
             << "\n\tVendor\t\t" << reinterpret_cast<const char*>(glGetString(GL_VENDOR))
             << "\n\tRenderer\t" << reinterpret_cast<const char*>(glGetString(GL_RENDERER))
             << "\n\tAnisotropic\t" << m_anisotropic
             << "\n\tTextureSize\t" << maxTexSize
             << "\n\tFrameBuffer\t" << maxFBWidth << 'x' << maxFBHeight;
}

void VideoRenderer::setRotateDisplay(int direction)
{
    TRACE_ARG(direction);
    m_rotateDisplay = direction;
}

void VideoRenderer::setStereoShift(qreal shift)
{
    TRACE_ARG(shift);
    m_stereoShift = shift;
}

void VideoRenderer::setProjection(int angle)
{
    TRACE_ARG(angle);
    float fov = qTan(qDegreesToRadians(0.5 * angle));
    QMatrix4x4 matrix;
    matrix.frustum(-fov, fov, -fov, fov, 1.0, 100.0);
    m_projection = matrix;
    m_projection.optimize();
}

void VideoRenderer::setOrientation(qreal pitch, qreal yaw)
{
    TRACE_ARG(pitch);
    QMatrix4x4 matrix;
    matrix.rotate(QQuaternion::fromEulerAngles(pitch, yaw, 0.0).inverted());
    m_orientation = matrix;
    m_orientation.optimize();
}

void VideoRenderer::setVideoFrame(const QVideoFrame &frame)
{
    bool isMapped = m_videoFrame.isMapped() || frame.isMapped();
    TRACE_ARG("isMapped" << isMapped << frame);
    if (!isMapped) {
        m_videoFrame = frame;
        ++m_frameCount;
    } else qWarning() << Q_FUNC_INFO << "Rendering busy - frame lost!";
}

void VideoRenderer::onBeforeRendering()
{
    TRACE();
    m_renderFrame = false;
    if (!m_frameCount || (!m_initialized && !initFunctions()))
        return; // just for sanity

    // Convert the QVideoFrame to a regular RGB texture

    m_initialized = true;
    if (m_videoFrame.map(QVideoFrame::ReadOnly)) {
        m_renderFrame = frameToTexture();
        m_videoFrame.unmap();
    } else qCritical() << Q_FUNC_INFO << "Can't map video frame";
}

void VideoRenderer::onBeforeRenderPassRecording()
{
    TRACE_ARG(m_frameSize << m_viewportSize);
    if (!m_window || !m_renderFrame || m_viewportSize.isEmpty() || m_frameSize.isEmpty())
        return; // just for sanity

    // Visualize the texture as a stereo image

    m_window->beginExternalCommands();
    for (int i = 0; i < 2; i++) {
        if (textureToView(i ? -(m_stereoShift / 250.0) : 0.0))
            renderDisplay(!i);
    }
    m_window->endExternalCommands();
}

GLuint VideoRenderer::setQuadVaoBuffer()
{
    TRACE();
    static const GLfloat positions[] = {
        -1.0f, +1.0f, 0.0f,
        +1.0f, +1.0f, 0.0f,
        +1.0f, -1.0f, 0.0f,
        -1.0f, -1.0f, 0.0f
    };
    static const GLfloat texCoords[] = {
        0.0f, 1.0f,
        1.0f, 1.0f,
        1.0f, 0.0f,
        0.0f, 0.0f
    };
    static const GLushort indices[] = {
        0, 3, 1, 1, 3, 2
    };
    GLuint quadVao;
    glGenVertexArrays(1, &quadVao);
    glBindVertexArray(quadVao);

    GLuint positionBuf;
    glGenBuffers(1, &positionBuf);
    glBindBuffer(GL_ARRAY_BUFFER, positionBuf);
    glBufferData(GL_ARRAY_BUFFER, sizeof(positions), positions, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(0);

    GLuint texCoordBuf;
    glGenBuffers(1, &texCoordBuf);
    glBindBuffer(GL_ARRAY_BUFFER, texCoordBuf);
    glBufferData(GL_ARRAY_BUFFER, sizeof(texCoords), texCoords, GL_STATIC_DRAW);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(1);

    GLuint indexBuf;
    glGenBuffers(1, &indexBuf);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuf);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glBindVertexArray(0);
    GLenum glErr = glGetError();
    if (glErr != GL_NO_ERROR) {
        qCritical() << Q_FUNC_INFO << "OpenGL Error" << glErr << "Line" << __LINE__;
        return 0;
    }
    return quadVao;
}

GLuint VideoRenderer::setCubeVaoBuffer()
{
    TRACE();
    static const GLfloat positions[] = {
        -10.0f, -10.0f, +10.0f,
        +10.0f, -10.0f, +10.0f,
        -10.0f, +10.0f, +10.0f,
        +10.0f, +10.0f, +10.0f,
        +10.0f, -10.0f, -10.0f,
        -10.0f, -10.0f, -10.0f,
        +10.0f, +10.0f, -10.0f,
        -10.0f, +10.0f, -10.0f,
        -10.0f, -10.0f, -10.0f,
        -10.0f, -10.0f, +10.0f,
        -10.0f, +10.0f, -10.0f,
        -10.0f, +10.0f, +10.0f,
        +10.0f, -10.0f, +10.0f,
        +10.0f, -10.0f, -10.0f,
        +10.0f, +10.0f, +10.0f,
        +10.0f, +10.0f, -10.0f,
        -10.0f, +10.0f, -10.0f,
        -10.0f, +10.0f, +10.0f,
        +10.0f, +10.0f, -10.0f,
        +10.0f, +10.0f, +10.0f,
        +10.0f, -10.0f, -10.0f,
        +10.0f, -10.0f, +10.0f,
        -10.0f, -10.0f, -10.0f,
        -10.0f, -10.0f, +10.0f
    };
    static const GLfloat texCoords[] = {
        0.0f, 0.0f,
        1.0f, 0.0f,
        0.0f, 1.0f,
        1.0f, 1.0f,
        0.0f, 0.0f,
        1.0f, 0.0f,
        0.0f, 1.0f,
        1.0f, 1.0f,
        0.0f, 0.0f,
        1.0f, 0.0f,
        0.0f, 1.0f,
        1.0f, 1.0f,
        0.0f, 0.0f,
        1.0f, 0.0f,
        0.0f, 1.0f,
        1.0f, 1.0f,
        0.0f, 0.0f,
        1.0f, 0.0f,
        0.0f, 1.0f,
        1.0f, 1.0f,
        0.0f, 0.0f,
        1.0f, 0.0f,
        0.0f, 1.0f,
        1.0f, 1.0f
    };
    static const unsigned short indices[] = {
        0, 1, 2, 1, 3, 2,
        4, 5, 6, 5, 7, 6,
        8, 9, 10, 9, 11, 10,
        12, 13, 14, 13, 15, 14,
        16, 17, 18, 17, 19, 18,
        20, 21, 22, 21, 23, 22
    };
    GLuint cubeVao;
    glGenVertexArrays(1, &cubeVao);
    glBindVertexArray(cubeVao);

    GLuint positionBuf;
    glGenBuffers(1, &positionBuf);
    glBindBuffer(GL_ARRAY_BUFFER, positionBuf);
    glBufferData(GL_ARRAY_BUFFER, sizeof(positions), positions, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(0);

    GLuint texCoordBuf;
    glGenBuffers(1, &texCoordBuf);
    glBindBuffer(GL_ARRAY_BUFFER, texCoordBuf);
    glBufferData(GL_ARRAY_BUFFER, sizeof(texCoords), texCoords, GL_STATIC_DRAW);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(1);

    GLuint indexBuf;
    glGenBuffers(1, &indexBuf);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuf);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glBindVertexArray(0);
    GLenum glErr = glGetError();
    if (glErr != GL_NO_ERROR) {
        qCritical() << Q_FUNC_INFO << "OpenGL Error" << glErr << "Line" << __LINE__;
        return false;
    }
    return cubeVao;
}

QString VideoRenderer::getShaderSource(const QString &name) const
{
    QFile file(QStringLiteral(":/shaders/") + name);
    if (!file.open(QIODevice::ReadOnly)) {
        qCritical() << Q_FUNC_INFO << "Can't read" << file.fileName();
        return QString();
    }
    QString text = QString::fromLatin1(file.readAll());
    if (name.contains("vert")) {
        text.prepend(m_openGLES ? "#version 300 es\n" : "#version 330\n");
    } else if (name.contains("frag")) {
        text.prepend(m_openGLES ? "#version 300 es\nprecision mediump float;\n" : "#version 330\n");
    }
    return text;
}

bool VideoRenderer::initFunctions()
{
    TRACE();
    glClearColor(0, 0, 0, 1);

    // Texture view

    glGenTextures(1, &m_viewTex);
    TRACE_ARG("Setup view texture" << m_viewTex);
    glBindTexture(GL_TEXTURE_2D, m_viewTex);
    if (m_openGLES)
         glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB10_A2, m_viewSize.width(), m_viewSize.height(), 0, GL_RGBA, GL_UNSIGNED_INT_2_10_10_10_REV, nullptr);
    else glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16,    m_viewSize.width(), m_viewSize.height(), 0, GL_RGBA, GL_UNSIGNED_SHORT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    if (m_anisotropic) glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY, 4.0);
    GLenum glErr = glGetError();
    if (glErr != GL_NO_ERROR) {
        qCritical() << Q_FUNC_INFO << "OpenGL Error" << glErr << "Line" << __LINE__;
        return false;
    }

    // Vertex Array Object quad geometry

    m_quadVao = setQuadVaoBuffer();
    if (!m_quadVao) return false;

    // Vertex Array Object cube geometry

    m_cubeVao = setCubeVaoBuffer();
    if (!m_cubeVao) return false;

    // Plane textures

    glGenTextures(3, m_planeTexs);
    GLuint blackColor = 0;
    for (int i = 0; i < 3; i++) {
        TRACE_ARG("Setup plane texture" << m_planeTexs[i]);
        glBindTexture(GL_TEXTURE_2D, m_planeTexs[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, 1, 1, 0, GL_RED, GL_UNSIGNED_BYTE, &blackColor);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        if (!i) {
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        } else {
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        }
    }
    glErr = glGetError();
    if (glErr != GL_NO_ERROR) {
        qCritical() << Q_FUNC_INFO << "OpenGL Error" << glErr << "Line" << __LINE__;
        return false;
    }

    // Frame textures

    glGenTextures(1, &m_frameTex);
    TRACE_ARG("Setup frame texture" << m_frameTex);
    glBindTexture(GL_TEXTURE_2D, m_frameTex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    if (m_anisotropic) glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY, 4.0);
    glErr = glGetError();
    if (glErr != GL_NO_ERROR) {
        qCritical() << Q_FUNC_INFO << "OpenGL Error" << glErr << "Line" << __LINE__;
        return false;
    }

    // FBO and PBO

    glGenFramebuffers(1, &m_frameFbo);
    glGenFramebuffers(1, &m_viewFbo);
    glGenTextures(1, &m_depthTex);
    TRACE_ARG("Setup depth texture" << m_depthTex);
    TRACE_ARG("Setup frame FBO" << m_frameFbo << "and view FBO" << m_viewFbo);
    glBindTexture(GL_TEXTURE_2D, m_depthTex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, 1, 1, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, nullptr);
    glBindFramebuffer(GL_FRAMEBUFFER, m_viewFbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_depthTex, 0);
    glErr = glGetError();
    if (glErr != GL_NO_ERROR) {
        qCritical() << Q_FUNC_INFO << "OpenGL Error" << glErr << "Line" << __LINE__;
        return false;
    }

    return true;
}

static int alignBytesPerLine(const void *ptr, int bpl)
{
    int align = 1;
    if (quint64(ptr) % 8 == 0 && bpl % 8 == 0)      align = 8;
    else if (quint64(ptr) % 4 == 0 && bpl % 4 == 0) align = 4;
    else if (quint64(ptr) % 2 == 0 && bpl % 2 == 0) align = 2;
    return align;
}

bool VideoRenderer::frameToTexture()
{
    TRACE_ARG(m_videoFrame.pixelFormat() << m_videoFrame.planeCount());
    const auto &frame = m_videoFrame;

    // Get the frame data into plane textures

    // Reset swizzling for plane0; might be changed below depending in the format
    glBindTexture(GL_TEXTURE_2D, m_planeTexs[0]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_R, GL_RED);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_G, GL_GREEN);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_B, GL_BLUE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_A, GL_ALPHA);

    // See shaders/color.frag
    int planeFormat = 2;
    switch (frame.pixelFormat()) {
    case QVideoFrameFormat::Format_YUV420P:
        if (frame.planeCount() != 3) {
            emitErrorOccured(QStringLiteral("Format_YUV420P must contain 3 plains!"));
            return false;
        }
        glPixelStorei(GL_UNPACK_ALIGNMENT, alignBytesPerLine(frame.bits(0), frame.bytesPerLine(0)));
        glPixelStorei(GL_UNPACK_ROW_LENGTH, frame.bytesPerLine(0));
        glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, frame.width(), frame.height(), 0, GL_RED, GL_UNSIGNED_BYTE, frame.bits(0));
        glBindTexture(GL_TEXTURE_2D, m_planeTexs[1]);
        glPixelStorei(GL_UNPACK_ALIGNMENT, alignBytesPerLine(frame.bits(1), frame.bytesPerLine(1)));
        glPixelStorei(GL_UNPACK_ROW_LENGTH, frame.bytesPerLine(1));
        glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, frame.width() / 2, frame.height() / 2, 0, GL_RED, GL_UNSIGNED_BYTE, frame.bits(1));
        glBindTexture(GL_TEXTURE_2D, m_planeTexs[2]);
        glPixelStorei(GL_UNPACK_ALIGNMENT, alignBytesPerLine(frame.bits(2), frame.bytesPerLine(2)));
        glPixelStorei(GL_UNPACK_ROW_LENGTH, frame.bytesPerLine(2));
        glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, frame.width() / 2, frame.height() / 2, 0, GL_RED, GL_UNSIGNED_BYTE, frame.bits(2));
        break;
    case QVideoFrameFormat::Format_YUV422P:
        if (frame.planeCount() != 3) {
            emitErrorOccured(QStringLiteral("Format_YUV422P must contain 3 plains!"));
            return false;
        }
        glPixelStorei(GL_UNPACK_ALIGNMENT, alignBytesPerLine(frame.bits(0), frame.bytesPerLine(0)));
        glPixelStorei(GL_UNPACK_ROW_LENGTH, frame.bytesPerLine(0));
        glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, frame.width(), frame.height(), 0, GL_RED, GL_UNSIGNED_BYTE, frame.bits(0));
        glBindTexture(GL_TEXTURE_2D, m_planeTexs[1]);
        glPixelStorei(GL_UNPACK_ALIGNMENT, alignBytesPerLine(frame.bits(1), frame.bytesPerLine(1)));
        glPixelStorei(GL_UNPACK_ROW_LENGTH, frame.bytesPerLine(1));
        glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, frame.width() / 2, frame.height(), 0, GL_RED, GL_UNSIGNED_BYTE, frame.bits(1));
        glBindTexture(GL_TEXTURE_2D, m_planeTexs[2]);
        glPixelStorei(GL_UNPACK_ALIGNMENT, alignBytesPerLine(frame.bits(2), frame.bytesPerLine(2)));
        glPixelStorei(GL_UNPACK_ROW_LENGTH, frame.bytesPerLine(2));
        glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, frame.width() / 2, frame.height(), 0, GL_RED, GL_UNSIGNED_BYTE, frame.bits(2));
        break;
    case QVideoFrameFormat::Format_NV12:
        if (frame.planeCount() != 2) {
            emitErrorOccured(QStringLiteral("Format_NV12 must contain 2 plains!"));
            return false;
        }
        glPixelStorei(GL_UNPACK_ALIGNMENT, alignBytesPerLine(frame.bits(0), frame.bytesPerLine(0)));
        glPixelStorei(GL_UNPACK_ROW_LENGTH, frame.bytesPerLine(0));
        glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, frame.width(), frame.height(), 0, GL_RED, GL_UNSIGNED_BYTE, frame.bits(0));
        glBindTexture(GL_TEXTURE_2D, m_planeTexs[1]);
        glPixelStorei(GL_UNPACK_ALIGNMENT, alignBytesPerLine(frame.bits(1), frame.bytesPerLine(1)));
        glPixelStorei(GL_UNPACK_ROW_LENGTH, frame.bytesPerLine(1) / 2);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RG8, frame.width() / 2, frame.height() / 2, 0, GL_RG, GL_UNSIGNED_BYTE, frame.bits(1));
        planeFormat = 4;
        break;
    case QVideoFrameFormat::Format_YV12:
        if (frame.planeCount() != 3) {
            emitErrorOccured(QStringLiteral("Format_YV12 must contain 3 plains!"));
            return false;
        }
        glPixelStorei(GL_UNPACK_ALIGNMENT, alignBytesPerLine(frame.bits(0), frame.bytesPerLine(0)));
        glPixelStorei(GL_UNPACK_ROW_LENGTH, frame.bytesPerLine(0));
        glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, frame.width(), frame.height(), 0, GL_RED, GL_UNSIGNED_BYTE, frame.bits(0));
        glBindTexture(GL_TEXTURE_2D, m_planeTexs[1]);
        glPixelStorei(GL_UNPACK_ALIGNMENT, alignBytesPerLine(frame.bits(1), frame.bytesPerLine(1)));
        glPixelStorei(GL_UNPACK_ROW_LENGTH, frame.bytesPerLine(1));
        glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, frame.width() / 2, frame.height() / 2, 0, GL_RED, GL_UNSIGNED_BYTE, frame.bits(1));
        glBindTexture(GL_TEXTURE_2D, m_planeTexs[2]);
        glPixelStorei(GL_UNPACK_ALIGNMENT, alignBytesPerLine(frame.bits(2), frame.bytesPerLine(2)));
        glPixelStorei(GL_UNPACK_ROW_LENGTH, frame.bytesPerLine(2));
        glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, frame.width() / 2, frame.height() / 2, 0, GL_RED, GL_UNSIGNED_BYTE, frame.bits(2));
        planeFormat = 3;
        break;
    default:
        emitErrorOccured(QStringLiteral("Unsupported frame pixel format"));
        return false;
    }
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    GLenum glErr = glGetError();
    if (glErr != GL_NO_ERROR) {
        qCritical() << Q_FUNC_INFO << "OpenGL Error" << glErr << "Line" << __LINE__;
        return false;
    }

    // Convert plane textures into linear RGB in the frame texture

    glBindTexture(GL_TEXTURE_2D, m_frameTex);
    if (m_openGLES)
         glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB10_A2, frame.width(), frame.height(), 0, GL_RGBA, GL_UNSIGNED_INT_2_10_10_10_REV, nullptr);
    else glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16,   frame.width(), frame.height(), 0, GL_BGRA, GL_UNSIGNED_SHORT, nullptr);
    glBindFramebuffer(GL_FRAMEBUFFER, m_frameFbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_frameTex, 0);
    glViewport(0, 0, frame.width(), frame.height());
    glDisable(GL_DEPTH_TEST);
    glErr = glGetError();
    if (glErr != GL_NO_ERROR) {
        qCritical() << Q_FUNC_INFO << "OpenGL Error" << glErr << "Line" << __LINE__;
        return false;
    }

    VideoFrameExt frameExt(planeFormat, frame.surfaceFormat());
    if (!m_colorProg.isLinked() || frameExt != m_frameExt) {
        QString colorVert = getShaderSource("color.vert");
        QString colorFrag = getShaderSource("color.frag");
        if (colorVert.isEmpty() || colorFrag.isEmpty()) return false; // should bot happend
        colorFrag.replace("$PLANE_FORMAT", QString::number(planeFormat));
        colorFrag.replace("$COLOR_RANGE_SMALL", frameExt.isColorFull() ? "false" : "true");
        colorFrag.replace("$COLOR_SPACE", QString::number(frameExt.colorSpace()));
        colorFrag.replace("$COLOR_TRANSFER", QString::number(frameExt.colorTransfer()));
        m_colorProg.removeAllShaders();
        m_colorProg.addCacheableShaderFromSourceCode(QOpenGLShader::Vertex, colorVert);
        m_colorProg.addCacheableShaderFromSourceCode(QOpenGLShader::Fragment, colorFrag);
        m_colorProg.link();
        m_frameExt = frameExt;
        TRACE_ARG("Setup color shader program" << m_colorProg.programId());
    }
    glUseProgram(m_colorProg.programId());
    m_colorProg.setUniformValue("masteringWhite", frameExt.colorWhite());
    for (int i = 0; i < frame.planeCount(); i++) {
        m_colorProg.setUniformValue(qPrintable(QString("plane%1").arg(i)), i);
        glActiveTexture(GL_TEXTURE0 + i);
        glBindTexture(GL_TEXTURE_2D, m_planeTexs[i]);
    }
    glBindVertexArray(m_quadVao);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0);
    glBindTexture(GL_TEXTURE_2D, m_frameTex);
    glGenerateMipmap(GL_TEXTURE_2D);

    glBindVertexArray(0);
    glErr = glGetError();
    if (glErr != GL_NO_ERROR) {
        qCritical() << Q_FUNC_INFO << "OpenGL Error" << glErr << "Line" << __LINE__;
        return false;
    }

    m_frameSize.setWidth(frame.width());
    m_frameSize.setHeight(frame.height());
    return true;
}

bool VideoRenderer::textureToView(float xOffs)
{
    TRACE_ARG(xOffs);

    // Prepare view texture and render view into it

    glBindTexture(GL_TEXTURE_2D, m_viewTex);
    if (m_frameSize != m_viewSize) {
        m_viewSize = m_frameSize;
        if (m_openGLES)
             glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB10_A2, m_viewSize.width(), m_viewSize.height(), 0, GL_RGBA, GL_UNSIGNED_INT_2_10_10_10_REV, nullptr);
        else glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16,    m_viewSize.width(), m_viewSize.height(), 0, GL_RGBA, GL_UNSIGNED_SHORT, nullptr);
    }
    glBindTexture(GL_TEXTURE_2D, m_depthTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, m_frameSize.width(), m_frameSize.height(), 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, nullptr);
    glEnable(GL_DEPTH_TEST);
    glBindFramebuffer(GL_FRAMEBUFFER, m_viewFbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_viewTex, 0);
    glViewport(0, 0, m_frameSize.width(), m_frameSize.height());
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    GLenum glErr = glGetError();
    if (glErr != GL_NO_ERROR) {
        qCritical() << Q_FUNC_INFO << "OpenGL Error" << glErr << "Line" << __LINE__;
        return false;
    }
    if (!m_viewProg.isLinked()) {
        if (!m_viewProg.addCacheableShaderFromSourceCode(QOpenGLShader::Vertex, getShaderSource("view.vert")) ||
            !m_viewProg.addCacheableShaderFromSourceCode(QOpenGLShader::Fragment, getShaderSource("view.frag")))
            return false;
        m_viewProg.link();
        TRACE_ARG("Setup view shader program" << m_viewProg.programId());
    }
    glUseProgram(m_viewProg.programId());
    m_viewProg.setUniformValue("projection", m_projection);
    m_viewProg.setUniformValue("orientation", m_orientation);
    m_viewProg.setUniformValue("frame_tex", 0);
    m_viewProg.setUniformValue("view_xoffs", xOffs);

    // Render scene
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_frameTex);
    glErr = glGetError();
    if (glErr != GL_NO_ERROR) {
        qCritical() << Q_FUNC_INFO << "OpenGL Error" << glErr << "Line" << __LINE__;
        return false;
    }

    // Setup filtering to work correctly at the horizontal wraparound
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // Render vertexes
    glBindVertexArray(m_cubeVao);
    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_SHORT, 0);

    // Reset filtering parameters to their defaults
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

    // Generate mipmaps for the view texture
    glBindTexture(GL_TEXTURE_2D, m_viewTex);
    glGenerateMipmap(GL_TEXTURE_2D);

    glBindVertexArray(0);
    glErr = glGetError();
    if (glErr != GL_NO_ERROR) {
        qCritical() << Q_FUNC_INFO << "OpenGL Error" << glErr << "Line" << __LINE__;
        return false;
    }
    return true;
}

void VideoRenderer::renderDisplay(bool first)
{
    TRACE_ARG(first);

    // Put the views on screen using framebuffer

    glBindFramebuffer(GL_FRAMEBUFFER, QOpenGLContext::currentContext()->defaultFramebufferObject());
    int halfWidth = m_viewportSize.width() / 2;
    int halfHeight = m_viewportSize.height() / 2;
    if (first) {
        if (halfWidth > halfHeight)
             glViewport(0, 0, halfWidth, m_viewportSize.height());
        else glViewport(0, 0, m_viewportSize.width(), halfHeight);
    } else {
        if (halfWidth > halfHeight)
             glViewport(halfWidth, 0, halfWidth, m_viewportSize.height());
        else glViewport(0, halfHeight, m_viewportSize.width(), halfHeight);
    }
    glDisable(GL_DEPTH_TEST);
    GLenum glErr = glGetError();
    if (glErr != GL_NO_ERROR) {
        qCritical() << Q_FUNC_INFO << "OpenGL Error" << glErr << "Line" << __LINE__;
        return;
    }
    if (!m_dispProg.isLinked()) {
        if (!m_dispProg.addCacheableShaderFromSourceCode(QOpenGLShader::Vertex, getShaderSource("display.vert")) ||
            !m_dispProg.addCacheableShaderFromSourceCode(QOpenGLShader::Fragment, getShaderSource("display.frag"))) {
            return;
        }
        m_dispProg.link();
        TRACE_ARG("Setup display shader program" << m_dispProg.programId());
    }
    glUseProgram(m_dispProg.programId());
    m_dispProg.setUniformValue("view_tex", 0);
    m_dispProg.setUniformValue("rotate_dir", m_rotateDisplay);

    // Bind textures for vertexes and draw its

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_viewTex);
    glBindVertexArray(m_quadVao);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0);

    glBindVertexArray(0);
    glErr = glGetError();
    if (glErr != GL_NO_ERROR) {
        qCritical() << Q_FUNC_INFO << "OpenGL Error" << glErr << "Line" << __LINE__;
    }
}
