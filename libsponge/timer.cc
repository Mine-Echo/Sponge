#include "timer.hh"

Timer::Timer(const unsigned short int retx_timeout)
    : _RTO(retx_timeout), _initial_retransmission_timeout(retx_timeout) {}

void Timer::start() {
    _start = true;
    reset();
}

void Timer::stop() { _start = false; }

void Timer::reset() {
    _RTO = _initial_retransmission_timeout;
    _consecutive_retransmissions = 0;
    resetTime();
}

void Timer::resetTime() { _time = 0; }

void Timer::tick(const long unsigned int ms_since_last_tick) { _time += ms_since_last_tick; }

void Timer::exponential_backof() {
    _RTO *= 2;
    _consecutive_retransmissions += 1;
}
