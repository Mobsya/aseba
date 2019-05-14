#pragma once
#include <boost/asio/io_service.hpp>
#include <random>
#include <limits>

namespace mobsya {

class aseba_nodeid_generator : public boost::asio::detail::service_base<aseba_nodeid_generator> {
public:
    aseba_nodeid_generator(boost::asio::execution_context& io_context)
        : boost::asio::detail::service_base<aseba_nodeid_generator>(static_cast<boost::asio::io_context&>(io_context))
        , distribution(1) {

        std::random_device r;
        std::seed_seq seq{r(), r(), r(), r(), r(), r(), r(), r(), r(), r()};
        generator.seed(seq);
    }
    uint16_t generate_network_id() {
        uint16_t n;
        do {
            n = distribution(generator);
        } while(n == 0x404F);
        return n;
    }

    uint16_t generate() {
        return distribution(generator);
    }

private:
    std::default_random_engine generator;
    std::uniform_int_distribution<int16_t> distribution;
};

}  // namespace mobsya