#include "AndroidUsbSerialDevice.h"
#include <cstring>

namespace mobsya {

extern const char* ACTIVITY_CLASS_NAME;

/*
JniByteBuffer::JniByteBuffer(jsize s) {
    QAndroidJniEnvironment env;
    data = env->NewByteArray(s);
    data = reinterpret_cast<jbyteArray>(env->NewGlobalRef(data));
    m_buffer.resize(s);
    env->SetByteArrayRegion(data, 0, m_buffer.size(), reinterpret_cast<jbyte*>(m_buffer.data()));
}

JniByteBuffer::~JniByteBuffer() {
    QAndroidJniEnvironment env;
    env->DeleteGlobalRef(data);
}

void JniByteBuffer::copy_to(QByteArray& buffer, int s) {
    QAndroidJniEnvironment env;
    auto size = qMin(env->GetArrayLength(data), s);
    buffer.reserve(buffer.size() + size);
    std::copy(std::begin(m_buffer), std::begin(m_buffer) + s, std::back_inserter(buffer));
    qDebug() << "read, copied" << buffer.size() << s;
}

JniByteBuffer::operator jbyteArray() const {
    return data;
}*/

/*
AndroidUsbSerialDeviceThread::AndroidUsbSerialDeviceThread(AndroidUsbSerialDevice& parent)
    : parent(parent) {
}
void AndroidUsbSerialDeviceThread::run() {
    while(!isInterruptionRequested()) {
        parent.doRead();
    }
}*/

AndroidUsbSerialDevice::AndroidUsbSerialDevice(const QAndroidJniObject& device, QObject* parent)
    : QIODevice(parent)
    , m_usbDevice(device) {
}

AndroidUsbSerialDevice::~AndroidUsbSerialDevice() {
}

qint64 AndroidUsbSerialDevice::bytesAvailable() const {
    QMutexLocker _(&m_readMutex);
    qDebug() << "Bytes Available" << m_readBuffer.size() << QIODevice::bytesAvailable();
    return m_readBuffer.size() + QIODevice::bytesAvailable();
}

void AndroidUsbSerialDevice::close() {
    m_connection.callMethod<void>("close", "()V");
}

bool AndroidUsbSerialDevice::open(OpenMode mode) {

    if(isOpen())
        return true;

    jint j(reinterpret_cast<jint>(this));

    // Get a connection for our device
    m_connection = QAndroidJniObject::callStaticObjectMethod(
        ACTIVITY_CLASS_NAME, "openUsbDevice",
        "(ILandroid/hardware/usb/UsbDevice;)Lorg/mobsya/thymiovpl/ThymioSerialConnection;", j,
        m_usbDevice.object());

    QIODevice::setOpenMode(mode);
    return true;
}

qint64 AndroidUsbSerialDevice::readData(char* data, qint64 maxSize) {
    QMutexLocker _(&m_readMutex);
    auto size = qMin(int(maxSize), m_readBuffer.size());
    qDebug() << maxSize << size << m_readBuffer.size();
    std::memcpy(data, m_readBuffer.data(), size_t(size));
    m_readBuffer.remove(0, size);
    qDebug() << maxSize << size << m_readBuffer.size();
    return size;
}

qint64 AndroidUsbSerialDevice::writeData(const char* data, qint64 maxSize) {
    QAndroidJniEnvironment env;
    auto array = env->NewByteArray(maxSize);
    env->SetByteArrayRegion(array, 0, maxSize, reinterpret_cast<const jbyte*>(data));
    auto s = m_connection.callMethod<jint>("write", "([B)I", array);
    env->DeleteLocalRef(array);
    return s;
}

void AndroidUsbSerialDevice::onData(jbyteArray data) {
    QAndroidJniEnvironment env;
    int size = env->GetArrayLength(data);
    if(size == 0)
        return;
    QMutexLocker _(&m_readMutex);
    int start = m_readBuffer.size();
    m_readBuffer.resize(m_readBuffer.size() + size);
    qDebug() << m_readBuffer.size() << size << start;
    env->GetByteArrayRegion(data, 0, size, reinterpret_cast<jbyte*>(m_readBuffer.data() + start));
    Q_EMIT readyRead();
}

void onAndroiUsbSerialData(JNIEnv*, jobject, jint obj, jbyteArray data) {
    reinterpret_cast<AndroidUsbSerialDevice*>(obj)->onData(data);
}

void RegisterAndroidUsbSerialDevice(JavaVM*) {
    JNINativeMethod methods[]{
        {"jni_onData", "(I[B)V", reinterpret_cast<void*>(onAndroiUsbSerialData)}};
    QAndroidJniEnvironment env;
    auto class_ThymioSerialConnection =
        env->FindClass("org/mobsya/thymiovpl/ThymioSerialConnection");
    env->RegisterNatives(class_ThymioSerialConnection, methods,
                         sizeof(methods) / sizeof(methods[0]));
    env->DeleteLocalRef(class_ThymioSerialConnection);
}

}    // namespace mobsya
