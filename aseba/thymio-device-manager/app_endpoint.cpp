#include "app_endpoint.h"

namespace mobsya {
template class application_endpoint<websocket_t>;
template class application_endpoint<mobsya::tcp::socket>;
}  // namespace mobsya