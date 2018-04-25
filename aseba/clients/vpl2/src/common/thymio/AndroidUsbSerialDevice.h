#pragma once
#include <QIODevice>
#include <QByteArray>
#include <QMutex>
#include <QtAndroidExtras/QtAndroidExtras>

struct libusb_device_handle;
struct libusb_device;
extern "C" {
void libusb_close(libusb_device_handle*);
}
namespace mobsya {


class AndroidUsbSerialDeviceThread;
class AndroidUsbSerialDevice : public QIODevice {
public:
    AndroidUsbSerialDevice(const QAndroidJniObject& device, QObject* parent = nullptr);
    ~AndroidUsbSerialDevice() override;
    qint64 bytesAvailable() const override;
    void close() override;
    bool open(OpenMode mode) override;
    bool isSequential() const override;

protected:
    qint64 readData(char* data, qint64 maxSize) override;
    qint64 writeData(const char* data, qint64 maxSize) override;

private Q_SLOTS:
    void onData(QByteArray data);

private:
    friend class AndroidUsbSerialDeviceThread;

    QMutex m_readBufferMutex;
    QByteArray m_readBuffer;
    QAndroidJniObject m_android_usb_device;
    AndroidUsbSerialDeviceThread* m_thread;
};
}  // namespace mobsya
