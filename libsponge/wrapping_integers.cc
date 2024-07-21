#include "wrapping_integers.hh"

// Dummy implementation of a 32-bit wrapping integer

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

using namespace std;

//! Transform an "absolute" 64-bit sequence number (zero-indexed) into a WrappingInt32
//! \param n The input absolute 64-bit sequence number
//! \param isn The initial sequence number
WrappingInt32 wrap(uint64_t n, WrappingInt32 isn) { return WrappingInt32{isn + static_cast<uint32_t>(n)}; }

//! Transform a WrappingInt32 into an "absolute" 64-bit sequence number (zero-indexed)
//! \param n The relative sequence number
//! \param isn The initial sequence number
//! \param checkpoint A recent absolute 64-bit sequence number
//! \returns the 64-bit sequence number that wraps to `n` and is closest to `checkpoint`
//!
//! \note Each of the two streams of the TCP connection has its own ISN. One stream
//! runs from the local TCPSender to the remote TCPReceiver and has one ISN,
//! and the other stream runs from the remote TCPSender to the local TCPReceiver and
//! has a different ISN.
uint64_t unwrap(WrappingInt32 n, WrappingInt32 isn, uint64_t checkpoint) {
    uint32_t seq_num = n - isn;
    uint64_t absolute_seqno = static_cast<uint64_t>(seq_num) + ((checkpoint >> 32) << 32);
    if (absolute_seqno < checkpoint && checkpoint - absolute_seqno > (1ul << 31)) {
        absolute_seqno += (1ul << 32);
    } else if (absolute_seqno >= (1ul << 32) && absolute_seqno > checkpoint &&
               absolute_seqno - checkpoint > (1ul << 31))
        absolute_seqno -= (1ul << 32);
    return absolute_seqno;
}
