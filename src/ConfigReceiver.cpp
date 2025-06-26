#include "ConfigReceiver.h"
#include "PanoramaView.h" // just for defaultStereoShift and FovAngle

#include <QTimer>
#include <QUdpSocket>
#include <QNetworkDatagram>
#include <QFileSystemWatcher>
#include <QFile>
#include <QFileInfo>
#include <QSettings>
#include <QtDebug>

#define TRACE_CONFIGRECEIVER
#ifdef  TRACE_CONFIGRECEIVER
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

ConfigReceiver::ConfigReceiver(QObject *parent)
    : QObject(parent)
    , m_active(false)
    , m_rotateDisplay(0)
    , m_enableCompass(false)
    , m_screenSaver(defaultScreenSaver)
    , m_adjustAngle(0)
    , m_fovAngle(PanoramaView::FovDef)
    , m_stereoShift(PanoramaView::defaultStereoShift)
    , m_udpPort(defaultUdpPort)
    , m_udpSocket(nullptr)
    , m_fileWatcher(new QFileSystemWatcher(this))
{
    TRACE();
    connect(m_fileWatcher, &QFileSystemWatcher::fileChanged, this, &ConfigReceiver::onFileWatcher);
}

ConfigReceiver::~ConfigReceiver()
{
    QSettings settings;
    if (m_adjustAngle != 0)
        settings.setValue("AdjustAngle", m_adjustAngle);

    if (m_fovAngle != PanoramaView::FovDef)
        settings.setValue("FovAngle", m_fovAngle);

    qreal value = roundTo2(m_stereoShift);
    if (value != PanoramaView::defaultStereoShift)
        settings.setValue("StereoShift", value);
}

bool ConfigReceiver::active() const
{
    return m_active;
}

void ConfigReceiver::setActive(bool yes)
{
    TRACE_ARG(yes);
    if (yes == m_active) return;
    if (yes) {
        QTimer::singleShot(0, this, &ConfigReceiver::onActiveTimer);
        return;
    }
    if (m_udpSocket) {
        m_udpSocket->deleteLater();
        m_udpSocket = nullptr;
    }
    auto files = m_fileWatcher->files();
    if (!files.isEmpty())
        m_fileWatcher->removePaths(files);

    m_active = false;
    emit activeChanged();
}

int ConfigReceiver::udpPort() const
{
    return m_udpPort;
}

void ConfigReceiver::setUdpPort(int port)
{
    TRACE_ARG(port);
    int num = qBound(1024, port, 65535);
    if (num != m_udpPort) {
        m_udpPort = num;
        emit udpPortChanged();
    }
}

QString ConfigReceiver::watchForVideo() const
{
    return m_watchForVideo;
}

void ConfigReceiver::setWatchForVideo(const QString &path)
{
    TRACE_ARG(path);
    if (path.isEmpty() || !QFile::exists(path)) {
        qWarning() << Q_FUNC_INFO << "The watchForVideo file must exist" << path;
        return;
    }
    if (path != m_watchForVideo) {
        if (!m_watchForVideo.isEmpty())
            m_fileWatcher->removePath(m_watchForVideo);
        if (!m_fileWatcher->addPath(path)) {
            qWarning() << Q_FUNC_INFO << "Can't watch" << path;
            return;
        }
        m_watchForVideo = path;
        emit watchForVideoChanged();
    }
}

QString ConfigReceiver::watchForRotate() const
{
    return m_watchForRotate;
}

void ConfigReceiver::setWatchForRotate(const QString &path)
{
    TRACE_ARG(path);
    if (path.isEmpty() || !QFile::exists(path)) {
        qWarning() << Q_FUNC_INFO << "The watchForRotate file must exist" << path;
        return;
    }
    if (path != m_watchForRotate) {
        if (!m_watchForRotate.isEmpty())
            m_fileWatcher->removePath(m_watchForRotate);
        if (!m_fileWatcher->addPath(path)) {
            qWarning() << Q_FUNC_INFO << "Can't watch" << path;
            return;
        }
        m_watchForRotate = path;
        emit watchForVideoChanged();
    }
}

QString ConfigReceiver::watchForCompass() const
{
    return m_watchForCompass;
}

void ConfigReceiver::setWatchForCompass(const QString &path)
{
    TRACE_ARG(path);
    if (path.isEmpty() || !QFile::exists(path)) {
        qWarning() << Q_FUNC_INFO << "The watchForCompass file must exist" << path;
        return;
    }
    if (path != m_watchForCompass) {
        if (!m_watchForCompass.isEmpty())
            m_fileWatcher->removePath(m_watchForCompass);
        if (!m_fileWatcher->addPath(path)) {
            qWarning() << Q_FUNC_INFO << "Can't watch" << path;
            return;
        }
        m_watchForCompass = path;
        emit watchForVideoChanged();
    }
}

QString ConfigReceiver::watchForSaver() const
{
    return m_watchForSaver;
}

void ConfigReceiver::setWatchForSaver(const QString &path)
{
    TRACE_ARG(path);
    if (path.isEmpty() || !QFile::exists(path)) {
        qWarning() << Q_FUNC_INFO << "The watchForSaver file must exist" << path;
        return;
    }
    if (path != m_watchForSaver) {
        if (!m_watchForSaver.isEmpty())
            m_fileWatcher->removePath(m_watchForSaver);
        if (!m_fileWatcher->addPath(path)) {
            qWarning() << Q_FUNC_INFO << "Can't watch" << path;
            return;
        }
        m_watchForSaver = path;
        emit watchForSaverChanged();
    }
}

QString ConfigReceiver::watchForSmoothing() const
{
    return m_watchForSmoothing;
}

void ConfigReceiver::setWatchForSmoothing(const QString &path)
{
    TRACE_ARG(path);
    if (path.isEmpty() || !QFile::exists(path)) {
        qWarning() << Q_FUNC_INFO << "The watchForSmoothing file must exist" << path;
        return;
    }
    if (path != m_watchForSmoothing) {
        if (!m_watchForSmoothing.isEmpty())
            m_fileWatcher->removePath(m_watchForSmoothing);
        if (!m_fileWatcher->addPath(path)) {
            qWarning() << Q_FUNC_INFO << "Can't watch" << path;
            return;
        }
        m_watchForSmoothing = path;
        emit watchForSmoothingChanged();
    }
}

QString ConfigReceiver::watchForPitchIdleAngle() const
{
    return m_watchForPitchIdleAngle;
}

void ConfigReceiver::setWatchForPitchIdleAngle(const QString &path)
{
    TRACE_ARG(path);
    if (path.isEmpty() || !QFile::exists(path)) {
        qWarning() << Q_FUNC_INFO << "The watchForPitchIdleAngle file must exist" << path;
        return;
    }
    if (path != m_watchForPitchIdleAngle) {
        if (!m_watchForPitchIdleAngle.isEmpty())
            m_fileWatcher->removePath(m_watchForPitchIdleAngle);
        if (!m_fileWatcher->addPath(path)) {
            qWarning() << Q_FUNC_INFO << "Can't watch" << path;
            return;
        }
        m_watchForPitchIdleAngle = path;
        emit watchForPitchIdleAngleChanged();
    }
}

QString ConfigReceiver::videoSource() const
{
    return m_videoSource;
}

int ConfigReceiver::rotateDisplay() const
{
    return m_rotateDisplay;
}

bool ConfigReceiver::enableCompass() const
{
    return m_enableCompass;
}

int ConfigReceiver::screenSaver() const
{
    return m_screenSaver;
}

int ConfigReceiver::adjustAngle() const
{
    return m_adjustAngle;
}

int ConfigReceiver::fovAngle() const
{
    return m_fovAngle;
}

qreal ConfigReceiver::stereoShift() const
{
    return m_stereoShift;
}

qreal ConfigReceiver::smoothingFactor() const
{
    return m_smoothingFactor;
}

int ConfigReceiver::pitchIdleAngle() const
{
    return m_pitchIdleAngle;
}

void ConfigReceiver::onFileWatcher(const QString &path)
{
    TRACE_ARG(path);
    if (path.isEmpty()) return;
    bool video     = (path == m_watchForVideo);
    bool rotate    = (path == m_watchForRotate);
    bool compass   = (path == m_watchForCompass);
    bool saver     = (path == m_watchForSaver);
    bool smoothing = (path == m_watchForSmoothing);
    bool pitchIdleAngle = (path == m_watchForPitchIdleAngle);

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly) || file.size() < 1) {
        if (video && !m_videoSource.isEmpty()) {
            m_videoSource.clear();
            emit videoSourceChanged();
        }
        if (rotate && m_rotateDisplay) {
            m_rotateDisplay = 0;
            emit rotateDisplayChanged();
        }
        if (compass && m_enableCompass) {
            m_enableCompass = false;
            emit enableCompassChanged();
        }
        if (compass && m_enableCompass) {
            m_enableCompass = false;
            emit enableCompassChanged();
        }
        if (saver && m_screenSaver) {
            m_screenSaver = defaultScreenSaver;
            emit enableCompassChanged();
        }
        if (smoothing && m_smoothingFactor) {
            m_smoothingFactor = 0;
            emit smoothingFactorChanged();
        }
        if (pitchIdleAngle && m_pitchIdleAngle) {
            m_pitchIdleAngle = 0;
            emit pitchIdleAngleChanged();
        }
        return;
    }
    if (file.size() > 1024) {
        qWarning() << Q_FUNC_INFO << "The watchForVideo seems to be wrong file";
        return;
    }

    // String values first
    QString value = QString::fromUtf8(file.readAll().trimmed());
    qDebug() << value;
    file.close();
    if (video) {
        QString src = QFileInfo(path).path() + '/' + videoSubdir + '/' + value;
        if (src != m_videoSource) {
            m_videoSource = src;
            emit videoSourceChanged();
        }
        return;
    }

    // Next numberic values
    int num = value.toInt();
    if (rotate) {
        int dir = qBound(-1, num, 1);
        if (dir != m_rotateDisplay) {
            m_rotateDisplay = dir;
            emit rotateDisplayChanged();
        }
        return;
    }
    if (compass) {
        int yes = qBound(0, num, 1);
        if (yes != m_enableCompass) {
            m_enableCompass = yes;
            emit enableCompassChanged();
        }
        return;
    }
    if (saver) {
        int min = qBound(1, num, 60);
        if (min != m_screenSaver) {
            m_screenSaver = min;
            emit screenSaverChanged();
        }
        return;
    }
    if (pitchIdleAngle) {
        int degree = qBound(-90, num, 90);
        if (degree != m_pitchIdleAngle) {
            m_pitchIdleAngle = degree;
            emit pitchIdleAngleChanged();
        }
    }

    // Floating-point values next
    double decimal = value.toDouble();
    if (smoothing) {
        double t = qBound(0.0, decimal, 1.0);
        if (t != m_smoothingFactor) {
            m_smoothingFactor = t;
            emit smoothingFactorChanged();
        }
    }
}

void ConfigReceiver::onActiveTimer()
{
    TRACE();
    if (m_udpPort < 1024 || m_udpPort > 65535) {
        qWarning() << Q_FUNC_INFO << "Bad port number" << m_udpPort;
        return;
    }
    if (!m_udpSocket || m_udpSocket->localPort() != m_udpPort) {
        if (m_udpSocket) m_udpSocket->deleteLater();
        m_udpSocket = new QUdpSocket(this);
        m_udpSocket->bind(QHostAddress::LocalHost, m_udpPort);
        connect(m_udpSocket, &QUdpSocket::readyRead, this, &ConfigReceiver::onUdpReadyRead);
    }
    onFileWatcher(m_watchForRotate);
    onFileWatcher(m_watchForCompass);
    onFileWatcher(m_watchForVideo);
    onFileWatcher(m_watchForSaver);
    onFileWatcher(m_watchForSmoothing);
    onFileWatcher(m_watchForPitchIdleAngle);

    QSettings settings;
    int num = settings.value("AdjustAngle", 0).toInt();
    int angle = qBound(-180, num, 180);
    if (angle != m_adjustAngle) {
        m_adjustAngle = angle;
        emit adjustAngleChanged();
    }
    num = settings.value("FovAngle", PanoramaView::FovDef).toInt();
    angle = qBound(int(PanoramaView::FovMin), num, int(PanoramaView::FovMax));
    if (angle != m_fovAngle) {
        m_fovAngle = angle;
        emit fovAngleChanged();
    }
    qreal val = settings.value("StereoShift", PanoramaView::defaultStereoShift).toDouble();
    qreal shift = roundTo2(qBound(0.0, val, 1.0));
    if (shift != m_stereoShift) {
        m_stereoShift = shift;
        emit stereoShiftChanged();
    }
    if (!m_active) {
        m_active = true;
        emit activeChanged();
    }
}

void ConfigReceiver::onUdpReadyRead()
{
    bool calibrate_req = false;
    bool reset_req = false;
    char buf[1500];
    while (m_udpSocket->hasPendingDatagrams()) {
        int len = m_udpSocket->readDatagram(buf, sizeof(buf)-1);
        if (len < 8) {
            qWarning() << Q_FUNC_INFO << "Garbage packet received" << len;
            continue;
        }
        buf[len] = '\0';
        QString token = QString(buf).trimmed().toLower();
        TRACE_ARG(token);
        if (token == "calibration") {
            calibrate_req = true;
            if (m_adjustAngle) {
                m_adjustAngle = 0;
                reset_req = true;
                emit adjustAngleChanged();
            }
            if (m_fovAngle != PanoramaView::FovDef) {
                m_fovAngle = PanoramaView::FovDef;
                reset_req = true;
                emit fovAngleChanged();
            }
            if (!qFuzzyCompare(m_stereoShift, PanoramaView::defaultStereoShift)) {
                m_stereoShift = PanoramaView::defaultStereoShift;
                reset_req = true;
                emit stereoShiftChanged();
            }
        } else if (token == "turn_left") {
            if (m_adjustAngle > -180) {
                m_adjustAngle -= 5;
                emit adjustAngleChanged();
            }
        } else if (token == "turn_right") {
            if (m_adjustAngle < 180) {
                m_adjustAngle += 5;
                emit adjustAngleChanged();
            }
        } else if (token == "zoom_plus") {
            if (m_fovAngle > PanoramaView::FovMin) {
                m_fovAngle -= 5;
                emit fovAngleChanged();
            }
        } else if (token == "zoom_minus") {
            if (m_fovAngle < PanoramaView::FovMax) {
                m_fovAngle += 5;
                emit fovAngleChanged();
            }
        } else if (token == "shift_lower") {
            if (m_stereoShift >= 0.1) {
                m_stereoShift -= 0.1;
                if (m_stereoShift < 0.1) m_stereoShift = 0.0;
                emit stereoShiftChanged();
            }
        } else if (token == "shift_upper") {
            if (m_stereoShift <= 0.9) {
                m_stereoShift += 0.1;
                emit stereoShiftChanged();
            }
        }
    }
    if (reset_req) {
        QSettings settings;
        settings.setValue("AdjustAngle", m_adjustAngle);
        settings.setValue("FovAngle", m_fovAngle);
        settings.setValue("StereoShift", m_stereoShift);
    }
    if (calibrate_req)
        emit calibrateRequested();
}
