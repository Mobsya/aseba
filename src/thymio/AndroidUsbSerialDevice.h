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

struct usb_handle {
    ~usb_handle() {
        if(h)
            libusb_close(h);
    }
    operator libusb_device_handle*&() {
        return h;
    }

private:
    libusb_device_handle* h = nullptr;
};

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

private:
    void doRead();
    void onData(jbyteArray data);

    friend class AndroidUsbSerialDeviceThread;
    friend void onAndroiUsbSerialData(JNIEnv*, jobject, jint obj, jbyteArray data);

    QByteArray m_readBuffer;
    mutable QMutex m_readMutex;
    QAndroidJniObject m_android_usb_device;
    QAndroidJniObject m_connection;
    QThread* m_thread;
    usb_handle m_usb_handle;
    libusb_device* m_usb_device;
    uint8_t m_out_address = 0;
    uint8_t m_in_address = 0;
    int m_read_size = 0;
    int m_write_size = 0;
};
}    // namespace mobsya
