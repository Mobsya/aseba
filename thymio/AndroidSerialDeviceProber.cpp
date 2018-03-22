#include "AndroidSerialDeviceProber.h"
#include <QtDebug>
#include <QtAndroidExtras/QtAndroidExtras>


namespace mobsya {

const char* ACTIVITY_CLASS_NAME = "org/mobsya/thymiovpl/ThymioVPLActivity";

class AndroidSerialThymioInfo : public AbstractThymioInfoPrivate {
public:
    AndroidSerialThymioInfo(QString portName, QString deviceName)
        : AbstractThymioInfoPrivate(ThymioInfo::DeviceProvider::AndroidSerial)
        , m_portName(portName)
        , m_deviceName(deviceName) {
    }

    QString name() const {
        return m_deviceName;
    }
    virtual bool equals(const ThymioInfo& other) {
        if(other.provider() != ThymioInfo::DeviceProvider::Serial)
            return false;
        auto port = static_cast<const AndroidSerialThymioInfo*>(other.data())->m_portName;
        return port == m_portName;
    }
    QString m_portName;
    QString m_deviceName;
};

AndroidSerialDeviceProber::AndroidSerialDeviceProber(QObject* parent)
    : AbstractDeviceProber(parent) {
}

AndroidSerialDeviceProber* AndroidSerialDeviceProber::instance() {
    static AndroidSerialDeviceProber the_instance;
    return &the_instance;
}

auto fromJni(QAndroidJniObject& device) {
    QAndroidJniObject deviceName = device.callObjectMethod<jstring>("getDeviceName");
    QAndroidJniObject productName = device.callObjectMethod<jstring>("getProductName");
    qDebug() << deviceName.toString() << productName.toString();
    return std::make_unique<AndroidSerialThymioInfo>(deviceName.toString().trimmed(),
                                                     productName.toString().trimmed());
}

std::vector<ThymioInfo> AndroidSerialDeviceProber::getThymios() {
    QAndroidJniEnvironment qjniEnv;

    QAndroidJniObject jDevices = QAndroidJniObject::callStaticObjectMethod(
        ACTIVITY_CLASS_NAME, "listDevices", "()[Landroid/hardware/usb/UsbDevice;");
    jobjectArray objectArray = jDevices.object<jobjectArray>();
    if(!objectArray)
        return {};

    std::vector<ThymioInfo> compatible_ports;
    const int n = qjniEnv->GetArrayLength(objectArray);
    for(int i = 0; i < n; ++i) {
        auto elem = QAndroidJniObject::fromLocalRef(qjniEnv->GetObjectArrayElement(objectArray, i));
        compatible_ports.emplace_back(fromJni(elem));
    }
    return compatible_ports;
}

AndroidSerialDeviceProber::~AndroidSerialDeviceProber() {
}

void AndroidSerialDeviceProber::onDeviceAvailabilityChanged() {
    Q_EMIT availabilityChanged();
}

void onDeviceAvailabilityChanged(JNIEnv*, jobject) {
    AndroidSerialDeviceProber::instance()->onDeviceAvailabilityChanged();
}

#define JNI_METHOD(MethodPtr, name) \
    jni::MakeNativePeerMethod<decltype(MethodPtr), (MethodPtr)>(name)

static void RegisterAndroidSerialDeviceProber(JavaVM*) {

    JNINativeMethod methods[]{{"onDeviceAvailabilityChanged", "()V",
                               reinterpret_cast<void*>(onDeviceAvailabilityChanged)}};

    QAndroidJniObject javaClass("org/mobsya/thymiovpl/ThymioVPLActivity");
    QAndroidJniEnvironment env;

    jclass objectClass = env->GetObjectClass(javaClass.object<jobject>());
    env->RegisterNatives(objectClass, methods, sizeof(methods) / sizeof(methods[0]));
    env->DeleteLocalRef(objectClass);
};
}    // namespace mobsya


extern "C" JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void*) {
    mobsya::RegisterAndroidSerialDeviceProber(vm);
    return JNI_VERSION_1_6;
}
