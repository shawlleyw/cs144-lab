#include "wrapping_integers.hh"
#include <bits/stdint-uintn.h>
#include <iostream>

// Dummy implementation of a 32-bit wrapping integer

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

//! Transform an "absolute" 64-bit sequence number (zero-indexed) into a WrappingInt32
//! \param n The input absolute 64-bit sequence number
//! \param isn The initial sequence number
WrappingInt32 wrap(uint64_t n, WrappingInt32 isn) {
    DUMMY_CODE(n, isn);
    uint32_t ret = n - ((n>>32)<<32);
    return WrappingInt32(ret+isn.raw_value());
}

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
    DUMMY_CODE(n, isn, checkpoint);
    WrappingInt32 prev = wrap(checkpoint, isn);
    uint64_t ret{0};
    const uint32_t &vn = n.raw_value(), &vprev = prev.raw_value();
    uint32_t d1 = vn-vprev, d2 = vprev-vn;
    if(d2 > checkpoint) {
        ret = checkpoint + d1;
    } else if(vn > vprev ) {
        ret = d1<d2 ? checkpoint+d1 : checkpoint-d2;   
        cout << "-(vprev - vn)" << -(vprev - vn) << endl;
    } else {
        ret = d2<d1 ? checkpoint-d2 : checkpoint+d1; 
    }
    return ret;
}