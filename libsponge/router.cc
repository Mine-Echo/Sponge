#include "router.hh"

#include <iostream>

using namespace std;

// Dummy implementation of an IP router

// Given an incoming Internet datagram, the router decides
// (1) which interface to send it out on, and
// (2) what next hop address to send it to.

// For Lab 6, please replace with a real implementation that passes the
// automated checks run by `make check_lab6`.

// You will need to add private members to the class declaration in `router.hh`

//! \param[in] route_prefix The "up-to-32-bit" IPv4 address prefix to match the datagram's destination address against
//! \param[in] prefix_length For this route to be applicable, how many high-order (most-significant) bits of the route_prefix will need to match the corresponding bits of the datagram's destination address?
//! \param[in] next_hop The IP address of the next hop. Will be empty if the network is directly attached to the router (in which case, the next hop address should be the datagram's final destination).
//! \param[in] interface_num The index of the interface to send the datagram out on.
void Router::add_route(const uint32_t route_prefix,
                       const uint8_t prefix_length,
                       const optional<Address> next_hop,
                       const size_t interface_num) {
    cerr << "DEBUG: adding route " << Address::from_ipv4_numeric(route_prefix).ip() << "/" << int(prefix_length)
         << " => " << (next_hop.has_value() ? next_hop->ip() : "(direct)") << " on interface " << interface_num << "\n";
    // Your code here.
    uint32_t prefix = route_prefix & ((((static_cast<uint32_t>(1)) << prefix_length) - 1) << (32 - prefix_length));
    _route_table.at(prefix_length)[prefix] = std::pair<optional<Address>, size_t>(next_hop, interface_num);
}

//! \param[in] dgram The datagram to be routed
void Router::route_one_datagram(InternetDatagram &dgram) {
    // Your code here.
    if (dgram.header().ttl <= 1)
        return;
    dgram.header().ttl -= 1;
    uint32_t ip = dgram.header().dst;
    uint32_t mask = static_cast<uint32_t>(-1);
    cout << mask << endl;
    for (int i = 32; i >= 0; i--) {
        uint32_t prefix = ip & mask;
        auto it = _route_table.at(i).find(prefix);
        if (it != _route_table.at(i).end()) {
            interface(it->second.second)
                .send_datagram(dgram,
                               it->second.first.has_value() ? it->second.first.value()
                                                            : Address::from_ipv4_numeric(dgram.header().dst));
            return;
        }
        mask ^= (static_cast<uint32_t>(1)) << (32 - i);
    }
}

void Router::route() {
    // Go through all the interfaces, and route every incoming datagram to its proper outgoing interface.
    for (auto &interface : _interfaces) {
        auto &queue = interface.datagrams_out();
        while (not queue.empty()) {
            route_one_datagram(queue.front());
            queue.pop();
        }
    }
}
