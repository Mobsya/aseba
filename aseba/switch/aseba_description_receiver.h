#pragma once
#include "aseba_message_parser.h"
#include "log.h"

namespace mobsya {

template <class AsyncReadStream, class Handler>
class read_aseba_description_message_op;

using read_description_message_op_cb_t = void(boost::system::error_code, uint16_t node, Aseba::TargetDescription);

template <class AsyncReadStream, class CompletionToken>
BOOST_ASIO_INITFN_RESULT_TYPE(CompletionToken, read_aseba_message_op_cb_t)
async_read_aseba_description_message(AsyncReadStream& stream, uint16_t node, CompletionToken&& token) {
    static_assert(boost::beast::is_async_read_stream<AsyncReadStream>::value, "AsyncReadStream requirements not met");

    boost::asio::async_completion<CompletionToken, read_description_message_op_cb_t> init{token};
    read_aseba_description_message_op<AsyncReadStream,
                                      BOOST_ASIO_HANDLER_TYPE(CompletionToken, read_aseba_message_op_cb_t)>{
        stream, node, std::forward<CompletionToken>(init.completion_handler)}();

    return init.result.get();
}


template <class AsyncReadStream, class Handler>
class read_aseba_description_message_op {
    struct state {
        AsyncReadStream& stream;
        uint16_t node;
        Aseba::TargetDescription description;
        struct {
            uint16_t variables{0}, event{0}, functions{0};
        } message_counter;

        explicit state(Handler const&, uint16_t node, AsyncReadStream& stream) : stream(stream), node(node) {}
    };
    boost::beast::handler_ptr<state, Handler> m_p;

public:
    read_aseba_description_message_op(read_aseba_description_message_op&&) = default;
    read_aseba_description_message_op(read_aseba_description_message_op const&) = default;

    template <class DeducedHandler, class... Args>
    read_aseba_description_message_op(AsyncReadStream& stream, uint16_t node, DeducedHandler&& handler)
        : m_p(std::forward<DeducedHandler>(handler), node, stream) {}

    using allocator_type = boost::asio::associated_allocator_t<Handler>;

    allocator_type get_allocator() const noexcept {
        return boost::asio::get_associated_allocator(m_p.handler());
    }

    using executor_type =
        boost::asio::associated_executor_t<Handler, decltype(std::declval<AsyncReadStream&>().get_executor())>;

    executor_type get_executor() const noexcept {
        return boost::asio::get_associated_executor(m_p.handler(), m_p->stream.get_executor());
    }

    void operator()() {
        auto& state = *m_p;
        return mobsya::async_read_aseba_message(state.stream, std::move(*this));
    }

    void operator()(boost::system::error_code ec, std::shared_ptr<Aseba::Message> msg) {
        state& s = *m_p;
        Aseba::TargetDescription& desc = s.description;
        auto& counter = s.message_counter;
        const uint16_t node = s.node;


        if(ec) {
            mLogError("Error in read_aseba_description_message_op while expecting an Aseba::Message");
            m_p.invoke(ec, node, Aseba::TargetDescription());
            return;
        }

        // The node was broadcasting a message we do not care for at the moment
        if(msg->source != node) {
            return mobsya::async_read_aseba_message(s.stream, std::move(*this));
        }

        const auto safe_description_update = [](auto&& description, auto& list, uint16_t& counter) {
            if(counter >= list.size())
                return;
            auto it = std::find_if(std::begin(list), std::end(list),
                                   [&description](const auto& variable) { return variable.name == description.name; });
            if(it != std::end(list))
                return;
            list[counter++] = std::forward<decltype(description)>(description);
        };

        msg->dump(std::wcout);
        std::wcout << std::endl;
        switch(msg->type) {
            case ASEBA_MESSAGE_DESCRIPTION:
                if(!desc.name.empty()) {
                    mLogWarn("Received an Aseba::Description but we already got one");
                }
                desc = *static_cast<const Aseba::Description*>(msg.get());
                break;
            case ASEBA_MESSAGE_NAMED_VARIABLE_DESCRIPTION:
                safe_description_update(*static_cast<const Aseba::NamedVariableDescription*>(msg.get()),
                                        desc.namedVariables, counter.variables);
                break;
            case ASEBA_MESSAGE_LOCAL_EVENT_DESCRIPTION:
                safe_description_update(*static_cast<const Aseba::LocalEventDescription*>(msg.get()), desc.localEvents,
                                        counter.event);
                break;
            case ASEBA_MESSAGE_NATIVE_FUNCTION_DESCRIPTION:
                safe_description_update(*static_cast<const Aseba::NativeFunctionDescription*>(msg.get()),
                                        desc.nativeFunctions, counter.functions);
                break;
            default: break;
        }
        const bool ready = !desc.name.empty() && counter.variables == desc.namedVariables.size() &&
            counter.event == desc.localEvents.size() && counter.functions == desc.nativeFunctions.size();

        if(!ready) {
            return mobsya::async_read_aseba_message(s.stream, std::move(*this));
        }
        Aseba::TargetDescription sd = std::move(s.description);
        m_p.invoke(ec, node, sd);
    }
};
}  // namespace mobsya
