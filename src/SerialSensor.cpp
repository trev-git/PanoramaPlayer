#include "SerialSensor.h"
#include "Lerp.h"

#include <QSerialPortInfo>
#include <QByteArray>
#include <QTimer>
#include <QMetaMethod>
#include <QtDebug>

// round up to 2 digit after point
static qreal roundTo2(qreal value)
{
    return int(value * 100.0 + 0.5) / 100.0;
}

SerialSensor::SerialSensor(QObject *parent)
    : QObject{parent}
    , m_active(false)
    , m_calibrate(false)
    , m_enableCompass(false)
    , m_adjustAngle(0)
    , m_pitchAngle(0.0)
    , m_yawAngle(0.0)
    , m_threshold(defaultThreshold)
    , m_frequency(defaultFrequency)
    , m_gyroscope(defaultGyroscope)
    , m_accelerometer(defaultAccelerometer)
    , m_magnetometer(defaultMagnetometer)
    , m_globalCS(0)
    , m_globalI(0)
    , m_rxIndex(0)
    , m_cmdLen(0)
    // , m_kfYaw(0.1, 0.01, 0.1)
    // , m_kfPitch(0.1, 0.01, 0.1)
{
    m_serialPort.setReadBufferSize(sizeof(m_buf));
    m_serialPort.setBaudRate(BaudRate115200);
    connect(&m_serialPort, &QSerialPort::readyRead, this, &SerialSensor::onReadyRead);
    connect(&m_serialPort, &QSerialPort::errorOccurred, this, &SerialSensor::setPortError);
    connect(&m_serialPort, &QSerialPort::baudRateChanged, this, &SerialSensor::baudRateChanged);
}

bool SerialSensor::active() const
{
    return m_active;
}

void SerialSensor::setActive(bool yes)
{
    if (yes == m_active) return;
    if (m_calibrate) {
        qWarning() << Q_FUNC_INFO << "Try again after completing the calibration";
        return;
    }
    if (yes) {
        QTimer::singleShot(200, this, &SerialSensor::initSensor);
        return;
    }
    if (m_serialPort.isOpen()) {
        m_serialPort.clear();
        m_serialPort.close();
    }
    m_active = false;
    emit activeChanged();
}

qreal SerialSensor::pitchAngle() const
{
    // m_pitchAngle = m_kfPitch.updateEstimate(m_pitchAngle);
    return m_pitchAngle;
}

qreal SerialSensor::yawAngle() const
{
    // m_yawAngle = m_kfYaw.updateEstimate(m_pitchAngle);
    return m_yawAngle;
}

QString SerialSensor::portName() const
{
    return m_serialPort.portName();
}

void SerialSensor::setPortName(const QString &name)
{
    bool was_active = m_active;
    if (was_active) setActive(false);
    if (name.isEmpty()) return;

    m_serialPort.setPortName(name);
    if (!m_serialPort.open(QSerialPort::ReadWrite)) {
        qWarning() << Q_FUNC_INFO << "Can't open" << name;
        return;
    }
    if (was_active) setActive(true);
    emit portNameChanged();
}

int SerialSensor::baudRate() const
{
    return m_serialPort.baudRate();
}

void SerialSensor::setBaudRate(int rate) // enum BaudRate
{
    switch (rate) {
    case BaudRate9600:
    case BaudRate19200:
    case BaudRate38400:
    case BaudRate57600:
    case BaudRate115200:
        m_serialPort.setBaudRate(rate);
        return;
    }
    qWarning() << Q_FUNC_INFO << "Invalid baudRate" << rate;
}

bool SerialSensor::enableCompass() const
{
    return m_enableCompass;
}

void SerialSensor::setEnableCompass(bool yes)
{
    if (yes != m_enableCompass) {
        if (m_active) {
            setActive(false);
            setActive(true);
        }
        m_enableCompass = yes;
        emit enableCompassChanged();
    }
}

int SerialSensor::adjustAngle() const
{
    return m_adjustAngle;
}

void SerialSensor::setAdjustAngle(int angle)
{
    int degree = qBound(-180, angle, 180);
    if (degree != m_adjustAngle) {
        int diff = m_adjustAngle - degree;
        m_adjustAngle = degree;
        m_yawAngle -= diff;
        emit adjustAngleChanged();
        emit yawAngleChanged();
    }
}

int SerialSensor::threshold() const
{
    return m_threshold;
}

void SerialSensor::setThreshold(int accel)
{
    if (accel != m_threshold) {
        m_threshold = accel;
        emit thresholdChanged();
    }
}

int SerialSensor::frequency() const
{
    return m_frequency;
}

void SerialSensor::setFrequency(int hz)
{
    int freq = qBound(1, hz, 250);
    if (freq != m_frequency) {
        m_frequency = freq;
        emit frequencyChanged();
    }
}

int SerialSensor::gyroscope() const
{
    return m_gyroscope;
}

void SerialSensor::setGyroscope(int filter)
{
    int coef = qBound(0, filter, 2);
    if (coef != m_gyroscope) {
        m_gyroscope = coef;
        emit gyroscopeChanged();
    }
}

int SerialSensor::accelerometer() const
{
    return m_accelerometer;
}

void SerialSensor::setAccelerometer(int filter)
{
    int coef = qBound(0, filter, 4);
    if (coef != m_accelerometer) {
        m_accelerometer = coef;
        emit accelerometerChanged();
    }
}

int SerialSensor::magnetometer() const
{
    return m_magnetometer;
}

void SerialSensor::setMagnetometer(int filter)
{
    int coef = qBound(0, filter, 9);
    if (coef != m_magnetometer) {
        m_magnetometer = coef;
        emit magnetometerChanged();
    }
}

double SerialSensor::smoothingFactor() const
{
    return m_smoothingFactor;
}

void SerialSensor::setSmoothingFactor(double factor)
{
    if (factor != m_smoothingFactor)
    {
        m_smoothingFactor = factor;
        emit smoothingFactorChanged();
    }
}

QString SerialSensor::errorText() const
{
    return m_errorText;
}

bool SerialSensor::calibrating() const
{
    return m_calibrate;
}

void SerialSensor::setPortError(QSerialPort::SerialPortError error)
{
    QString text;
    switch (error) {
    case QSerialPort::NoError:
        setErrorText(QString()); return;
    case QSerialPort::DeviceNotFoundError:
        text = QStringLiteral("Attempt to open a non-existing device"); break;
    case QSerialPort::PermissionError:
        text = QStringLiteral("An attempt to open an already opened device by another process or a user not having enough permission and credentials to open"); break;
    case QSerialPort::OpenError:
        text = QStringLiteral("An attempt to open an already opened device in this object"); break;
    case QSerialPort::NotOpenError:
        text = QStringLiteral("An attempt to perform an operation that can only be successfully performed if the device is open"); break;
    case QSerialPort::WriteError:
        text = QStringLiteral("An I/O error occurred while writing the data"); break;
    case QSerialPort::ReadError:
        text = QStringLiteral("An I/O error occurred while reading the data"); break;
    case QSerialPort::ResourceError:
        text = QStringLiteral("An I/O error occurred when a resource becomes unavailable, e.g. when the device is unexpectedly removed from the system"); break;
    case QSerialPort::UnsupportedOperationError:
        text = QStringLiteral("The requested device operation is not supported or prohibited by the running operating system"); break;
    case QSerialPort::TimeoutError:
        text = QStringLiteral("A timeout error occurred"); break;
    case QSerialPort::UnknownError:
        text = QStringLiteral("An unidentified error occurred"); break;
    }
    setErrorText(portName() + ": " + text);
}

void SerialSensor::setErrorText(const QString &text)
{
    if (text != m_errorText) {
        m_errorText = text;
        emit errorTextChanged();
    }
}

//static
QStringList SerialSensor::allPortNames()
{
    QStringList names;
    const auto list = QSerialPortInfo::availablePorts();
    for (const auto &info : list) names += info.portName();
    return names;
}

void SerialSensor::startCalibrate()
{
    if (!m_active) {
        qWarning() << Q_FUNC_INFO << "The sensor is currently inactive";
        return;
    }
    if (m_calibrate) {
        qWarning() << Q_FUNC_INFO << "The calibration already in progress";
        return;
    }
    connect(this, &SerialSensor::initialized, this, &SerialSensor::calibrateSequence);
    m_calibrate = true;
    initSensor();
    emit calibratingChanged();
}

/*
 * All of the following code supports only the Waveshare 10_OF_ROS_IMO_(A) sensor.
 * Keep in mind that no other sensor has been tested yet!
 */
void SerialSensor::initSensor()
{
    if (!m_serialPort.isOpen()) {
        qWarning() << Q_FUNC_INFO << "The serial port is not open";
        return;
    }

    // 1. Parameter settings
    // Whether to use magnetic field fusion 0: Not used 1: Used
    int isCompassOn = (!m_calibrate && m_enableCompass) ? 1 : 0;
    int barometerFilter = 2;
    int cmdReportTag = 0x7F;     // Feature subscription tag
    QByteArray params(11, '\0');
    params[0] = 0x12;
    params[1] = m_threshold;     // Stationary state acceleration threshold
    params[2] = 0xff;            // Static zero return speed (unit cm/s) 0: No return to zero 255: Return to zero immediately
    params[3] = 0;               // Dynamic zero return speed (unit cm/s) 0: No return to zero
    params[4] = ((barometerFilter & 3) << 1) | (isCompassOn & 1);
    params[5] = m_frequency;     // The transmission frame rate of data actively reported [value 0-250HZ], 0 means 0.5HZ
    params[6] = m_gyroscope;     // Gyroscope filter coefficient [value 0-2], the larger the value, the more stable it is but the worse the real-time performance.
    params[7] = m_accelerometer; // Accelerometer filter coefficient [value 0-4], the larger the value, the more stable it is but the worse the real-time performance.
    params[8] = m_magnetometer;  // Magnetometer filter coefficient [value 0-9], the larger the value, the more stable it is but the worse the real-time performance.
    params[9] = cmdReportTag & 0xff;
    params[10] = (cmdReportTag >> 8) & 0xff;
    transmitData(params);        // Send commands to sensors

    QTimer::singleShot(200, this, [this]() {
        // 2. Wake up sensor
        transmitData(QByteArray(1, 0x03));

        QTimer::singleShot(200, this, [this]() {
            // 3.Enable proactive reporting
            transmitData(QByteArray(1, 0x19));
            emit initialized(); // Can be connected to calibrateSequence()
        });
    });
    if (!m_active) {
        m_active = true;
        emit activeChanged();
    }
}

void SerialSensor::calibrateSequence()
{
    if (!m_calibrate) return; // should not happend

    // 4. Start magnetometer calibration
    transmitData(QByteArray(1, 0x32));

    // Make the Z axis of the module perpendicular to the horizontal plane and balance the Z axis to the horizontal plane,
    // and rotate it more than one turn on the horizontal plane (a few more turns will give better results).
    // Within 10 seconds, rotate the Z-axis of the module perpendicular to the horizontal plane and the Z-axis balanced on
    // the horizontal plane and rotate more than once in the horizontal plane.

    QTimer::singleShot(10000, this, [this]() {
        // 5. Finish magnetometer calibration
        transmitData(QByteArray(1, 0x04));

        // 6. Z axis angle reset to zero
        transmitData(QByteArray(1, 0x05));

        // 7. At least read data once by onReadyRead() signal

        // Calibration is completed, so disable this sequence for further normal operation.
        disconnect(this, &SerialSensor::initialized, this, &SerialSensor::calibrateSequence);
        m_calibrate = false;
        emit calibratingChanged();
    });
}

int SerialSensor::transmitData(const QByteArray &data)
{
    if (!m_serialPort.isOpen() || data.isEmpty() || data.size() > 19)
        return 0; // Illegal parameters

    // Build a send packet cache, including the 50-byte preamble
    QByteArray buf(46 + 7, '\0');
    buf[47] = buf[49] = buf[51] = 0xff;
    buf[50] = cmdPacketBegin;
    buf[52] = data.size();
    buf += data;

    // Calculate the checksum, starting from the address code to the end of the data body
    int sum = 0;
    for (int i = 51; i < buf.size(); i++) sum += buf.at(i);
    buf.append(sum & 0xff); // Take the lower 8 bits
    buf.append(cmdPacketEnd); // Add closing code

    // Send data
    return m_serialPort.write(buf);
}

void SerialSensor::onReadyRead()
{
    quint8 buf[sizeof(m_buf)];
    int len = m_serialPort.read(reinterpret_cast<char*>(buf), sizeof(buf));

    // Calculate the check code while receiving the data. The check code is the sum of the data from
    // the beginning of the address code (including the address code) to before the check code.
    for (int i = 0; i < len; i++) {
        m_globalCS += buf[i];
        switch (m_rxIndex) {
        case 0: // Start code
            if (buf[i] == cmdPacketBegin) {
                m_globalI = 0;
                m_buf[m_globalI++] = cmdPacketBegin;
                m_globalCS = 0; // Calculate the check code starting from the next byte
                m_rxIndex = 1;
            }
            break;
        case 1: // The address code of the data body
            m_buf[m_globalI++] = buf[i];
            if (buf[i] == 255) { // 255 is the broadcast address，module as slave，Its address cannot appear 255
                m_rxIndex = 0;
            } else {
                m_rxIndex++;
            }
            break;
        case 2: // The length of the data body
            m_buf[m_globalI++] = buf[i];
            if (buf[i] > cmdPacketMaxDatSizeRx || buf[i] == 0) { // Invalid length
                m_rxIndex = 0;
            } else {
                m_rxIndex++;
                m_cmdLen = buf[i];
            }
            break;
        case 3: // Get the data of the data body
            m_buf[m_globalI++] = buf[i];
            if (m_globalI >= m_cmdLen + 3) { // Data body has been received
                m_rxIndex++;
            }
            break;
        case 4: // Compare verification code
            m_globalCS -= buf[i];
            if ((m_globalCS & 0xff) == buf[i]) { // Verification is correct
                m_buf[m_globalI++] = buf[i];
                m_rxIndex++;
            } else { // Verification failed
                m_rxIndex = 0;
            }
            break;
        case 5: // End code
            m_rxIndex = 0;
            if (buf[i] == cmdPacketEnd) { // Get the complete package
                m_buf[m_globalI++] = buf[i];
                //hex_string = " ".join(f"{b:02X}" for b in buf[0:i])
                //print(f"U-Rx[Len={i}]:{hex_string}")
                //Cmd_RxUnpack(buf[3:i-2], i-5); // Process the data body of the packet
                unpackData(m_buf + 3, m_globalI - 5);
            }
            break;
        default:
            m_rxIndex = 0;
            break;
        }
    }
}

void SerialSensor::unpackData(const quint8 *buf, int size)
{
    static const qreal scaleAccel       = 0.00478515625;
    static const qreal scaleQuat        = 0.000030517578125;
    static const qreal scaleAngle       = 0.0054931640625;
    static const qreal scaleAngleSpeed  = 0.06103515625;
    static const qreal scaleMag         = 0.15106201171875;
    static const qreal scaleTemperature = 0.01;
    static const qreal scaleAirPressure = 0.0002384185791;
    static const qreal scaleHeight      = 0.0010728836;

    if (buf[0] != 0x11) {
        //qDebug() << Q_FUNC_INFO << "Data head not defined";
        return;
    }
    int ctl = (buf[2] << 8) | buf[1];
    int ms = (buf[6] << 24) | (buf[5] << 16) | (buf[4] << 8) | (buf[3] << 0);
    //qDebug() << "\n subscribe tag:" << QString::number(ctl, 16) << "\n ms:" << ms;

    qreal tmpX, tmpY, tmpZ;
    int len = 7; // Starting from the 7th byte, the remaining data is parsed according to the subscription identification tag.
    if ((ctl & 0x0001) != 0) { // Acceleration without gravity
        tmpX = short((short(buf[len + 1]) << 8) | buf[len]) * scaleAccel; len += 2;
        //qDebug() << "\taX:" << tmpX; // Acceleration ax without gravity
        tmpY = short((short(buf[len + 1]) << 8) | buf[len]) * scaleAccel; len += 2;
        //qDebug() << "\taY:" << tmpY; // Acceleration ay without gravity
        tmpZ = short((short(buf[len + 1]) << 8) | buf[len]) * scaleAccel; len += 2;
        //qDebug() << "\taZ:" << tmpZ; // Acceleration az without gravity
    }
    if ((ctl & 0x0002) != 0) { // Acceleration with gravity
        tmpX = short((short(buf[len + 1]) << 8) | buf[len]) * scaleAccel; len += 2;
        //qDebug() << "\tAX:" << tmpX; // Acceleration AX with gravity
        tmpY = short((short(buf[len + 1]) << 8) | buf[len]) * scaleAccel; len += 2;
        //qDebug() << "\tAY:" << tmpY; // Acceleration AY gravity
        tmpZ = short((short(buf[len + 1]) << 8) | buf[len]) * scaleAccel; len += 2;
        //qDebug() << "\tAZ:" << tmpZ; // Acceleration AZ gravity
    }
    if ((ctl & 0x0004) != 0) { // Angular velocity
        tmpX = short((short(buf[len + 1]) << 8) | buf[len]) * scaleAngleSpeed; len += 2;
        //qDebug() << "\tGX:" << tmpX; // Angular velocity GX
        tmpY = short((short(buf[len + 1]) << 8) | buf[len]) * scaleAngleSpeed; len += 2;
        //qDebug() << "\tGY:" << tmpY; // Angular velocity GY
        tmpZ = short((short(buf[len + 1]) << 8) | buf[len]) * scaleAngleSpeed; len += 2;
        //qDebug() << "\tGZ:" << tmpZ; // Angular velocity GZ
    }
    if ((ctl & 0x0008) != 0) { // Magnetic field
        tmpX = short((short(buf[len + 1]) << 8) | buf[len]) * scaleMag; len += 2;
        //qDebug() << "\tCX:" << tmpX; // Magnetic field data CX
        tmpY = short((short(buf[len + 1]) << 8) | buf[len]) * scaleMag; len += 2;
        //qDebug() << "\tCY:" << tmpY; // Magnetic field data CY
        tmpZ = short((short(buf[len + 1]) << 8) | buf[len]) * scaleMag; len += 2;
        //qDebug() << "\tCZ:" << tmpZ; // Magnetic field data CZ
    }
    if ((ctl & 0x0010) != 0) { // Temperature
        tmpX = short((short(buf[len + 1]) << 8) | buf[len]) * scaleTemperature; len += 2;
        //qDebug() << "\ttemperature:" << tmpX; // temperature

        quint32 tmpU32 = quint32((quint32(buf[len + 2]) << 16) | (quint32(buf[len + 1]) << 8) | quint32(buf[len]));
        if ((tmpU32 & 0x800000) == 0x800000) // If the highest bit of the 24-digit number is 1, the value is a negative number and needs to be converted to a 32-bit negative number, just add ff directly.
            tmpU32 = (tmpU32 | 0xff000000);
        tmpY = qint32(tmpU32) * scaleAirPressure; len += 3;
        //qDebug() << "\tairPressure:" << tmpY; // air pressure

        tmpU32 = quint32((quint32(buf[len + 2]) << 16) | (quint32(buf[len + 1]) << 8) | quint32(buf[len]));
        if ((tmpU32 & 0x800000) == 0x800000) // If the highest bit of the 24-digit number is 1, the value is a negative number and needs to be converted to a 32-bit negative number, just add ff directly.
            tmpU32 = (tmpU32 | 0xff000000);
        tmpZ = qint32(tmpU32) * scaleHeight; len += 3;
        //qDebug() << "\theight:" << tmpZ; // high
    }
    if ((ctl & 0x0020) != 0) { // Quaternions
        qreal tmpAbs = short((short(buf[len + 1]) << 8) | buf[len]) * scaleQuat; len += 2;
        //qDebug() << "\tQW:" << tmpAbs; // Quaternions w
        tmpX = short((short(buf[len + 1]) << 8) | buf[len]) * scaleQuat; len += 2;
        //qDebug() << "\tQX:" << tmpX; // Quaternions x
        tmpY =   short((short(buf[len + 1]) << 8) | buf[len]) * scaleQuat; len += 2;
        //qDebug() << "\tQY:" << tmpY; // Quaternions y
        tmpZ =   short((short(buf[len + 1]) << 8) | buf[len]) * scaleQuat; len += 2;
        //qDebug() << "\tQZ:" << tmpZ; // Quaternions z
    }
    if ((ctl & 0x0040) != 0) { // Euler angles
        tmpX = short((short(buf[len + 1]) << 8) | buf[len]) * scaleAngle; len += 2;
        //qDebug() << "\tangleX:" << tmpX; // Euler angles x
        tmpY = short((short(buf[len + 1]) << 8) | buf[len]) * scaleAngle; len += 2;
        //qDebug() << "\tangleY:" << tmpY; // Euler angles y
        tmpZ = short((short(buf[len + 1]) << 8) | buf[len]) * scaleAngle; len += 2;
        //qDebug() << "\tangleZ:" << tmpZ; // Euler angles z

        qreal pitch = roundTo2(qBound(-90.0, -tmpY, 90.0));
        qreal yaw = roundTo2(qBound(-180.0, -tmpZ, 180.0));

        bool pitch_changed = (pitch != m_pitchAngle);
        if (pitch_changed)
        {
            m_pitchAngle = repeat(lerpAngle(m_pitchAngle+180.0, pitch+180.0, m_smoothingFactor), 360.0) - 180.0;
        }

        if (m_adjustAngle) tmpZ -= m_adjustAngle;

        bool yaw_changed = (yaw != m_yawAngle);
        if (yaw_changed)
        {
            m_yawAngle = repeat(lerpAngle(m_yawAngle+180.0, yaw+180.0, m_smoothingFactor), 360.0) - 180.0;
        }
        if (pitch_changed) emit pitchAngleChanged();
        if (yaw_changed) emit yawAngleChanged();
    }
}
