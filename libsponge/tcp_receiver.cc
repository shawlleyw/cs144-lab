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
    }// many syn ??
    
    string data = seg.payload().copy();
    uint64_t seq = unwrap(seg.header().seqno, _isn, _checkpoint);
    uint64_t index = seq - _offset;
    bool ret = index < _capacity + _reassembler.stream_out().bytes_read() 
               && index + seg.length_in_sequence_space() >= _checkpoint;
    int added{};
    if(seg.header().syn) {
        _offset ++;
        added ++;
    }
    if(seg.header().fin) {
        _offset ++;
        added ++;
    }
    if(seg.payload().copy().empty()&&!added) {
        _offset ++;
    } 
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