#include "app_endpoint.h"

namespace mobsya {

#ifdef HAS_FB_WS
    template class application_endpoint<websocket_t>;
#endif

#ifdef HAS_FB_TCP
    template class application_endpoint<mobsya::tcp::socket>;
#endif

}  // namespace mobsya
