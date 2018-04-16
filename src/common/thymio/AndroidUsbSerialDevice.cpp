#include "AndroidUsbSerialDevice.h"
#include <cstring>
#include <libusb/libusb.h>
#include <vector>
#include <memory>

namespace mobsya {

extern const char* ACTIVITY_CLASS_NAME;

namespace {
    class LibUSB {
    public:
        LibUSB() {
            libusb_init(NULL);
        }
        ~LibUSB() {
            libusb_exit(NULL);
        }
    };
    static LibUSB lib_usb_handle;

    struct usb_handle {
        usb_handle()
            : h(nullptr) {
        }
        usb_handle(const usb_handle& other) = delete;
        usb_handle(usb_handle&& other)
            : h(nullptr) {
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

}    // namespace


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

static void write_callback(libusb_transfer* transfer) {
    //   reinterpret_cast<AndroidUsbSerialDeviceThread*>(transfer->user_data)->onWrite();
}

class AndroidUsbSerialDeviceThread : public QThread {
    Q_OBJECT
public:
    AndroidUsbSerialDeviceThread(AndroidUsbSerialDeviceData&& data, QObject* parent)
        : QThread(parent)
        , m_data(std::move(data))
        , m_readTransfer(nullptr) {
    }
    void run() {
        // unsigned char buffer[8];
        while(!isInterruptionRequested()) {
            struct timeval tv;
            tv.tv_sec = 1;
            tv.tv_usec = 0;
            libusb_handle_events_timeout_completed(nullptr, &tv, nullptr);
            // doWrite();
            doReadNext();
            // libusb_clear_halt(m_data.usb_handle, m_data.in_address);
            // libusb_interrupt_transfer(m_data.usb_handle, m_data.in_address, buffer, 0, nullptr,
            // 0);
            // doRead();
        }
        if(m_readTransfer) {
            libusb_cancel_transfer(m_readTransfer);
        }
        qDebug() << "*";
        m_data.connection.callMethod<void>("close");
    }

public Q_SLOTS:
    void write(const char* data, qint64 maxSize);

Q_SIGNALS:
    void read(QByteArray data);

public:
    void doReadNext();
    void doWriteNext();
    void onReadCompleted();

private:
    using transfer = std::unique_ptr<libusb_transfer, void (*)(libusb_transfer*)>;

    struct write_transfer {
        QByteArray data;
        libusb_transfer* transfer;
        write_transfer(const char* data, qint64 maxSize, AndroidUsbSerialDeviceThread* parent)
            : data(data, int(maxSize)) {
            transfer = libusb_alloc_transfer(0);
            libusb_fill_bulk_transfer(transfer, parent->m_data.usb_handle,
                                      parent->m_data.out_address,
                                      reinterpret_cast<unsigned char*>(this->data.data()),
                                      this->data.size(), write_callback, parent, 0);
        }
        write_transfer(const write_transfer& other) = delete;
        write_transfer(write_transfer&& other) {
            data = std::move(other.data);
            transfer = other.transfer;
            other.transfer = nullptr;
        }

        ~write_transfer() {
            libusb_free_transfer(transfer);
        }

        void send() {
            libusb_submit_transfer(transfer);
        }
    };

    qint64 doWrite();
    void doRead();
    mutable QMutex m_writeMutex;
    AndroidUsbSerialDeviceData m_data;
    libusb_transfer* m_readTransfer;
    std::vector<write_transfer> m_writeTransfers;
    QByteArray m_readBuffer;


    auto create_transfer() {
        return transfer(libusb_alloc_transfer(0), libusb_free_transfer);
    }
};


static void callback(libusb_transfer* transfer) {
    reinterpret_cast<AndroidUsbSerialDeviceThread*>(transfer->user_data)->onReadCompleted();
}


void AndroidUsbSerialDeviceThread::doReadNext() {
    if(!m_readTransfer) {
        m_readBuffer.reserve(10 * m_data.read_size);
        m_readTransfer = libusb_alloc_transfer(0);
        if(libusb_claim_interface(m_data.usb_handle, 0)) {
            qDebug() << "libusb_claim_interface FAILED";
        }
        libusb_fill_bulk_transfer(m_readTransfer, m_data.usb_handle, m_data.in_address,
                                  reinterpret_cast<unsigned char*>(m_readBuffer.data()),
                                  m_readBuffer.capacity(), callback, this, 50000);
        libusb_submit_transfer(m_readTransfer);
    }
}

void AndroidUsbSerialDeviceThread::onReadCompleted() {
    if(!m_readTransfer)
        return;
    if(m_readTransfer->actual_length > 0 && (m_readTransfer->status == LIBUSB_TRANSFER_COMPLETED ||
                                             m_readTransfer->status == LIBUSB_TRANSFER_TIMED_OUT)) {
        QByteArray data(reinterpret_cast<char*>(m_readTransfer->buffer),
                        m_readTransfer->actual_length);
        Q_EMIT read(data);
    } else {
        qDebug() << m_readTransfer->status;
    }
    libusb_free_transfer(m_readTransfer);
    m_readTransfer = nullptr;
    if(libusb_release_interface(m_data.usb_handle, 0)) {
        qDebug() << "libusb_claim_interface FAILED";
    }
}

void AndroidUsbSerialDeviceThread::doRead() {
    std::vector<unsigned char> buffer;
    const auto s = m_data.read_size * 10;
    buffer.resize(s);
    while(true) {
        int read = 0;
        // qDebug() << "*" << m_data.read_size << m_data.usb_handle << m_data.in_address;
        libusb_bulk_transfer(m_data.usb_handle, m_data.in_address, buffer.data(), s, &read, 0);
        if(read == 0)
            return;
        QByteArray arr;
        arr.reserve(read);
        for(std::size_t i = 0; i < read; i++) {
            arr.append(static_cast<char>(buffer.at(i)));
        }
        qDebug() << "*" << s << read << m_data.in_address << arr.toHex() << arr.size();
        Q_EMIT this->read(arr);
    }
}

void AndroidUsbSerialDeviceThread::write(const char* data, qint64 maxSize) {
    auto transfer = write_transfer(data, maxSize, this);
    transfer.send();
    m_writeTransfers.push_back(std::move(transfer));
    // return maxSize;
}

qint64 AndroidUsbSerialDeviceThread::doWrite() {
    /* QMutexLocker _(&m_writeMutex);
     unsigned char* b = reinterpret_cast<unsigned char*>(const_cast<char*>(m_writeBuffer.data()));
     qint64 totalWritten = 0;
     qint64 maxSize = m_writeBuffer.size();
     while(maxSize > 0) {
         int toWrite = int(qMin(maxSize, qint64(m_data.write_size)));
         int written = 0;
         if(libusb_bulk_transfer(m_data.usb_handle, m_data.out_address, b + totalWritten, toWrite,
                                 &written, 0) < 0) {
             qDebug() << "AndroidUsbSerialDevice::writeData failed";
             m_writeBuffer.remove(0, int(totalWritten + written));
             return -1;
         }
         totalWritten += qint64(written);
         maxSize -= written;
     }
     m_writeBuffer.clear();
     return totalWritten;
     */
}

AndroidUsbSerialDevice::AndroidUsbSerialDevice(const QAndroidJniObject& device, QObject* parent)
    : QIODevice(parent)
    , m_android_usb_device(device)
    , m_thread(nullptr) {
}

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
        "(Landroid/hardware/usb/UsbDevice;)Landroid/hardware/usb/UsbDeviceConnection;",
        m_android_usb_device.object());

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

    qDebug() << (libusb_kernel_driver_active(data.usb_handle, 0) ? "KERNEL DRIVER ACTIVE"
                                                                 : "KERNEL DRIVER INACTIVE");
    if(libusb_kernel_driver_active(data.usb_handle, 0)) {
        libusb_detach_kernel_driver(data.usb_handle, 0);
    }
    data.usb_device = libusb_get_device(data.usb_handle);
    if(!data.usb_device) {
        qDebug() << "libusb_get_device failed";
        return false;
    }

    if(libusb_control_transfer(data.usb_handle, 0x21, 0x22, 0x01, 0, nullptr, 0, 0) < 0) {
        qDebug() << "libusb_control_transfer failed";
    }

    /*unsigned char encoding[] = {
        // 0x00, 0xC2, 0x01, 0x00,    // baud rate, little endian
        0x00, 0xFA, 0x00, 0x00,    // 64000
        0,                         // stop bits
        0,                         // parity (none)
        0x08                       // bits
    };
    if(libusb_control_transfer(data.usb_handle, 0x21, 0x20, 0, 0, encoding, sizeof(encoding), 0) <
       0) {
        qDebug() << "libusb_control_transfer failed (encoding)";
    }*/


    libusb_config_descriptor* desc;
    libusb_get_active_config_descriptor(data.usb_device, &desc);
    for(int i = 0; i < desc->bNumInterfaces; i++) {
        const auto interface = desc->interface[i];
        for(int s = 0; s < interface.num_altsetting; s++) {
            if(interface.altsetting[s].bInterfaceClass != LIBUSB_CLASS_DATA)
                continue;
            for(int e = 0; e < interface.altsetting[s].bNumEndpoints; e++) {
                const auto endpoint = interface.altsetting[s].endpoint[e];
                if(endpoint.bmAttributes & LIBUSB_TRANSFER_TYPE_BULK) {
                    if((endpoint.bEndpointAddress & LIBUSB_ENDPOINT_IN) == LIBUSB_ENDPOINT_IN) {
                        data.in_address = endpoint.bEndpointAddress;
                        data.read_size = endpoint.wMaxPacketSize;
                    } else if((endpoint.bEndpointAddress & LIBUSB_ENDPOINT_OUT) ==
                              LIBUSB_ENDPOINT_OUT) {
                        data.out_address = endpoint.bEndpointAddress;
                        data.write_size = endpoint.wMaxPacketSize;
                    }
                }
            }
        }
    }

    m_thread = new AndroidUsbSerialDeviceThread(std ::move(data), this);
    connect(m_thread, &AndroidUsbSerialDeviceThread::read, this, &AndroidUsbSerialDevice::onData,
            Qt::QueuedConnection);
    m_thread->start();
    QIODevice::setOpenMode(mode);
    return true;
}

qint64 AndroidUsbSerialDevice::readData(char* data, qint64 maxSize) {
    QMutexLocker _(&m_readBufferMutex);
    if(maxSize == 0)
        return 0;
    auto size = qMin(int(maxSize), m_readBuffer.size());
    std::memcpy(data, m_readBuffer.data(), size_t(size));
    m_readBuffer.remove(0, size);
    qDebug() << m_readBuffer.toHex() << m_readBuffer.size();
    return size;
}

qint64 AndroidUsbSerialDevice::writeData(const char* data, qint64 maxSize) {
    if(!m_thread)
        return -1;
    m_thread->write(data, maxSize);
    return maxSize;
}

void AndroidUsbSerialDevice::onData(QByteArray data) {
    QMutexLocker _(&m_readBufferMutex);
    m_readBuffer.append(data);
    QTimer::singleShot(0, this, &AndroidUsbSerialDevice::readyRead);
}

}    // namespace mobsya

#include "AndroidUsbSerialDevice.moc"
