#ifndef SERIALSENSOR_H
#define SERIALSENSOR_H

#include <QObject>
#include <QQmlEngine>
#include <QSerialPort>
#include <QPointer>

class SerialSensor : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool            active READ active          WRITE setActive          NOTIFY activeChanged FINAL)
    Q_PROPERTY(QString       portName READ portName        WRITE setPortName        NOTIFY portNameChanged FINAL)
    Q_PROPERTY(int           baudRate READ baudRate        WRITE setBaudRate        NOTIFY baudRateChanged FINAL)
    Q_PROPERTY(int          threshold READ threshold       WRITE setThreshold       NOTIFY thresholdChanged FINAL)
    Q_PROPERTY(int          frequency READ frequency       WRITE setFrequency       NOTIFY frequencyChanged FINAL)
    Q_PROPERTY(int          gyroscope READ gyroscope       WRITE setGyroscope       NOTIFY gyroscopeChanged FINAL)
    Q_PROPERTY(int      accelerometer READ accelerometer   WRITE setAccelerometer   NOTIFY accelerometerChanged FINAL)
    Q_PROPERTY(int       magnetometer READ magnetometer    WRITE setMagnetometer    NOTIFY magnetometerChanged FINAL)
    Q_PROPERTY(bool     enableCompass READ enableCompass   WRITE setEnableCompass   NOTIFY enableCompassChanged FINAL)
    Q_PROPERTY(int        adjustAngle READ adjustAngle     WRITE setAdjustAngle     NOTIFY adjustAngleChanged FINAL)
    Q_PROPERTY(double smoothingFactor READ smoothingFactor WRITE setSmoothingFactor NOTIFY smoothingFactorChanged FINAL)
    Q_PROPERTY(qreal       pitchAngle READ pitchAngle    NOTIFY pitchAngleChanged FINAL)
    Q_PROPERTY(qreal         yawAngle READ yawAngle      NOTIFY yawAngleChanged FINAL)
    Q_PROPERTY(QString      errorText READ errorText     NOTIFY errorTextChanged FINAL)
    Q_PROPERTY(bool       calibrating READ calibrating   NOTIFY calibratingChanged FINAL)
    QML_ELEMENT

public:
    static constexpr int const defaultThreshold     = 5;
    static constexpr int const defaultFrequency     = 30;
    static constexpr int const defaultGyroscope     = 1;
    static constexpr int const defaultAccelerometer = 3;
    static constexpr int const defaultMagnetometer  = 5;

    static constexpr int const cmdPacketBegin = 0x49; // Start code
    static constexpr int const cmdPacketEnd   = 0x4D; // End code
    // The maximum length of the data body of the data packet sent by the module
    static constexpr int const cmdPacketMaxDatSizeRx = 73;

    explicit SerialSensor(QObject *parent = nullptr);

    enum BaudRate {
        BaudRate9600   = 9600,
        BaudRate19200  = 19200,
        BaudRate38400  = 38400,
        BaudRate57600  = 57600,
        BaudRate115200 = 115200
    };
    Q_ENUM(BaudRate)

    bool active() const;
    void setActive(bool yes);

    QString portName() const;
    void setPortName(const QString &name);

    int baudRate() const;
    void setBaudRate(int rate); // enum BaudRate

    int threshold() const;
    void setThreshold(int accel);

    int frequency() const;
    void setFrequency(int hz); // 1..250Hz

    int gyroscope() const;
    void setGyroscope(int filter); // 0..2

    int accelerometer() const;
    void setAccelerometer(int filter); // 0..4

    int magnetometer() const;
    void setMagnetometer(int filter); // 0..9

    bool enableCompass() const;
    void setEnableCompass(bool yes);

    int adjustAngle() const;
    void setAdjustAngle(int angle); // -180..180 in degree

    double smoothingFactor() const;
    void setSmoothingFactor(double factor); // 0..1
    
    qreal pitchAngle() const;
    qreal yawAngle() const;

    QString errorText() const;
    bool calibrating() const;

    Q_INVOKABLE static QStringList allPortNames();

public slots:
    void startCalibrate();

signals:
    void activeChanged();
    void initialized();
    void portNameChanged();
    void baudRateChanged();
    void thresholdChanged();
    void frequencyChanged();
    void gyroscopeChanged();
    void accelerometerChanged();
    void magnetometerChanged();
    void enableCompassChanged();
    void adjustAngleChanged();
    void pitchAngleChanged();
    void yawAngleChanged();
    void errorTextChanged();
    void calibratingChanged();
    void smoothingFactorChanged();

private:
    void setPortError(QSerialPort::SerialPortError error);
    void setErrorText(const QString &text);
    void onReadyRead();
    void initSensor();
    int transmitData(const QByteArray &data);
    void unpackData(const quint8 *data, int size);
    void calibrateSequence();

    bool m_active;
    bool m_calibrate;
    bool m_enableCompass;
    int m_adjustAngle;
    qreal m_pitchAngle;
    qreal m_yawAngle;
    qreal m_rollAngle;
    QString m_errorText;
    double m_smoothingFactor;

    QSerialPort m_serialPort;
    int m_threshold;
    int m_frequency;
    int m_gyroscope;
    int m_accelerometer;
    int m_magnetometer;

    int m_globalCS; // Checksum
    int m_globalI;
    int m_rxIndex;
    quint8 m_buf[5 + cmdPacketMaxDatSizeRx]; // Receive packet cache
    int m_cmdLen; // length
};

#endif // SERIALSENSOR_H
