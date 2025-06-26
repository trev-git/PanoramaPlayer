#ifndef CONFIGRECEIVER_H
#define CONFIGRECEIVER_H

#include <QObject>
#include <QQmlComponent>
#include <qtmetamacros.h>

class QUdpSocket;
class QFileSystemWatcher;

class ConfigReceiver : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(bool               active READ active            WRITE setActive            NOTIFY activeChanged FINAL)
    Q_PROPERTY(int               udpPort READ udpPort           WRITE setUdpPort           NOTIFY udpPortChanged FINAL)
    Q_PROPERTY(QString     watchForVideo READ watchForVideo     WRITE setWatchForVideo     NOTIFY watchForVideoChanged FINAL)
    Q_PROPERTY(QString    watchForRotate READ watchForRotate    WRITE setWatchForRotate    NOTIFY watchForRotateChanged FINAL)
    Q_PROPERTY(QString   watchForCompass READ watchForCompass   WRITE setWatchForCompass   NOTIFY watchForCompassChanged FINAL)
    Q_PROPERTY(QString     watchForSaver READ watchForSaver     WRITE setWatchForSaver     NOTIFY watchForSaverChanged FINAL)
    Q_PROPERTY(QString watchForSmoothing READ watchForSmoothing WRITE setWatchForSmoothing NOTIFY watchForSmoothingChanged FINAL)
    Q_PROPERTY(QString watchForPitchIdleAngle READ watchForPitchIdleAngle WRITE setWatchForPitchIdleAngle NOTIFY watchForPitchIdleAngleChanged FINAL)
    Q_PROPERTY(QString       videoSource READ videoSource     NOTIFY videoSourceChanged FINAL)
    Q_PROPERTY(int         rotateDisplay READ rotateDisplay   NOTIFY rotateDisplayChanged FINAL)
    Q_PROPERTY(bool        enableCompass READ enableCompass   NOTIFY enableCompassChanged FINAL)
    Q_PROPERTY(int           screenSaver READ screenSaver     NOTIFY screenSaverChanged FINAL)
    Q_PROPERTY(int           adjustAngle READ adjustAngle     NOTIFY adjustAngleChanged FINAL)
    Q_PROPERTY(int              fovAngle READ fovAngle        NOTIFY fovAngleChanged FINAL)
    Q_PROPERTY(qreal         stereoShift READ stereoShift     NOTIFY stereoShiftChanged FINAL)
    Q_PROPERTY(double    smoothingFactor READ smoothingFactor NOTIFY smoothingFactorChanged FINAL)
    Q_PROPERTY(int pitchIdleAngle READ pitchIdleAngle NOTIFY pitchIdleAngleChanged FINAL)

public:
    static constexpr char const *videoSubdir = "player";
    static constexpr int const defaultUdpPort = 12345;
    static constexpr int const defaultScreenSaver = 5; // minutes

    explicit ConfigReceiver(QObject *parent = nullptr);
    ~ConfigReceiver() override;

    bool active() const;
    void setActive(bool yes);

    int udpPort() const;
    void setUdpPort(int port);

    QObject *videoOutput() const;
    void setVideoOutput(QObject *item);

    QString watchForVideo() const;
    void setWatchForVideo(const QString &path);

    QString watchForRotate() const;
    void setWatchForRotate(const QString &path);

    QString watchForCompass() const;
    void setWatchForCompass(const QString &path);

    QString watchForSaver() const;
    void setWatchForSaver(const QString &path);

    QString watchForSmoothing() const;
    void setWatchForSmoothing(const QString &path);

    QString watchForPitchIdleAngle() const;
    void setWatchForPitchIdleAngle(const QString &path);

    QString videoSource() const;
    int rotateDisplay() const; // -1/0/1
    bool enableCompass() const;
    int screenSaver() const;
    int adjustAngle() const;
    int fovAngle() const;
    qreal stereoShift() const;
    double smoothingFactor() const;
    int pitchIdleAngle() const;

signals:
    void activeChanged();
    void udpPortChanged();
    void videoOutputChanged();
    void watchForVideoChanged();
    void watchForRotateChanged();
    void watchForCompassChanged();
    void watchForSaverChanged();
    void watchForSmoothingChanged();
    void watchForPitchIdleAngleChanged();

    void videoSourceChanged();
    void rotateDisplayChanged();
    void enableCompassChanged();
    void screenSaverChanged();
    void adjustAngleChanged();
    void smoothingFactorChanged();
    void pitchIdleAngleChanged();

    void fovAngleChanged();
    void stereoShiftChanged();
    void calibrateRequested();

private:
    void onFileWatcher(const QString &path);
    void onActiveTimer();
    void onUdpReadyRead();

    bool m_active;
    QString m_watchForVideo;
    QString m_watchForRotate;
    QString m_watchForCompass;
    QString m_watchForSaver;
    QString m_watchForSmoothing;
    QString m_watchForPitchIdleAngle;

    QString m_videoSource;
    int m_rotateDisplay;
    bool m_enableCompass;
    int m_screenSaver;
    double m_smoothingFactor;
    int m_pitchIdleAngle;

    int m_adjustAngle;
    int m_fovAngle;
    qreal m_stereoShift;

    int m_udpPort;
    QUdpSocket *m_udpSocket;
    QFileSystemWatcher *m_fileWatcher;
};

#endif // CONFIGRECEIVER_H
