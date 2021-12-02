#include "tcp_receiver.hh"
#include "wrapping_integers.hh"
#include <cstdint>
#include <iostream>

// Dummy implementation of a TCP receiver

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

bool TCPReceiver::segment_received(const TCPSegment &seg) {
    DUMMY_CODE(seg);
    if(!_set_syn && !seg.header().syn) {
        return false;
    }
    if(seg.header().syn && ! _set_syn) {
        _set_syn = true;
        _isn = seg.header().seqno;
    }
    
    string data = seg.payload().copy();
    uint64_t seq = unwrap(seg.header().seqno, _isn, _checkpoint);
    uint64_t index = seq - _offset;
    size_t length = seg.length_in_sequence_space();
    // accept an empty segment (consider its length as 1)
    if(length == 0) {
        length = 1;
    }
    bool ret = index < _capacity + _reassembler.stream_out().bytes_read() 
               && index + length > _reassembler.stream_out().bytes_written();
    if(!ret) {
        return false;
    }
    _offset += seg.header().syn;
    _offset += seg.header().fin;
    _reassembler.push_substring(data, index, seg.header().fin);
    _checkpoint = _reassembler.stream_out().bytes_written()+_offset;
    return ret;
}

optional<WrappingInt32> TCPReceiver::ackno() const { 
    if(!_set_syn) {
        return {};
    } else {
        return wrap(_checkpoint, _isn); 
    }
}

size_t TCPReceiver::window_size() const { 
    return _capacity + _reassembler.stream_out().bytes_read() - _reassembler.stream_out().bytes_written(); 
}