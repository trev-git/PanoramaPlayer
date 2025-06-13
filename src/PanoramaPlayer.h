#ifndef PANORAMAPLAYER_H
#define PANORAMAPLAYER_H

#include <QObject>
#include <QQmlEngine>
#include <QUrl>
#include <QSize>
#include <QMediaPlayer>
//#include <QMediaMetaData>
#include <QAudioOutput>
#include <QPointer>

class QTimer;
class QVideoSink;
class QVideoFrame;
class PanoramaView;

class PanoramaPlayer : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QUrl               source READ source       WRITE setSource      NOTIFY sourceChanged FINAL)
    Q_PROPERTY(QAudioOutput* audioOutput READ audioOutput  WRITE setAudioOutput NOTIFY audioOutputChanged FINAL)
    Q_PROPERTY(QObject*      videoOutput READ videoOutput  WRITE setVideoOutput NOTIFY videoOutputChanged FINAL)
    Q_PROPERTY(int            mediaState READ mediaState   NOTIFY mediaStateChanged FINAL)
    Q_PROPERTY(QString         errorText READ errorText    NOTIFY errorTextChanged FINAL)
    //Q_PROPERTY(QMediaMetaData   metaData READ metaData     NOTIFY metaDataChanged FINAL)
    Q_PROPERTY(QSize           videoSize READ videoSize    NOTIFY videoSizeChanged FINAL)

public:
    explicit PanoramaPlayer(QObject *parent = nullptr);

    enum MediaState {
        MediaUnknown,
        MediaReady,
        MediaEnd,
    };
    Q_ENUM(MediaState)

    QUrl source() const;
    void setSource(const QUrl &url);

    QAudioOutput *audioOutput() const;
    void setAudioOutput(QAudioOutput *item);

    QObject *videoOutput() const;
    void setVideoOutput(QObject *item);

    int mediaState() const;
    QString errorText() const;
    //QMediaMetaData metaData() const;
    QSize videoSize() const;

    Q_INVOKABLE bool isPlaying() const;
    Q_INVOKABLE static QStringList allVideoFiles(const QString &path); // folder and optional fileMask

public slots:
    void play();
    void pause();
    void stop();

signals:
    void sourceChanged();
    void audioOutputChanged();
    void videoOutputChanged();
    void mediaStateChanged();
    void errorTextChanged();
    //void metaDataChanged();
    void videoSizeChanged();

private:
    void setMediaState();
    void setErrorText(const QString &text);
    void onMediaStatusChanged(QMediaPlayer::MediaStatus status);
    void onVideoFrameChanged(const QVideoFrame &frame);

    QMediaPlayer *m_mediaPlayer;
    QVideoSink *m_videoSink;
    QTimer *m_statusTimer;
    int m_mediaState;
    QString m_errorText;
    QPointer<PanoramaView> m_viewItem;
};

#endif // PANORAMAPLAYER_H
