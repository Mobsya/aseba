#include "app_server.h"

namespace mobsya {

template class application_server<websocket_t>;
template class application_server<mobsya::tcp::socket>;

}  // namespace mobsya