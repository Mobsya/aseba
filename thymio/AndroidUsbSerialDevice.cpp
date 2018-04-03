#include "AndroidUsbSerialDevice.h"
#include <cstring>
#include <libusb.h>

namespace mobsya {

extern const char* ACTIVITY_CLASS_NAME;

namespace {
    struct libusb {
        libusb() {
            libusb_init(&ctx);
        }
        ~libusb() {
            libusb_exit(ctx);
        }

        static libusb& instance() {
            static libusb inst;
            return inst;
        }

        libusb_context* context() const {
            return ctx;
        }

    private:
        libusb_context* ctx;
    };

}    // namespace

class AndroidUsbSerialDeviceThread : public QThread {
public:
    AndroidUsbSerialDeviceThread(AndroidUsbSerialDevice& parent)
        : parent(parent) {
    }
    void run() {
        while(!isInterruptionRequested()) {
            parent.doRead();
        }
    }

private:
    AndroidUsbSerialDevice& parent;
};

AndroidUsbSerialDevice::AndroidUsbSerialDevice(const QAndroidJniObject& device, QObject* parent)
    : QIODevice(parent)
    , m_android_usb_device(device)
    , m_thread(new AndroidUsbSerialDeviceThread(*this)) {
}

AndroidUsbSerialDevice::~AndroidUsbSerialDevice() {
    close();
}

qint64 AndroidUsbSerialDevice::bytesAvailable() const {
    QMutexLocker _(&m_readMutex);
    return m_readBuffer.size() + QIODevice::bytesAvailable();
}

void AndroidUsbSerialDevice::close() {
    m_usb_handle = {};
    m_connection.callMethod<void>("close");
    m_thread->requestInterruption();
    m_thread->wait();
}


bool AndroidUsbSerialDevice::isSequential() const {
    return true;
}

bool AndroidUsbSerialDevice::open(OpenMode mode) {

    if(isOpen())
        return true;

    QAndroidJniEnvironment env;

    m_connection = QAndroidJniObject::callStaticObjectMethod(
        ACTIVITY_CLASS_NAME, "openUsbDevice",
        "(Landroid/hardware/usb/UsbDevice;)Landroid/hardware/usb/UsbDeviceConnection;",
        m_android_usb_device.object());

    int fd = -1;

    if(m_connection.isValid()) {
        fd = m_connection.callMethod<jint>("getFileDescriptor");
        qDebug() << "Got file descriptor from android : " << fd;
    }

    if(fd == -1 || env->ExceptionCheck()) {
        m_connection = QAndroidJniObject();
        qDebug() << "Unable to open the device";
        env->ExceptionClear();
        return false;
    }

    if(libusb_wrap_fd(libusb::instance().context(), fd,
                      &(static_cast<libusb_device_handle*&>(m_usb_handle))) != 0) {
        qDebug() << "libusb_wrap_fd failed";
        m_usb_handle = {};
        m_connection = QAndroidJniObject();
        return false;
    }
    m_usb_device = libusb_get_device(m_usb_handle);
    if(!m_usb_device) {
        qDebug() << "libusb_get_device failed";
        m_usb_handle = {};
        m_connection = QAndroidJniObject();
        return false;
    }
    if(libusb_interrupt_transfer(m_usb_handle, 0x81, nullptr, 0, nullptr, 100) < 0) {
        qDebug() << "libusb_interrupt_transfer failed";
    }

    if(libusb_control_transfer(m_usb_handle, 0x21, 0x22, 0x03, 0, nullptr, 0, 0) < 0) {
        qDebug() << "libusb_control_transfer failed";
    }

    unsigned char encoding[] = {
        0x00, 0xC2, 0x01, 0x00,    // baud rate, little endian
        0,                         // stop bits
        0,                         // parity (none)
        0x08                       // bits
    };
    if(libusb_control_transfer(m_usb_handle, 0x21, 0x20, 0, 0, encoding, sizeof(encoding), 0) < 0) {
        qDebug() << "libusb_control_transfer failed (encoding)";
    }

    libusb_config_descriptor* desc;
    libusb_get_active_config_descriptor(m_usb_device, &desc);
    for(int i = 0; i < desc->bNumInterfaces; i++) {
        const auto interface = desc->interface[i];
        for(int s = 0; s < interface.num_altsetting; s++) {
            if(interface.altsetting[s].bInterfaceClass != LIBUSB_CLASS_DATA)
                continue;
            for(int e = 0; e < interface.altsetting[s].bNumEndpoints; e++) {
                const auto endpoint = interface.altsetting[s].endpoint[e];
                if(endpoint.bmAttributes & LIBUSB_TRANSFER_TYPE_BULK) {
                    if((endpoint.bEndpointAddress & LIBUSB_ENDPOINT_IN) == LIBUSB_ENDPOINT_IN) {
                        m_in_address = endpoint.bEndpointAddress;
                        m_read_size = endpoint.wMaxPacketSize;
                    } else if((endpoint.bEndpointAddress & LIBUSB_ENDPOINT_OUT) ==
                              LIBUSB_ENDPOINT_OUT) {
                        m_out_address = endpoint.bEndpointAddress;
                        m_write_size = endpoint.wMaxPacketSize;
                    }
                }
            }
        }
    }

    QIODevice::setOpenMode(mode);
    m_thread->start();
    return true;
}

qint64 AndroidUsbSerialDevice::readData(char* data, qint64 maxSize) {
    if(maxSize == 0)
        return 0;
    QMutexLocker _(&m_readMutex);
    auto size = qMin(int(maxSize), m_readBuffer.size());
    std::memcpy(data, m_readBuffer.data(), size_t(size));
    m_readBuffer.remove(0, size);
    return size;
}

qint64 AndroidUsbSerialDevice::writeData(const char* data, qint64 maxSize) {
    unsigned char* b = reinterpret_cast<unsigned char*>(const_cast<char*>(data));
    qint64 totalWritten = 0;
    while(maxSize > 0) {
        int toWrite = int(qMin(maxSize, qint64(m_write_size)));
        int written = 0;
        if(libusb_bulk_transfer(m_usb_handle, m_out_address, b, toWrite, &written, 0) < 0) {
            qDebug() << "AndroidUsbSerialDevice::writeData failed";
            return -1;
        }
        totalWritten += qint64(written);
        maxSize -= written;
    }
    return totalWritten;
}

void AndroidUsbSerialDevice::doRead() {
    QByteArray buffer;
    buffer.resize(m_read_size);
    int read = 0;
    libusb_bulk_transfer(m_usb_handle, m_in_address,
                         reinterpret_cast<unsigned char*>(buffer.data()), m_read_size, &read, 200);
    if(read > 0) {
        QMutexLocker lock(&m_readMutex);
        m_readBuffer.append(buffer.data(), read);
        Q_EMIT readyRead();
    }
}

}    // namespace mobsya
