#pragma once
#include <QObject>
#include <QEventLoop>
#include <QCoreApplication>
#include <QRandomGenerator>
#include <memory>
#include <optional>
#include <mutex>
#include <aseba/flatbuffers/fb_message_ptr.h>
#include <QtDebug>
#include <iostream>

namespace mobsya {

class Error {
    Q_GADGET

public:
    using E = mobsya::fb::ErrorType;
    Q_ENUM(E);

public:
    Error(E e = E::no_error) : m_error(e) {}

    Q_INVOKABLE E error() const {
        return m_error;
    }

    Q_INVOKABLE operator E() const {
        return m_error;
    }

    Q_INVOKABLE QString toString() {
        switch(m_error) {
            case E::node_busy: return "Node busy";
            case E::unknown_node: return "Unknown node";
            case E::unsupported_variable_type: return "Unsupported variable type";
            case E::unknown_error: return "Unknown error";
            case E::thymio2_pairing_write_dongle_failed: return "Unable to save the wireless settings in the dongle";
            case E::thymio2_pairing_write_robot_failed: return "Unable to save the wireless settings in the robot";
            case E::no_error: break;
        }
        return {};
    }

private:
    E m_error;
};

namespace detail {

    class RequestDataBase : public QObject {
        Q_OBJECT
    public:
        using request_id = quint32;
        using shared_ptr = std::shared_ptr<RequestDataBase>;

        RequestDataBase(quint32 id, quint32 type) : m_id(id), m_type(type) {
            m_canceled.store(false);
        }
        void cancel() {
            m_canceled.store(true);
            Q_EMIT finished();
            Q_EMIT canceled();
        }
        bool isCanceled() const {
            return m_canceled;
        }

        quint32 id() const {
            return m_id;
        }

        quint32 type() const {
            return m_type;
        }

        template <typename... Args>
        void setError(Args&&... args) {
            {
                std::unique_lock<std::mutex> _(m_mutex);
                m_error = Error(std::forward<Args>(args)...);
            }
            Q_EMIT finished();
        }

        void setError(Error r) {
            {
                std::unique_lock<std::mutex> _(m_mutex);
                m_error = std::move(r);
            }
            Q_EMIT finished();
        }
        Error getError() {
            std::unique_lock<std::mutex> _(m_mutex);
            if(m_error)
                return *m_error;
            return {};
        }

        template <typename T>
        const T* as() const {
            if(T::type != m_type)
                return {};
            return static_cast<const T*>(this);
        }

        template <typename T>
        T* as() {
            if(T::type != m_type)
                return {};
            return static_cast<T*>(this);
        }

    Q_SIGNALS:
        void finished();
        void canceled();

    protected:
        mutable std::mutex m_mutex;
        const quint32 m_id;
        const quint32 m_type;
        std::atomic_bool m_canceled;
        std::optional<Error> m_error;
        friend class ThymioDeviceManagerClientEndpoint;
    };

    template <typename Result>
    class RequestData : public RequestDataBase {
    public:
        static constexpr auto type = Result::type;
        using RequestDataBase::RequestDataBase;

        void setResult(Result r) {
            {
                std::unique_lock<std::mutex> _(m_mutex);
                m_data = std::move(r);
            }
            Q_EMIT finished();
        }

        template <typename... Args>
        void setResult(Args&&... args) {
            {
                std::unique_lock<std::mutex> _(m_mutex);
                m_data = Result(std::forward<Args>(args)...);
            }
            Q_EMIT finished();
        }

        Result getResult() {
            if(!isFinished())
                wait();
            std::unique_lock<std::mutex> _(m_mutex);
            return *m_data;
        }


        bool isFinished() const {
            std::unique_lock<std::mutex> _(m_mutex);
            return m_canceled || m_error || m_data;
        }

        bool success() const {
            std::unique_lock<std::mutex> _(m_mutex);
            return !m_canceled && m_data;
        }

        void wait() {
            while(!isFinished()) {
                QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
            }
        }

    private:
        std::optional<Result> m_data;
    };
}  // namespace detail

template <typename Result>
class BasicRequest {
    friend class ThymioDeviceManagerClientEndpoint;

public:
    BasicRequest() {}
    BasicRequest(const BasicRequest& other) = default;
    virtual ~BasicRequest() {}
    using request_id = quint32;
    using internal_ptr_type = detail::RequestData<Result>;
    static request_id generate_request_id() {
        quint32 value = QRandomGenerator::global()->generate();
        return value;
    }

private:
    std::shared_ptr<detail::RequestData<Result>> get_ptr() const {
        return ptr;
    }
    static BasicRequest make_request() {
        BasicRequest<Result> r;
        r.ptr = std::make_shared<detail::RequestData<Result>>(generate_request_id(), Result::type);
        return r;
    }

public:
    request_id id() const {
        return ptr ? ptr->id() : 0;
    }

    Result getResult() const {
        return ptr->getResult();
    }

    Error getError() const {
        return ptr->getError();
    }

    bool success() const {
        return ptr && ptr->success();
    }

    void wait() {
        if(!ptr)
            return;
        return ptr->wait();
    }

    bool isCanceled() const {
        return !ptr || ptr->isCanceled();
    }

    bool isFinished() const {
        return !ptr || ptr->isFinished();
    }

private:
    template <typename R>
    friend class BasicRequestWatcher;
    std::shared_ptr<detail::RequestData<Result>> ptr;
};

namespace detail {
    class RequestWatcherBase : public QObject {
        Q_OBJECT
        using QObject::QObject;

    Q_SIGNALS:
        void finished();
        void canceled();
    };

}  // namespace detail

template <typename Result>
class BasicRequestWatcher : public detail::RequestWatcherBase, public BasicRequest<Result> {
public:
    BasicRequestWatcher(QObject* parent = nullptr) : detail::RequestWatcherBase(parent) {}
    BasicRequestWatcher(const BasicRequest<Result>& req, QObject* parent = nullptr)
        : detail::RequestWatcherBase(parent) {

        setRequest(req);
    }

    void setRequest(BasicRequest<Result> req) {
        if(this->ptr)
            disconnect(this->ptr.get());
        this->ptr = req.ptr;
        if(this->ptr) {
            connect(this->ptr.get(), &detail::RequestDataBase::finished, this, &BasicRequestWatcher<Result>::finished,
                    Qt::QueuedConnection);
            connect(this->ptr.get(), &detail::RequestDataBase::canceled, this, &BasicRequestWatcher<Result>::canceled,
                    Qt::QueuedConnection);
        }
    }
};


struct SimpleRequestResult {
    Q_GADGET

public:
    static constexpr quint32 type = 0x8543ec0d;
    Q_INVOKABLE QString toString() {
        return {};
    }
};


struct Thymio2WirelessDonglePairingResult {
    Q_GADGET

public:
    Thymio2WirelessDonglePairingResult() = default;
    Thymio2WirelessDonglePairingResult(uint16_t network, uint8_t channel) : m_networkId(network), m_channel(channel) {}

    static constexpr quint32 type = 0x478fa12d;
    Q_INVOKABLE quint16 networkId() const {
        return m_networkId;
    }

    Q_INVOKABLE quint8 channel() const {
        return m_channel;
    }

private:
    uint16_t m_networkId;
    uint16_t m_channel;
};

struct CompilationError {

    CompilationError(const QString& msg, quint32 pos, quint32 line, quint32 column)
        : m_message(msg), m_character(pos), m_line(line), m_column(column) {}

    CompilationError() = default;

    Q_INVOKABLE QString errorMessage() const {
        return m_message;
    }

    Q_INVOKABLE quint32 line() const {
        return m_line;
    }

    Q_INVOKABLE quint32 column() const {
        return m_column;
    }

    Q_INVOKABLE quint32 charater() const {
        return m_character;
    }

private:
    QString m_message;
    quint32 m_character = 0;
    quint32 m_line = 0;
    quint32 m_column = 0;
};


struct CompilationResult {
    static constexpr quint32 type = 0x61513229;
    Q_GADGET
public:
public:
    Q_INVOKABLE QString toString() const {
        if(m_errors.empty()) {
            return QStringLiteral("Compilation ok - bytecode: %1, variables %2").arg(int(m_bytecode_size), int(m_variables_size));
        }
        return QStringLiteral("Compilation error :").arg(m_errors.first().errorMessage());
    }

    Q_INVOKABLE bool success() const {
        return m_errors.empty();
    }

    Q_INVOKABLE size_t variables_size() const {
        return m_variables_size;
    }

    Q_INVOKABLE size_t bytecode_size() const {
        return m_bytecode_size;
    }

    Q_INVOKABLE QVector<CompilationError> errors() const {
        return m_errors;
    }

    Q_INVOKABLE CompilationError error() const {
        return m_errors.empty() ? CompilationError{} : m_errors.first();
    }

    Q_INVOKABLE size_t bytecode_total_size() const {
        return m_bytecode_total_size;
    }
    Q_INVOKABLE size_t variables_total_size() const {
        return m_variables_total_size;
    }

    static CompilationResult make_error(const QString& msg, quint32 pos, quint32 line, quint32 column) {
        CompilationResult r;
        r.m_errors.append(CompilationError(msg, pos, line, column));
        return r;
    }

    static CompilationResult make_success(size_t bytecode_size, size_t variables_size, size_t bytecode_total_size,
                                          size_t variables_total_size) {
        CompilationResult r;
        r.m_bytecode_size = bytecode_size;
        r.m_variables_size = variables_size;
        r.m_bytecode_total_size = bytecode_total_size;
        r.m_variables_total_size = variables_total_size;
        return r;
    }

private:
    QVector<CompilationError> m_errors;
    size_t m_bytecode_size = 0;
    size_t m_variables_size = 0;
    size_t m_bytecode_total_size = 0;
    size_t m_variables_total_size = 0;
};

struct SetBreakpointRequestResult {
    Q_GADGET

public:
    static constexpr quint32 type = 0xf2f36208;
    SetBreakpointRequestResult(QVector<unsigned> breakpoints) : m_breakpoints(std::move(breakpoints)) {}
    SetBreakpointRequestResult() = default;


    Q_INVOKABLE QString toString() {
        return {};
    }

    Q_INVOKABLE QVector<unsigned> breakpoints() const {
        return m_breakpoints;
    }

private:
    QVector<unsigned> m_breakpoints;
};


struct AsebaVMFunctionParameterDescription {
    Q_GADGET
    Q_PROPERTY(QString name READ name)
    Q_PROPERTY(uint16_t size READ size)
public:
    AsebaVMFunctionParameterDescription() = default;
    AsebaVMFunctionParameterDescription(const QString& name, uint16_t size) : m_name(name), m_size(size) {}

    QString name() const {
        return m_name;
    }
    uint16_t size() const {
        return m_size;
    }

private:
    QString m_name;
    uint16_t m_size;
};

struct AsebaVMFunctionDescription {

    Q_GADGET
    Q_PROPERTY(QString name READ name)
    Q_PROPERTY(QString description READ description)
    Q_PROPERTY(QVector<AsebaVMFunctionParameterDescription> parameters READ parameters)

public:
    AsebaVMFunctionDescription(const QString& name, const QString& description,
                               const QVector<AsebaVMFunctionParameterDescription>& params = {})
        : m_name(name), m_description(description), m_parameters(params) {}
    AsebaVMFunctionDescription() = default;

    QString name() const {
        return m_name;
    }

    QString description() const {
        return m_description;
    }

    QVector<AsebaVMFunctionParameterDescription> parameters() const {
        return m_parameters;
    }

private:
    QString m_name;
    QString m_description;
    QVector<AsebaVMFunctionParameterDescription> m_parameters;
};

using AsebaVMLocalEventDescription = AsebaVMFunctionDescription;

struct AsebaVMDescriptionRequestResult {
    Q_GADGET
    Q_PROPERTY(QVector<AsebaVMFunctionDescription> functions READ functions)
    Q_PROPERTY(QVector<AsebaVMLocalEventDescription> events READ events)

public:
    static constexpr quint32 type = 0xdb0d6e14;
    AsebaVMDescriptionRequestResult() = default;
    AsebaVMDescriptionRequestResult(const fb::NodeAsebaVMDescriptionT& description);


    Q_INVOKABLE QString toString() {
        return {};
    }

    QVector<AsebaVMFunctionDescription> functions() {
        return m_functions;
    }

    QVector<AsebaVMFunctionDescription> events() {
        return m_events;
    }

private:
    QVector<AsebaVMFunctionDescription> m_functions;
    QVector<AsebaVMLocalEventDescription> m_events;
};


using Request = BasicRequest<SimpleRequestResult>;
using RequestWatcher = BasicRequestWatcher<SimpleRequestResult>;

using Thymio2WirelessDonglePairingRequest = BasicRequest<Thymio2WirelessDonglePairingResult>;
using Thymio2WirelessDonglePairingRequestWatcher = BasicRequestWatcher<Thymio2WirelessDonglePairingResult>;

using CompilationRequest = BasicRequest<CompilationResult>;
using CompilationRequestWatcher = BasicRequestWatcher<CompilationResult>;

using BreakpointsRequest = BasicRequest<SetBreakpointRequestResult>;
using BreakpointsRequestWatcher = BasicRequestWatcher<SetBreakpointRequestResult>;

using AsebaVMDescriptionRequest = BasicRequest<AsebaVMDescriptionRequestResult>;
using AsebaVMDescriptionRequestWatcher = BasicRequestWatcher<AsebaVMDescriptionRequestResult>;

}  // namespace mobsya