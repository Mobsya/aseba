#if defined(__APPLE__) || defined(ANDROID) || defined(__ANDROID__) || (defined(__clang_major__) && defined(__GLIBCXX__)) || defined(HAS_MPARK)
#    include <mpark/variant.hpp>
namespace variant_ns = mpark;
#else
#    include <variant>
namespace variant_ns = std;
#endif
