#include "tcp_sender.hh"

#include "tcp_config.hh"

#include <random>

// Dummy implementation of a TCP sender

// For Lab 3, please replace with a real implementation that passes the
// automated checks run by `make check_lab3`.

using namespace std;

//! \param[in] capacity the capacity of the outgoing byte stream
//! \param[in] retx_timeout the initial amount of time to wait before retransmitting the oldest outstanding segment
//! \param[in] fixed_isn the Initial Sequence Number to use, if set (otherwise uses a random ISN)
TCPSender::TCPSender(const size_t capacity, const uint16_t retx_timeout, const std::optional<WrappingInt32> fixed_isn)
    : _timer(retx_timeout), _isn(fixed_isn.value_or(WrappingInt32{random_device()()})), _stream(capacity) {}

uint64_t TCPSender::bytes_in_flight() const { return _bytes_in_flight; }

void TCPSender::fill_window() {
    if (!_syn_flag) {
        _syn_flag = true;
        TCPSegment segment;
        segment.header().syn = true;
        segment.header().seqno = _isn;
        _segment_in_flight.push(segment);
        _segments_out.push(segment);
        _bytes_in_flight += segment.length_in_sequence_space();
        _next_seqno += segment.length_in_sequence_space();
        // 计时器没打开则打开计时器
        if (!_timer.is_start()) {
            _timer.start();
        }
    }
    uint16_t window_size = _window_size > 0 ? _window_size : 1;
    uint16_t remain;
    while ((remain = window_size - _bytes_in_flight) > 0 && !_fin_flag) {
        TCPSegment segment;
        uint16_t size = remain > TCPConfig::MAX_PAYLOAD_SIZE ? TCPConfig::MAX_PAYLOAD_SIZE : remain;
        segment.payload() = Buffer(_stream.read(size));
        segment.header().seqno = wrap(_next_seqno, _isn);
        if (_stream.eof() && segment.length_in_sequence_space() < remain) {
            segment.header().fin = true;
            _fin_flag = true;
        }
        // 没有数据停止发送
        if (!segment.length_in_sequence_space()) {
            break;
        }
        _segment_in_flight.push(segment);
        _segments_out.push(segment);
        _bytes_in_flight += segment.length_in_sequence_space();
        _next_seqno += segment.length_in_sequence_space();
        // 如果计时器没有打开，开启计时器
        if (!_timer.is_start()) {
            _timer.start();
        }
    }
}

//! \param ackno The remote receiver's ackno (acknowledgment number)
//! \param window_size The remote receiver's advertised window size
void TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) {
    uint64_t absolute_ackno = unwrap(ackno, _isn, _recv_absolute_ackno);
    // ackno大于下一个要发送的序列号，不合理，忽略
    if (absolute_ackno > _next_seqno)
        return;
    _window_size = window_size;
    // ackno<=_recv_absolute_ackno，只需要获取窗口大小，无需后续操作
    if (absolute_ackno <= _recv_absolute_ackno)
        return;
    // 修改接收序列号和窗口大小
    _bytes_in_flight -= absolute_ackno - _recv_absolute_ackno;
    _recv_absolute_ackno = absolute_ackno;
    // 将已经确认的segment从_segment_in_flight队列中移除
    while (!_segment_in_flight.empty()) {
        TCPSegment segment = _segment_in_flight.front();
        if (unwrap(segment.header().seqno, _isn, _recv_absolute_ackno) + segment.length_in_sequence_space() <=
            _recv_absolute_ackno) {
            // _bytes_in_flight -= segment.length_in_sequence_space();
            _segment_in_flight.pop();
        } else {
            break;
        }
    }
    // 重置重传计数
    if (_segment_in_flight.empty()) {
        // 都被接收，停止计时器
        _timer.stop();
    } else {
        _timer.reset();
    }
    // 尝试发送新的数据
    fill_window();
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void TCPSender::tick(const size_t ms_since_last_tick) {
    if (_timer.is_start()) {
        _timer.tick(ms_since_last_tick);
        if (_timer.is_timeout()) {
            // 重传
            _segments_out.push(_segment_in_flight.front());
            if (_window_size != 0) {
                _timer.exponential_backof();
            }
            _timer.resetTime();
        }
    }
}

unsigned int TCPSender::consecutive_retransmissions() const { return _timer.get_consecutive_retransmissions(); }

void TCPSender::send_empty_segment() {
    TCPSegment segment;
    segment.header().seqno = wrap(_next_seqno, _isn);
    _segments_out.push(segment);
}
