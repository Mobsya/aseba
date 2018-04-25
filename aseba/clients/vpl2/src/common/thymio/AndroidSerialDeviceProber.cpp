#include "AndroidSerialDeviceProber.h"
#include "AndroidUsbSerialDevice.h"
#include <QtDebug>
#include <QtAndroidExtras/QtAndroidExtras>


namespace mobsya {

const char* ACTIVITY_CLASS_NAME = "org/mobsya/thymiovpl/ThymioVPLActivity";

class AndroidSerialThymioProviderInfo : public AbstractThymioProviderInfoPrivate {
public:
    AndroidSerialThymioProviderInfo(QString portName, QString deviceName, const QAndroidJniObject& device)
        : AbstractThymioProviderInfoPrivate(ThymioProviderInfo::ProviderType::AndroidSerial)
        , m_portName(portName)
        , m_deviceName(deviceName)
        , m_device(device) {}

    QString name() const {
        return m_deviceName;
    }
    bool equals(const ThymioProviderInfo& other) override {
        if(other.type() != ThymioProviderInfo::ProviderType::AndroidSerial)
            return false;
        auto port = static_cast<const AndroidSerialThymioProviderInfo*>(other.data())->m_portName;
        return port == m_portName;
    }
    bool lt(const ThymioProviderInfo& other) override {
        if(other.type() != ThymioProviderInfo::ProviderType::AndroidSerial)
            return false;
        auto port = static_cast<const AndroidSerialThymioProviderInfo*>(other.data())->m_portName;
        return port < m_portName;
    }

    QString m_portName;
    QString m_deviceName;
    QAndroidJniObject m_device;
};

AndroidSerialDeviceProber::AndroidSerialDeviceProber(QObject* parent) : AbstractDeviceProber(parent) {}

AndroidSerialDeviceProber* AndroidSerialDeviceProber::instance() {
    static AndroidSerialDeviceProber the_instance;
    return &the_instance;
}

auto fromJni(QAndroidJniObject device) {
    QAndroidJniObject deviceName = device.callObjectMethod<jstring>("getDeviceName");
    QAndroidJniObject productName = device.callObjectMethod<jstring>("getProductName");
    qDebug() << deviceName.toString() << productName.toString();
    return std::make_unique<AndroidSerialThymioProviderInfo>(deviceName.toString().trimmed(),
                                                             productName.toString().trimmed(), device);
}

std::vector<ThymioProviderInfo> AndroidSerialDeviceProber::getDevices() {
    QAndroidJniEnvironment qjniEnv;

    QAndroidJniObject jDevices = QAndroidJniObject::callStaticObjectMethod(ACTIVITY_CLASS_NAME, "listDevices",
                                                                           "()[Landroid/hardware/usb/UsbDevice;");
    jobjectArray objectArray = jDevices.object<jobjectArray>();
    if(!objectArray)
        return {};

    std::vector<ThymioProviderInfo> compatible_ports;
    const int n = qjniEnv->GetArrayLength(objectArray);
    for(int i = 0; i < n; ++i) {
        auto elem = QAndroidJniObject::fromLocalRef(qjniEnv->GetObjectArrayElement(objectArray, i));
        compatible_ports.emplace_back(fromJni(elem));
    }
    return compatible_ports;
}

AndroidSerialDeviceProber::~AndroidSerialDeviceProber() {}

void AndroidSerialDeviceProber::onDeviceAvailabilityChanged() {
    Q_EMIT availabilityChanged();
}

void onDeviceAvailabilityChanged(JNIEnv*, jobject) {
    AndroidSerialDeviceProber::instance()->onDeviceAvailabilityChanged();
}

std::shared_ptr<DeviceQtConnection> AndroidSerialDeviceProber::openConnection(const ThymioProviderInfo& info) {
    if(info.type() != ThymioProviderInfo::ProviderType::AndroidSerial)
        return {};

    auto connection = std::make_unique<AndroidUsbSerialDevice>(
        (static_cast<const AndroidSerialThymioProviderInfo*>(info.data()))->m_device);
    if(!connection->open(QIODevice::ReadWrite)) {
        return {};
    }
    auto wrapper = std::make_shared<DeviceQtConnection>(info, connection.get());
    connection.release();
    return wrapper;
}

static void RegisterAndroidSerialDeviceProber(JavaVM*) {

    JNINativeMethod methods[]{
        {"onDeviceAvailabilityChanged", "()V", reinterpret_cast<void*>(onDeviceAvailabilityChanged)}};

    QAndroidJniEnvironment env;
    auto class_ThymioVPLActivity = env->FindClass("org/mobsya/thymiovpl/ThymioVPLActivity");
    env->RegisterNatives(class_ThymioVPLActivity, methods, sizeof(methods) / sizeof(methods[0]));
}

// extern void RegisterAndroidUsbSerialDevice(JavaVM*);

}  // namespace mobsya


extern "C" JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void*) {
    mobsya::RegisterAndroidSerialDeviceProber(vm);
    // mobsya::RegisterAndroidUsbSerialDevice(vm);
    return JNI_VERSION_1_6;
}
