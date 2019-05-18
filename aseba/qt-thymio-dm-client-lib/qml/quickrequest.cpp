#include "quickrequest.h"


#include <QGuiApplication>
#include <QtQml>
#include <QQmlComponent>

#include "quickrequest.h"

namespace mobsya {
namespace qml {

    static QMap<int, VariantWrapperBase*> m_wrappers;

    static int typeId(const QVariant& v) {
        return v.userType();
    }

    QmlRequest::QmlRequest(QObject* parent) : QObject(parent) {}

    void QmlRequest::registerType(int typeId, VariantWrapperBase* wrapper) {
        if(m_wrappers.contains(typeId)) {
            qWarning()
                << QString("QmlRequest::registerType:It is already registered:%1").arg(QMetaType::typeName(typeId));
            return;
        }

        m_wrappers[typeId] = wrapper;
    }

    VariantWrapperBase* getWrapper(const QVariant& request) {
        QVariant res;
        if(!m_wrappers.contains(typeId(request))) {
            qWarning()
                << QString("request: Can not handle input data type: %1").arg(QMetaType::typeName(request.type()));
            return nullptr;
        }
        return m_wrappers[typeId(request)];
    }

    QJSEngine* QmlRequest::engine() const {
        return m_engine;
    }

    void QmlRequest::setEngine(QQmlEngine* engine) {
        m_engine = engine;
        if(m_engine.isNull()) {
            return;
        }

        QString qml = R"(
import QtQuick 2.0
import org.mobsya 1.0
QtObject {
    function create(request) {
        var promise = new Promise(function(resolve, reject) {
            Request.onFinished(request, function(res, value) {
                if(res)
                    resolve(value)
                else reject(value)
            });
        });
        return promise;
    }
})";

        QQmlComponent comp(engine);
        comp.setData(qml.toUtf8(), QUrl());
        QObject* holder = comp.create();
        if(holder == 0) {
            return;
        }

        promiseCreator = engine->newQObject(holder);
    }

    bool QmlRequest::isFinished(const QVariant& request) {
        auto wrapper = getWrapper(request);
        if(!wrapper)
            return false;
        return wrapper->isFinished(request);
    }

    bool QmlRequest::isCanceled(const QVariant& request) {
        auto wrapper = getWrapper(request);
        if(!wrapper)
            return false;
        return wrapper->isCanceled(request);
    }

    void QmlRequest::onFinished(const QVariant& request, QJSValue func, QJSValue owner) {
        auto wrapper = getWrapper(request);
        if(!wrapper)
            return;
        wrapper->onFinished(m_engine, request, func, owner.toQObject());
    }

    void QmlRequest::onCanceled(const QVariant& request, QJSValue func, QJSValue owner) {
        auto wrapper = getWrapper(request);
        if(!wrapper)
            return;
        wrapper->onCanceled(m_engine, request, func, owner.toQObject());
    }

    QVariant QmlRequest::result(const QVariant& request) {
        auto wrapper = getWrapper(request);
        if(!wrapper)
            return {};
        return wrapper->result(request);
    }

    QVariant QmlRequest::error(const QVariant& request) {
        auto wrapper = getWrapper(request);
        if(!wrapper)
            return {};
        return wrapper->result(request);
    }

    bool QmlRequest::success(const QVariant& request) {
        auto wrapper = getWrapper(request);
        if(!wrapper)
            return {};
        return wrapper->success(request);
    }


    QJSValue QmlRequest::promise(QJSValue request) {
        QJSValue create = promiseCreator.property("create");
        QJSValueList args;
        args << request;

        QJSValue result = create.call(args);
        if(result.isError() || result.isUndefined()) {
            qWarning() << "Can't create a promise - Qt too old ?";
            result = QJSValue();
        }
        return result;
    }

    static QObject* provider(QQmlEngine* engine, QJSEngine* scriptEngine) {
        Q_UNUSED(scriptEngine);

        QmlRequest* object = new QmlRequest();
        object->setEngine(engine);

        return object;
    }

    static void init() {
        bool called = false;
        if(called) {
            return;
        }
        called = true;
        QObject::connect(new QObject(qApp), &QObject::destroyed, [=]() { qDeleteAll(m_wrappers); });
        qmlRegisterSingletonType<QmlRequest>("org.mobsya", 1, 0, "Request", provider);
        QmlRequest::registerType<SimpleRequestResult>();
        QmlRequest::registerType<CompilationResult>();
        QmlRequest::registerType<Thymio2WirelessDongleInfoResult>();
        qRegisterMetaType<Request>("Request");
        qRegisterMetaType<CompilationError>();
        qRegisterMetaType<SetBreakpointRequestResult>();
        qRegisterMetaType<AsebaVMDescriptionRequestResult>();
    }

    Q_COREAPP_STARTUP_FUNCTION(init)
}  // namespace qml
}  // namespace mobsya