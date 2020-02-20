#pragma once
#include "request.h"
#include <QQmlEngine>
#include <QPointer>
#include <QtDebug>
#include <QTimer>

namespace mobsya {
namespace qml {

    typedef std::function<QVariant(void*)> Converter;

    template <typename T>
    inline QJSValueList value(const QPointer<QQmlEngine>& engine, const mobsya::BasicRequest<T>& request) {
        QJSValue value;
        if(request.isFinished() && request.success())
            value = engine->toScriptValue<T>(request.getResult());
        if(request.isFinished() && !request.success())
            value = engine->toScriptValue<Error>(request.getError());
        return QJSValueList() << value;
    }

    template <typename F>
    inline void nextTick(F func) {
        QTimer::singleShot(0, func);
    }


    inline void printException(QJSValue value) {
        QString message = QString("%1:%2: %3: %4")
                              .arg(value.property("fileName").toString())
                              .arg(value.property("lineNumber").toString())
                              .arg(value.property("name").toString())
                              .arg(value.property("message").toString());
        qWarning() << message;
    }

    class VariantWrapperBase {
    public:
        VariantWrapperBase() {}

        virtual inline ~VariantWrapperBase() {}

        virtual QVariant result(const QVariant& v) = 0;
        virtual QVariant error(const QVariant& v) = 0;
        virtual bool isFinished(const QVariant& v) = 0;
        virtual bool isCanceled(const QVariant& v) = 0;
        virtual bool success(const QVariant& v) = 0;

        virtual void onFinished(QPointer<QQmlEngine> engine, const QVariant& v, const QJSValue& func,
                                QObject* owner) = 0;

        virtual void onCanceled(QPointer<QQmlEngine> engine, const QVariant& v, const QJSValue& func,
                                QObject* owner) = 0;
        Converter converter;
    };

    template <typename T>
    class VariantWrapper : public VariantWrapperBase {
    public:
        bool isFinished(const QVariant& v) override {
            BasicRequest<T> req = v.value<BasicRequest<T>>();
            return req.isFinished();
        }
        bool isCanceled(const QVariant& v) override {
            BasicRequest<T> req = v.value<BasicRequest<T>>();
            return req.isCanceled();
        }

        void onFinished(QPointer<QQmlEngine> engine, const QVariant& v, const QJSValue& func, QObject* owner) override {
            QPointer<QObject> context = owner;
            if(!func.isCallable()) {
                qWarning() << "Request: isFinished Callback is not callable";
                return;
            }
            BasicRequest<T> request = v.value<BasicRequest<T>>();
            auto listener = [=]() {
                if(!engine.isNull()) {
                    QJSValue callback = func;
                    QJSValue ret = callback.call(QJSValueList() << request.success() << value(engine, request));
                    if(ret.isError()) {
                        printException(ret);
                    }
                }
            };
            if(request.isFinished()) {
                nextTick([=]() {
                    if(owner && !context.isNull()) {
                        return;
                    }
                    listener();
                });
            } else {
                BasicRequestWatcher<T>* watcher = new BasicRequestWatcher<T>(request);
                QObject::connect(watcher, &detail::RequestWatcherBase::finished, [=]() {
                    listener();
                    delete watcher;
                });
                watcher->setParent(owner);
            }
        }

        void onCanceled(QPointer<QQmlEngine> engine, const QVariant& v, const QJSValue& func, QObject* owner) override {
            QPointer<QObject> context = owner;
            if(!func.isCallable()) {
                qWarning() << "Request: isCanceled Callback is not callable";
                return;
            }
            BasicRequest<T> request = v.value<BasicRequest<T>>();
            auto listener = [=]() {
                if(!engine.isNull()) {
                    QJSValue callback = func;
                    QJSValue ret = callback.call(value(engine, request));
                    if(ret.isError()) {
                        printException(ret);
                    }
                }
            };
            if(request.isCanceled()) {
                nextTick([=]() {
                    if(owner && !context.isNull()) {
                        return;
                    }
                    listener();
                });
            } else {
                BasicRequestWatcher<T>* watcher = new BasicRequestWatcher<T>(request);
                QObject::connect(watcher, &detail::RequestWatcherBase::canceled, [=]() {
                    listener();
                    delete watcher;
                });
                watcher->setParent(owner);
            }
        }

        QVariant result(const QVariant& request) override {
            BasicRequest<T> r = request.value<BasicRequest<T>>();
            if(!r.isFinished() || !r.success()) {
                qWarning() << "BasicRequest.result(): The result is not ready!";
                return QVariant();
            }
            QVariant ret;
            if(converter != nullptr) {
                T t = r.getResult();
                ret = converter(&t);
            } else {
                ret = QVariant::fromValue<T>(r.getResult());
            }
            return ret;
        }

        QVariant error(const QVariant& v) override {
            BasicRequest<T> r = v.value<BasicRequest<T>>();
            if(!r.isFinished()) {
                qWarning() << "BasicRequest.error(): The result is not ready!";
                return QVariant();
            }
            return QVariant::fromValue<Error>(r.getError());
        }

        virtual bool success(const QVariant& v) override {
            BasicRequest<T> r = v.value<BasicRequest<T>>();
            if(!r.isFinished()) {
                qWarning() << "BasicRequest.success(): The result is not ready!";
                return false;
            }
            return r.success();
        }
    };  // namespace qml


    class QmlRequest : public QObject {

        Q_OBJECT
    public:
        explicit QmlRequest(QObject* parent = 0);

        template <typename T>
        static void registerType() {
            registerType(qRegisterMetaType<BasicRequest<T>>(), new VariantWrapper<T>());
        }

        template <typename T>
        static void registerType(std::function<QVariant(T)> converter) {
            VariantWrapper<T>* wrapper = new VariantWrapper<T>();
            wrapper->converter = [=](void* data) { return converter(*(T*)data); };
            registerType(qRegisterMetaType<BasicRequest<T>>(), wrapper);
        }

        QJSEngine* engine() const;

        void setEngine(QQmlEngine* engine);

    public Q_SLOTS:
        bool isFinished(const QVariant& request);

        bool isCanceled(const QVariant& request);

        QVariant result(const QVariant& request);

        QVariant error(const QVariant& request);

        bool success(const QVariant& request);

        QJSValue promise(QJSValue request);

        void onFinished(const QVariant& request, QJSValue func, QJSValue owner = QJSValue());

        void onCanceled(const QVariant& request, QJSValue func, QJSValue owner = QJSValue());

    private:
        static void registerType(int typeId, VariantWrapperBase* wrapper);

        QPointer<QQmlEngine> m_engine;
        QJSValue promiseCreator;
    };


    template <typename T>
    static void registerType() {
        QmlRequest::registerType<T>();
    }

    template <typename T>
    static void registerType(std::function<QVariant(T)> converter) {
        QmlRequest::registerType<T>(converter);
    }


}  // namespace qml


}  // namespace mobsya

Q_DECLARE_METATYPE(mobsya::Error)
Q_DECLARE_METATYPE(mobsya::Request)
Q_DECLARE_METATYPE(mobsya::CompilationRequest)
Q_DECLARE_METATYPE(mobsya::SimpleRequestResult)
Q_DECLARE_METATYPE(mobsya::CompilationResult)
Q_DECLARE_METATYPE(mobsya::CompilationError)
Q_DECLARE_METATYPE(mobsya::BreakpointsRequest)
Q_DECLARE_METATYPE(mobsya::SetBreakpointRequestResult)
Q_DECLARE_METATYPE(mobsya::AsebaVMDescriptionRequest)
Q_DECLARE_METATYPE(mobsya::AsebaVMDescriptionRequestResult)
Q_DECLARE_METATYPE(mobsya::Thymio2WirelessDonglePairingRequest)
Q_DECLARE_METATYPE(mobsya::Thymio2WirelessDonglePairingResult)
