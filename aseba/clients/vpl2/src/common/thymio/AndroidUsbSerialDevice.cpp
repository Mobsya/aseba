#include "AndroidUsbSerialDevice.h"
#include <cstring>
#include <libusb/libusb.h>
#include <vector>
#include <memory>

void log_with_qt(const char* text) {
    qDebug() << text << "\n";
}

namespace mobsya {

extern const char* ACTIVITY_CLASS_NAME;


namespace {
    class LibUSB {
    public:
        LibUSB() {
            libusb_init(NULL);
            libusb_set_option(NULL, LIBUSB_OPTION_LOG_LEVEL, LIBUSB_LOG_LEVEL_DEBUG);
        }
        ~LibUSB() {
            libusb_exit(NULL);
        }
    };
    static LibUSB lib_usb_handle;

    struct usb_handle {
        usb_handle() : h(nullptr) {}
        usb_handle(const usb_handle& other) = delete;
        usb_handle(usb_handle&& other) : h(nullptr) {
            std::swap(h, other.h);
        }
        ~usb_handle() {
            if(h)
                libusb_close(h);
        }
        operator libusb_device_handle*&() {
            return h;
        }

    public:
        libusb_device_handle* h = nullptr;
    };

}  // namespace


struct AndroidUsbSerialDeviceData {
    QAndroidJniObject connection;
    libusb_device* usb_device;
    uint8_t out_address = 0;
    uint8_t in_address = 0;
    int read_size = 0;
    int write_size = 0;
    mobsya::usb_handle usb_handle;
};

static void callback(libusb_transfer* transfer);

class AndroidUsbSerialDeviceThread : public QThread {
    Q_OBJECT
public:
    AndroidUsbSerialDeviceThread(AndroidUsbSerialDeviceData&& data, QObject* parent)
        : QThread(parent), m_data(std::move(data)), m_readTransfer(nullptr) {}
    void run() {
        m_readBuffer.resize(64 * 100);
        while(!isInterruptionRequested()) {
            doReadNext();
        }
        qDebug() << "*";
        m_data.connection.callMethod<void>("close");
    }

    quint64 write(const char* data, qint64 maxSize);

Q_SIGNALS:
    void read(QByteArray data);

public:
    void doReadNext();
    void doWriteNext();
    void onReadCompleted(libusb_transfer* transfer);

private:
    void doRead();
    AndroidUsbSerialDeviceData m_data;
    libusb_transfer* m_readTransfer;
    QByteArray m_readBuffer;
};

/*
static void callback(libusb_transfer* transfer) {
    // reinterpret_cast<AndroidUsbSerialDeviceThread*>(transfer->user_data)->onReadCompleted(transfer);
}*/


void AndroidUsbSerialDeviceThread::doReadNext() {
    int read = 0;
    libusb_bulk_transfer(m_data.usb_handle, m_data.in_address, reinterpret_cast<unsigned char*>(m_readBuffer.data()),
                         m_readBuffer.size(), &read, 0);
    if(read > 0) {
        Q_EMIT this->read(QByteArray(m_readBuffer.data(), read));
    }
}
/*
void AndroidUsbSerialDeviceThread::onReadCompleted(libusb_transfer* transfer) {
    if(transfer != m_readTransfer) {
        qDebug() << "Not us";
        goto end;
    }

    if(m_readTransfer->status == LIBUSB_TRANSFER_OVERFLOW) {
        libusb_reset_device(m_data.usb_handle);
        goto end;
    }

    if(m_readTransfer->actual_length > 0 &&
       (m_readTransfer->status == LIBUSB_TRANSFER_COMPLETED || m_readTransfer->status == LIBUSB_TRANSFER_TIMED_OUT)) {
        QByteArray data(reinterpret_cast<char*>(m_readTransfer->buffer), m_readTransfer->actual_length);
        Q_EMIT read(data);
    } else {
        qDebug() << m_readTransfer->status;
    }

end:
    libusb_free_transfer(m_readTransfer);
    m_readTransfer = nullptr;
}*/

quint64 AndroidUsbSerialDeviceThread::write(const char* data, qint64 maxSize) {
    int s = 0;
    auto r = libusb_bulk_transfer(m_data.usb_handle, m_data.out_address, (unsigned char*)(data), maxSize, &s, 0);
    return r == LIBUSB_SUCCESS ? s : -1;
}

AndroidUsbSerialDevice::AndroidUsbSerialDevice(const QAndroidJniObject& device, QObject* parent)
    : QIODevice(parent), m_android_usb_device(device), m_thread(nullptr) {}

AndroidUsbSerialDevice::~AndroidUsbSerialDevice() {
    close();
}

qint64 AndroidUsbSerialDevice::bytesAvailable() const {
    return m_readBuffer.size() + QIODevice::bytesAvailable();
}

void AndroidUsbSerialDevice::close() {
    if(m_thread) {
        m_thread->requestInterruption();
        m_thread->wait();
        m_thread->deleteLater();
        m_thread = nullptr;
    }
}


bool AndroidUsbSerialDevice::isSequential() const {
    return true;
}

bool AndroidUsbSerialDevice::open(OpenMode mode) {
    if(isOpen())
        return true;

    QAndroidJniEnvironment env;

    AndroidUsbSerialDeviceData data;

    libusb_set_debug(nullptr, LIBUSB_LOG_LEVEL_DEBUG);

    data.connection = QAndroidJniObject::callStaticObjectMethod(
        ACTIVITY_CLASS_NAME, "openUsbDevice",
        "(Landroid/hardware/usb/UsbDevice;)Landroid/hardware/usb/UsbDeviceConnection;", m_android_usb_device.object());

    int fd = -1;

    if(data.connection.isValid()) {
        fd = data.connection.callMethod<jint>("getFileDescriptor");
        qDebug() << "Got file descriptor from android : " << fd;
    }

    if(fd == -1 || env->ExceptionCheck()) {
        qDebug() << "Unable to open the device";
        env->ExceptionClear();
        return false;
    }

    if(libusb_wrap_fd(nullptr, fd, &data.usb_handle.h) != 0) {
        qDebug() << "libusb_wrap_fd failed";
        return false;
    }

    libusb_reset_device(data.usb_handle);

    qDebug() << (libusb_kernel_driver_active(data.usb_handle, 0) ? "KERNEL DRIVER ACTIVE" : "KERNEL DRIVER INACTIVE");
    if(libusb_kernel_driver_active(data.usb_handle, 0)) {
        libusb_detach_kernel_driver(data.usb_handle, 0);
    }
    data.usb_device = libusb_get_device(data.usb_handle);
    if(!data.usb_device) {
        qDebug() << "libusb_get_device failed";
        return false;
    }

    libusb_control_transfer(data.usb_handle, 0x21, 0x22, 0x00, 0, nullptr, 0, 0);
    if(libusb_control_transfer(data.usb_handle, 0x21, 0x22, 0x03, 0, nullptr, 0, 0) < 0) {
        qDebug() << "libusb_control_transfer failed";
    }

    unsigned char encoding[] = {
        0x00, 0xC2, 0x01, 0x00,  // baud rate, little endian
        0,                       // stop bits
        0,                       // parity (none)
        0x08                     // bits
    };
    if(libusb_control_transfer(data.usb_handle, 0x21, 0x20, 0, 0, encoding, sizeof(encoding), 0) < 0) {
        qDebug() << "libusb_control_transfer failed (encoding)";
    }


    libusb_config_descriptor* desc;
    libusb_get_active_config_descriptor(data.usb_device, &desc);
    for(int i = 0; i < desc->bNumInterfaces; i++) {
        const auto interface = desc->interface[i];
        libusb_claim_interface(data.usb_handle, i);
        for(int s = 0; s < interface.num_altsetting; s++) {
            if(interface.altsetting[s].bInterfaceClass != LIBUSB_CLASS_DATA)
                continue;
            for(int e = 0; e < interface.altsetting[s].bNumEndpoints; e++) {
                const auto endpoint = interface.altsetting[s].endpoint[e];
                if(endpoint.bmAttributes & LIBUSB_TRANSFER_TYPE_BULK) {
                    if((endpoint.bEndpointAddress & LIBUSB_ENDPOINT_IN) == LIBUSB_ENDPOINT_IN) {
                        data.in_address = endpoint.bEndpointAddress;
                        data.read_size = endpoint.wMaxPacketSize;
                    } else if((endpoint.bEndpointAddress & LIBUSB_ENDPOINT_OUT) == LIBUSB_ENDPOINT_OUT) {
                        data.out_address = endpoint.bEndpointAddress;
                        data.write_size = endpoint.wMaxPacketSize;
                    }
                }
            }
        }
    }

    m_thread = new AndroidUsbSerialDeviceThread(std ::move(data), this);
    connect(m_thread, &AndroidUsbSerialDeviceThread::read, this, &AndroidUsbSerialDevice::onData, Qt::QueuedConnection);
    m_thread->start();
    QIODevice::setOpenMode(mode);
    return true;
}

qint64 AndroidUsbSerialDevice::readData(char* data, qint64 maxSize) {
    QMutexLocker _(&m_readBufferMutex);
    if(maxSize == 0)
        return 0;
    auto size = qMin(int(maxSize), m_readBuffer.size());
    std::copy(m_readBuffer.data(), m_readBuffer.data() + size_t(size), data);
    m_readBuffer.remove(0, size);
    qDebug() << m_readBuffer.toHex() << m_readBuffer.size();
    return size;
}

qint64 AndroidUsbSerialDevice::writeData(const char* data, qint64 maxSize) {
    return m_thread->write(data, maxSize);
}

void AndroidUsbSerialDevice::onData(QByteArray data) {
    QMutexLocker _(&m_readBufferMutex);
    m_readBuffer.append(data);
    QTimer::singleShot(0, this, &AndroidUsbSerialDevice::readyRead);
}

}  // namespace mobsya

#include "AndroidUsbSerialDevice.moc"
