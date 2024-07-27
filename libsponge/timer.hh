#ifndef SPONGE_LIBSPONGE_TIMER_HH
#define SPONGE_LIBSPONGE_TIMER_HH

class Timer {
  private:
    // 计时开始
    bool _start{};
    // 超时重传时间
    unsigned int _RTO;
    // 当前的时间
    unsigned int _time{0};
    // 连续重传次数
    unsigned int _consecutive_retransmissions{0};
    // 初始重传次数
    unsigned int _initial_retransmission_timeout;

  public:
    Timer(const unsigned short int retx_timeout);
    unsigned int get_consecutive_retransmissions() const { return _consecutive_retransmissions; }
    bool is_start() const { return _start; };
    bool is_timeout() const { return _time >= _RTO; };
    void start();
    void stop();
    void reset();
    void resetTime();
    void tick(const long unsigned int ms_since_last_tick);
    void exponential_backof();
};

#endif