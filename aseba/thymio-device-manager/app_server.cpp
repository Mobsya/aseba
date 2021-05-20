#include "app_server.h"

namespace mobsya {

#ifdef HAS_FB_WS
    template class application_server<websocket_t>;
#endif

#ifdef HAS_FB_TCP
    template class application_server<mobsya::tcp::socket>;
#endif

}  // namespace mobsya
