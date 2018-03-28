#pragma once
#include <QIODevice>
#include <QByteArray>
#include <QMutex>
#include <QtAndroidExtras/QtAndroidExtras>

namespace mobsya {

class AndroidUsbSerialDevice : public QIODevice {
public:
    AndroidUsbSerialDevice(const QAndroidJniObject& device, QObject* parent = nullptr);
    ~AndroidUsbSerialDevice();
    qint64 bytesAvailable() const override;
    void close() override;
    bool open(OpenMode mode) override;

protected:
    qint64 readData(char* data, qint64 maxSize) override;
    qint64 writeData(const char* data, qint64 maxSize) override;

private:
    void doRead();
    void onData(jbyteArray data);

    friend class AndroidUsbSerialDeviceThread;
    friend void onAndroiUsbSerialData(JNIEnv*, jobject, jint obj, jbyteArray data);

    QByteArray m_readBuffer;
    mutable QMutex m_readMutex;
    QAndroidJniObject m_usbDevice;
    QAndroidJniObject m_connection;
};
}    // namespace mobsya
