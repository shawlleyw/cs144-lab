#include "tcp_sender.hh"

#include "tcp_config.hh"
#include "tcp_segment.hh"
#include "wrapping_integers.hh"

#include <cstdint>
#include <algorithm>
#include <random>
#include <tuple>
#include <iostream>

// Dummy implementation of a TCP sender

// For Lab 3, please replace with a real implementation that passes the
// automated checks run by `make check_lab3`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

//! \param[in] capacity the capacity of the outgoing byte stream
//! \param[in] retx_timeout the initial amount of time to wait before retransmitting the oldest outstanding segment
//! \param[in] fixed_isn the Initial Sequence Number to use, if set (otherwise uses a random ISN)
TCPSender::TCPSender(const size_t capacity, const uint16_t retx_timeout, const std::optional<WrappingInt32> fixed_isn)
    : _isn(fixed_isn.value_or(WrappingInt32{random_device()()}))
    , _initial_retransmission_timeout{retx_timeout}
    , _stream(capacity)
    , _retransmission_timeout(retx_timeout) {}

uint64_t TCPSender::bytes_in_flight() const { 
    return _next_seqno - _checkpoint; 
}

void TCPSender::fill_window() {
    bool segment_sent{};
    if(!_syn_sent) {
        _syn_sent = true;
        TCPSegment segment{};
        segment.header().syn = true;
        segment.header().seqno = wrap(_next_seqno, _isn);
        _retransmission_queue.push(segment, _next_seqno);
        _next_seqno ++;
        _segments_out.push(segment);
        segment_sent = true;
    }
    while(!_fin_sent && _window_end > _next_seqno) {
        TCPSegment segment{};
        size_t data_length = min(TCPConfig::MAX_PAYLOAD_SIZE, _window_end-_next_seqno);
        Buffer buffer(_stream.read(data_length));
        segment.payload() = buffer;
        if(_window_end - _next_seqno > segment.length_in_sequence_space() && _stream.eof()) {
            segment.header().fin = true;
            _fin_sent = true;
        }
        //cerr << "here the buffer: " << buffer.str() << " | here the fin: " << segment.header().fin << endl;
        if(segment.length_in_sequence_space() == 0) {
            break;
        }
        segment.header().seqno = wrap(_next_seqno, _isn);
        _retransmission_queue.push(segment, _next_seqno);
        _next_seqno += segment.length_in_sequence_space();
        _segments_out.push(segment);
        segment_sent = true;
    }
    if(!_timer_set && segment_sent) {
        _timer_set = true;
        _timer_start = _time_miliseconds;
    }
}

//! \param ackno The remote receiver's ackno (acknowledgment number)
//! \param window_size The remote receiver's advertised window size
//! \returns `false` if the ackno appears invalid (acknowledges something the TCPSender hasn't sent yet)
bool TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) {
    DUMMY_CODE(ackno, window_size);
    _retransmission_timeout = _initial_retransmission_timeout;
    _consecutive_retransmissions = 0;
    uint64_t ack = unwrap(ackno, _isn, _checkpoint);
    if(ack > _next_seqno) {
        return false;
    }
    
    if(ack > _checkpoint) {
        _checkpoint = ack;
    }
    _window_start = _checkpoint;
    _window_end = _checkpoint + window_size;
    _retransmission_queue.pop(ack);
    //! reset the retransmission timer
    if (!_retransmission_queue.empty()){
        _timer_set = true;
        _timer_start = _time_miliseconds;
    } else {
        _timer_set = false;
    }
    return true;
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void TCPSender::tick(const size_t ms_since_last_tick) { 
    DUMMY_CODE(ms_since_last_tick); 
    _time_miliseconds += ms_since_last_tick;
    if (_timer_set && _time_miliseconds - _timer_start >= _retransmission_timeout) {
        //! resend
        TCPSegment segment{};
        std::tie(segment, std::ignore) = _retransmission_queue.front();
        _segments_out.push(segment);

        if (_window_end > _window_start) {
            _consecutive_retransmissions ++;
            _retransmission_timeout <<= 1;
        }

        _timer_set = true;
        _timer_start = _time_miliseconds;
        
    }
}

unsigned int TCPSender::consecutive_retransmissions() const { 
    return _consecutive_retransmissions; 
}

void TCPSender::send_empty_segment() {
    TCPSegment segment{};
    //cerr << "here send an empty one" << endl;
    segment.header().seqno = wrap(_next_seqno, _isn);
    _segments_out.push(segment);
}

bool RetransmissionQueue::empty() {
    return _segments_queue.empty();
}

void RetransmissionQueue::reset() {
    while(!_segments_queue.empty()) {
        _segments_queue.pop();
        _seqno_queue.pop();
    }
}

void RetransmissionQueue::pop(uint64_t ackno) {
    while(!_segments_queue.empty()) {
        TCPSegment head_seg = _segments_queue.front();
        uint64_t seqno = _seqno_queue.front();
        if(ackno < seqno + head_seg.length_in_sequence_space()) {
            break;
        }
        _segments_queue.pop();
        _seqno_queue.pop();
    }
}

void RetransmissionQueue::push(TCPSegment segment, uint64_t seqno) {
    _segments_queue.push(segment);
    _seqno_queue.push(seqno);
}

std::tuple<TCPSegment, uint64_t> RetransmissionQueue::front() {
    TCPSegment segment_ret = _segments_queue.front();
    uint64_t seqno = _seqno_queue.front();
    return std::tie(segment_ret, seqno);
}