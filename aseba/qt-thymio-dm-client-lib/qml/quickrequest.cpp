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
        /*
                QString qml = "import QtQuick 2.0\n"
                              "QtObject { \n"
                              "function create(request) {\n"
                              "    var promise = Q.promise();\n"
                              "    request.onFinished(request, function(value) {\n"
                              "        if (request.isCanceled(request)) {\n"
                              "            promise.reject();\n"
                              "        } else {\n"
                              "            promise.resolve(value);\n"
                              "        }\n"
                              "    });\n"
                              "    return promise;\n"
                              "}\n"
                              "}\n";

                QQmlComponent comp(engine);
                comp.setData(qml.toUtf8(), QUrl());
                QObject* holder = comp.create();
                if(holder == 0) {
                    return;
                }*/

        // promiseCreator = engine->newQObject(holder);
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
        /*QJSValue create = promiseCreator.property("create");
        QJSValueList args;
        args << request;

        QJSValue result = create.call(args);
        if(result.isError() || result.isUndefined()) {
            qWarning() << "request.promise: QuickPromise is not installed or setup properly";
            result = QJSValue();
        }

        return result;
        */
        return {};
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
        qRegisterMetaType<Request>("Request");
    }

    Q_COREAPP_STARTUP_FUNCTION(init)
}  // namespace qml
}  // namespace mobsya