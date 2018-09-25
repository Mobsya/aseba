#pragma once

namespace mobsya::tdm {
constexpr const unsigned protocolVersion = 1;
constexpr const unsigned minProtocolVersion = 1;
constexpr const unsigned maxAppEndPointMessageSize = 102400;  // 100k ought to be enough for anyone
}  // namespace mobsya::tdm