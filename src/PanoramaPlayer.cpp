#include "PanoramaPlayer.h"
#include "PanoramaView.h"

#include <QVideoSink>
#include <QVideoFrame>
#include <QTimer>
#include <QDir>
#include <QFileInfo>
#include <QtDebug>

//#define TRACE_PANORAMAPLAYER
#ifdef  TRACE_PANORAMAPLAYER
#include <QTime>
#include <QThread>
#define TRACE()      qDebug() << QTime::currentTime().toString("hh:mm:ss.zzz") << QThread::currentThreadId() << Q_FUNC_INFO;
#define TRACE_ARG(x) qDebug() << QTime::currentTime().toString("hh:mm:ss.zzz") << QThread::currentThreadId() << Q_FUNC_INFO << x;
#else
#define TRACE()
#define TRACE_ARG(x)
#endif

PanoramaPlayer::PanoramaPlayer(QObject *parent)
    : QObject(parent)
    , m_mediaPlayer(new QMediaPlayer(this))
    , m_videoSink(new QVideoSink(this))
    , m_statusTimer(nullptr)
    , m_mediaState(MediaUnknown)
{
    TRACE();
    m_mediaPlayer->setLoops(QMediaPlayer::Infinite);
    m_mediaPlayer->setVideoSink(m_videoSink);

    connect(m_mediaPlayer, &QMediaPlayer::sourceChanged, this, &PanoramaPlayer::sourceChanged);
    connect(m_mediaPlayer, &QMediaPlayer::audioOutputChanged, this, &PanoramaPlayer::audioOutputChanged);
    connect(m_mediaPlayer, &QMediaPlayer::mediaStatusChanged, this, &PanoramaPlayer::onMediaStatusChanged);
    connect(m_mediaPlayer, &QMediaPlayer::errorOccurred, this, [this](QMediaPlayer::Error, const QString &text) {
        setErrorText(source().toDisplayString() + ": " + text);
    });
    //connect(m_mediaPlayer, &QMediaPlayer::metaDataChanged, this, &PanoramaPlayer::metaDataChanged);

    connect(m_videoSink, &QVideoSink::videoFrameChanged, this, &PanoramaPlayer::onVideoFrameChanged);
    connect(m_videoSink, &QVideoSink::videoSizeChanged, this, &PanoramaPlayer::videoSizeChanged);
}

QUrl PanoramaPlayer::source() const
{
    return m_mediaPlayer->source();
}

void PanoramaPlayer::setSource(const QUrl &url)
{
    TRACE_ARG(url);
    onMediaStatusChanged(QMediaPlayer::NoMedia);
    m_mediaPlayer->setSource(url);
}

QAudioOutput *PanoramaPlayer::audioOutput() const
{
    return m_mediaPlayer->audioOutput();
}

void PanoramaPlayer::setAudioOutput(QAudioOutput *item)
{
    TRACE_ARG(item);
    m_mediaPlayer->setAudioOutput(item);
}

QObject *PanoramaPlayer::videoOutput() const
{
    return m_viewItem.data();
}

void PanoramaPlayer::setVideoOutput(QObject *item)
{
    TRACE_ARG(item);
    if (item) {
        if (item == m_viewItem.data()) return;
        m_viewItem = qobject_cast<PanoramaView *>(item);
        if (m_viewItem.isNull()) {
            qWarning() << Q_FUNC_INFO << "Bad output, VideoItem expected";
            return;
        }
    } else {
        if (m_viewItem.isNull()) return;
        m_viewItem.clear();
    }
    emit videoOutputChanged();
}

int PanoramaPlayer::mediaState() const
{
    return m_mediaState;
}

void PanoramaPlayer::onMediaStatusChanged(QMediaPlayer::MediaStatus status)
{
    TRACE_ARG(status);
    if (status == QMediaPlayer::StalledMedia ||
            status == QMediaPlayer::BufferingMedia ||
            status == QMediaPlayer::BufferedMedia) return;
    if (!m_statusTimer) {
        m_statusTimer = new QTimer(this);
        m_statusTimer->setSingleShot(true);
        m_statusTimer->setInterval(0);
        connect(m_statusTimer, &QTimer::timeout, this, &PanoramaPlayer::setMediaState);
    }
    if (!m_statusTimer->isActive())
        m_statusTimer->start();
}

void PanoramaPlayer::setMediaState()
{
    TRACE();
    int prev_state = m_mediaState;
    switch (m_mediaPlayer->mediaStatus()) {
    case QMediaPlayer::NoMedia:
    case QMediaPlayer::LoadingMedia:
    case QMediaPlayer::InvalidMedia:
        m_mediaState = MediaUnknown;
        break;
    case QMediaPlayer::LoadedMedia:
        m_mediaState = MediaReady;
        break;
    case QMediaPlayer::EndOfMedia:
        m_mediaState = MediaEnd;
        break;
    case QMediaPlayer::StalledMedia:
    case QMediaPlayer::BufferingMedia:
    case QMediaPlayer::BufferedMedia:
        return;
    }
    if (m_mediaState != prev_state)
        emit mediaStateChanged();
}

QString PanoramaPlayer::errorText() const
{
    return m_errorText;
}

void PanoramaPlayer::setErrorText(const QString &text)
{
    TRACE_ARG(text);
    if (text != m_errorText) {
        m_errorText = text;
        emit errorTextChanged();
    }
}

/*QMediaMetaData PanoramaPlayer::metaData() const
{
    return m_mediaPlayer->metaData();
}*/

QSize PanoramaPlayer::videoSize() const
{
    return m_videoSink->videoSize();
}

void PanoramaPlayer::play()
{
    TRACE();
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
    if (m_mediaPlayer->isPlaying()) return;
#endif
    if (!m_mediaPlayer->isAvailable()) {
        qWarning() << Q_FUNC_INFO << "The media player is not supported on this platform";
        return;
    }
    if (m_mediaState == MediaUnknown) {
        qWarning() << Q_FUNC_INFO << "The media is not ready for playback";
        return;
    }
    m_mediaPlayer->play();
}

void PanoramaPlayer::stop()
{
    TRACE();
    m_mediaPlayer->stop();
}

void PanoramaPlayer::pause()
{
    TRACE();
    m_mediaPlayer->pause();
}

bool PanoramaPlayer::isPlaying() const
{
    return m_mediaPlayer->isPlaying();
}

//static
QStringList PanoramaPlayer::allVideoFiles(const QString &path)
{
    if (path.isEmpty()) return QStringList();
    QString mask = path.startsWith('~') ? QDir::homePath() + path.mid(1) : path;
    QDir dir(mask);
    QStringList nameFilters;
    if (!dir.exists()) {
        QFileInfo info(mask);
        dir.setPath(info.path());
        if (!dir.exists()) {
            qWarning() << Q_FUNC_INFO << dir.path() << QStringLiteral("No such directory");
            return QStringList();
        }
        nameFilters.append(info.fileName());
    } else nameFilters.append("*.mp4");

    QStringList files, list = dir.entryList(nameFilters, QDir::Files | QDir::Readable, QDir::Time);
    for (int i = 0; i < list.size(); i++) {
        files.append(dir.filePath(list.at(i)));
    }
    return files;
}

void PanoramaPlayer::onVideoFrameChanged(const QVideoFrame &frame)
{
    TRACE_ARG(frame);
    if (!m_viewItem.isNull())
        m_viewItem->setVideoFrame(frame);
}
