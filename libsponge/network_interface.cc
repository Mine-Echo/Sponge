#include "network_interface.hh"

#include "arp_message.hh"
#include "ethernet_frame.hh"

#include <iostream>

// Dummy implementation of a network interface
// Translates from {IP datagram, next hop address} to link-layer frame, and from link-layer frame to IP datagram

// For Lab 5, please replace with a real implementation that passes the
// automated checks run by `make check_lab5`.

// You will need to add private members to the class declaration in `network_interface.hh`

using namespace std;

//! \param[in] ethernet_address Ethernet (what ARP calls "hardware") address of the interface
//! \param[in] ip_address IP (what ARP calls "protocol") address of the interface
NetworkInterface::NetworkInterface(const EthernetAddress &ethernet_address, const Address &ip_address)
    : _ethernet_address(ethernet_address), _ip_address(ip_address) {
    cerr << "DEBUG: Network interface has Ethernet address " << to_string(_ethernet_address) << " and IP address "
         << ip_address.ip() << "\n";
}

//! \param[in] dgram the IPv4 datagram to be sent
//! \param[in] next_hop the IP address of the interface to send it to (typically a router or default gateway, but may also be another host if directly connected to the same network as the destination)
//! (Note: the Address type can be converted to a uint32_t (raw 32-bit IP address) with the Address::ipv4_numeric() method.)
void NetworkInterface::send_datagram(const InternetDatagram &dgram, const Address &next_hop) {
    // convert IP address of next hop to raw 32-bit representation (used in ARP header)
    const uint32_t next_hop_ip = next_hop.ipv4_numeric();
    EthernetFrame eth_frame;
    eth_frame.header().src = _ethernet_address;
    eth_frame.header().type = EthernetHeader::TYPE_IPv4;
    eth_frame.payload() = dgram.serialize();
    if (_arp_cache.find(next_hop_ip) == _arp_cache.end()) {
        // 无缓存，如果五秒内未发送过ARP则发送ARP请求
        if (_last_arp_request.find(next_hop_ip) == _last_arp_request.end()) {
            ARPMessage arp_request;
            arp_request.opcode = ARPMessage::OPCODE_REQUEST;
            arp_request.sender_ethernet_address = _ethernet_address;
            arp_request.sender_ip_address = _ip_address.ipv4_numeric();
            // arp_request.target_ethernet_address = ETHERNET_BROADCAST;
            arp_request.target_ip_address = next_hop_ip;
            EthernetFrame arp_eth_frame;
            arp_eth_frame.header().src = _ethernet_address;
            arp_eth_frame.header().dst = ETHERNET_BROADCAST;
            arp_eth_frame.header().type = EthernetHeader::TYPE_ARP;
            arp_eth_frame.payload() = arp_request.serialize();
            // 发送
            _frames_out.push(arp_eth_frame);
            // 记录已经发送过该IP的ARP请求，ttl为5000
            _last_arp_request[next_hop_ip] = 5000;
        }
        // 将dgram放入等待队列
        if (_frames_wait.find(next_hop_ip) != _frames_wait.end())
            _frames_wait.at(next_hop_ip).push_back(eth_frame);
        else
            _frames_wait[next_hop_ip].push_back(eth_frame);
    } else {
        // 有ARP缓存，直接发送
        eth_frame.header().dst = _arp_cache.at(next_hop_ip).first;
        _frames_out.push(eth_frame);
    }
}

//! \param[in] frame the incoming Ethernet frame
optional<InternetDatagram> NetworkInterface::recv_frame(const EthernetFrame &frame) {
    // 不是广播，mac地址不等于此接口的mac地址
    if (frame.header().dst != ETHERNET_BROADCAST && frame.header().dst != _ethernet_address)
        return nullopt;
    InternetDatagram dgram;
    if (frame.header().type == EthernetHeader::TYPE_IPv4 && dgram.parse(frame.payload()) == ParseResult::NoError) {
        return dgram;
    }
    ARPMessage arp;
    if (frame.header().type == EthernetHeader::TYPE_ARP && arp.parse(frame.payload()) == ParseResult::NoError) {
        // ARP包，更新缓存，ttl为30
        _arp_cache[arp.sender_ip_address] = std::pair<EthernetAddress, size_t>(arp.sender_ethernet_address, 30000);
        // 发送等待发送的数据包
        if (_frames_wait.find(arp.sender_ip_address) != _frames_wait.end()) {
            vector<EthernetFrame> &v = _frames_wait.at(arp.sender_ip_address);
            for (EthernetFrame &eth_frame : v) {
                eth_frame.header().dst = arp.sender_ethernet_address;
                _frames_out.push(eth_frame);
            }
            _frames_wait.erase(arp.sender_ip_address);
        }
        // 如果是请求包且目标ip地址是此接口ip地址
        if (arp.opcode == ARPMessage::OPCODE_REQUEST && arp.target_ip_address == _ip_address.ipv4_numeric()) {
            // 发送ARP回应
            ARPMessage arp_reply;
            arp_reply.opcode = ARPMessage::OPCODE_REPLY;
            arp_reply.sender_ethernet_address = _ethernet_address;
            arp_reply.sender_ip_address = _ip_address.ipv4_numeric();
            arp_reply.target_ethernet_address = arp.sender_ethernet_address;
            arp_reply.target_ip_address = arp.sender_ip_address;
            EthernetFrame arp_eth_frame;
            arp_eth_frame.header().src = _ethernet_address;
            arp_eth_frame.header().dst = arp_reply.target_ethernet_address;
            arp_eth_frame.header().type = EthernetHeader::TYPE_ARP;
            arp_eth_frame.payload() = arp_reply.serialize();
            // 发送
            _frames_out.push(arp_eth_frame);
        }
    }
    return nullopt;
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void NetworkInterface::tick(const size_t ms_since_last_tick) {
    // 更新ARP缓存
    for (auto it = _arp_cache.begin(); it != _arp_cache.end();) {
        if (it->second.second <= ms_since_last_tick) {
            auto tmpIt = it;
            ++it;
            _arp_cache.erase(tmpIt);
        } else {
            it->second.second -= ms_since_last_tick;
            ++it;
        }
    }
    // 更新_last_arp_request
    for (auto it = _last_arp_request.begin(); it != _last_arp_request.end();) {
        if (it->second <= ms_since_last_tick) {
            auto tmpIt = it;
            ++it;
            _last_arp_request.erase(tmpIt);
        } else {
            it->second -= ms_since_last_tick;
            ++it;
        }
    }
}
