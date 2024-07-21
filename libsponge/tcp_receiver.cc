#include "tcp_receiver.hh"

// Dummy implementation of a TCP receiver

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

using namespace std;

void TCPReceiver::segment_received(const TCPSegment &seg) {
    TCPHeader header = seg.header();
    // 没有收到syn且当前seg不是syn
    if (!_syn_flag) {
        if (header.syn) {
            _syn_flag = true;
            _isn = header.seqno;
        } else {
            return;
        }
    }
    WrappingInt32 seqno = header.seqno;
    // 一种特殊的错误情况，发送过来的第一个带payload且无syn数据包的seqno为isn，
    // 用_reassembler.getNextAssembledIdx()==0来判断是不是第一个带payload的数据包
    // 正常来说第一个带payload且不是syn的数据包的序列号应该是_isn+n (n可能为1,2...)
    if (seqno == _isn && !header.syn && _reassembler.getNextAssembledIdx() == 0)
        return;
    uint64_t check_point = _reassembler.stream_out().input_ended() ? _reassembler.getNextAssembledIdx() + 2
                                                                   : _reassembler.getNextAssembledIdx() + 1;
    uint64_t absolute_seqno = unwrap(seqno, _isn, check_point);
    uint64_t index = absolute_seqno > 0 ? absolute_seqno - 1 : 0;
    _reassembler.push_substring(seg.payload().copy(), index, header.fin);
}

optional<WrappingInt32> TCPReceiver::ackno() const {
    if (_syn_flag) {
        uint64_t absolute_seqno = _reassembler.stream_out().input_ended() ? _reassembler.getNextAssembledIdx() + 2
                                                                          : _reassembler.getNextAssembledIdx() + 1;
        return wrap(absolute_seqno, _isn);
    } else
        return nullopt;
}

size_t TCPReceiver::window_size() const { return _capacity - stream_out().buffer_size(); }
