#include "tcp_connection.hh"

#include <iostream>

// Dummy implementation of a TCP connection

// For Lab 4, please replace with a real implementation that passes the
// automated checks run by `make check`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

size_t TCPConnection::remaining_outbound_capacity() const { 
    return _sender.stream_in().remaining_capacity();
}

size_t TCPConnection::bytes_in_flight() const { 
    return _sender.bytes_in_flight();
}

size_t TCPConnection::unassembled_bytes() const { 
    return _receiver.unassembled_bytes(); 
}

size_t TCPConnection::time_since_last_segment_received() const { 
    return _time_cur - _last_recieved; 
}

void TCPConnection::pack_outbound_segment(TCPSegment &outbound_seg, bool send_rst) {
    auto receiver_ack = _receiver.ackno();
    if(receiver_ack.has_value()) {
        outbound_seg.header().ackno = receiver_ack.value();
        outbound_seg.header().ack = true;
    }
    outbound_seg.header().win = _receiver.window_size();
    if(send_rst) {
        outbound_seg.header().rst = true;
    }
}
void TCPConnection::segment_received(const TCPSegment &seg) { 
    DUMMY_CODE(seg);
    _last_recieved = _time_cur; 
    if(in_syn_sent() && seg.header().ack && seg.payload().size() > 0) {
        return;
    }
    bool send_empty_ack = false;
    bool res_receiver = _receiver.segment_received(seg);
    if(!res_receiver) {
        send_empty_ack = true;
    }
    if(_sender.next_seqno_absolute() > 0 && seg.header().ack) {
        if(!_sender.ack_received(seg.header().ackno, seg.header().win)) {
            send_empty_ack = true;
        }
    }
    if(seg.header().rst) {
        if(in_syn_sent() && !seg.header().ack) {
            return;
        }
        unclean_shutdown(false);
        return;        
    }
    if(seg.length_in_sequence_space() > 0) {
        send_empty_ack = true;
    }
    if(res_receiver) {
        // deal with SYN flag
        if(seg.header().syn && _sender.next_seqno_absolute() == 0) {
            connect();
            return;
        }
        // deal with FIN flag
        if(seg.header().fin) {
            _receiver_fin_received = true;
        }
        // Prerqq #3: The outbound stream has been fully acknowledged by the remote peer.
        if(_sender_fin_sent && _sender.bytes_in_flight() == 0) { 
            _end_sender = true;
        }
        // Prereq #1: The inbound stream has been fully assembled and has ended.
        if(_receiver_fin_received && _receiver.unassembled_bytes() == 0 ) {
            _end_receiver = true;
        }
        if(_end_receiver && !_sender_fin_sent) {
            _linger_after_streams_finish = false;
        }
    }
    if(send_empty_ack && _receiver.ackno().has_value()) {
        _sender.send_empty_segment();
    }
    send_to_outbound(false); 
}

bool TCPConnection::active() const { 
    if(_rst_flag) {
        return false; 
    }
    if(_end_receiver && _end_sender && !_linger_after_streams_finish) {
        return false;
    }
    return _connection_alive;
}

void TCPConnection::send_to_outbound(bool send_syn, bool send_rst) { 
    if(send_syn || _receiver.ackno().has_value()) {
        _sender.fill_window();
    }    
    while(!_sender.segments_out().empty()) {
        TCPSegment seg_send = _sender.segments_out().front();
        _sender.segments_out().pop();
        pack_outbound_segment(seg_send, send_rst);
        _segments_out.push(seg_send);
    }
}

size_t TCPConnection::write(const string &data) {
    DUMMY_CODE(data);
    size_t bytes_written = _sender.stream_in().write(data);
    send_to_outbound(false);
    return bytes_written;
}

//! \param[in] ms_since_last_tick number of milliseconds since the last call to this method
void TCPConnection::tick(const size_t ms_since_last_tick) { 
    DUMMY_CODE(ms_since_last_tick); 
    _sender.tick(ms_since_last_tick); // ** send all the timeout segments ? seems not... **
    if(_sender.consecutive_retransmissions() > TCPConfig::MAX_RETX_ATTEMPTS) {
        //! send a RST to peer
        unclean_shutdown(true);
        return;
    }
    send_to_outbound(false);
    _time_cur += ms_since_last_tick;
    
    if(_end_receiver && _end_sender && _time_cur - _last_recieved >= 10 * _cfg.rt_timeout) {
        _connection_alive = false;
    }
}

void TCPConnection::end_input_stream() {
    if(_end_receiver && !_sender_fin_sent) {
        _linger_after_streams_finish = false;
    }
    // prereq #2:  The outbound stream has been ended by the local application 
    //             and fully sent (including the fact that it ended,
    //             and that i.e. a segment with fin ) to the remote peer.
    _sender.stream_in().end_input();
    write(string());
    _sender_fin_sent = true;
}

void TCPConnection::unclean_shutdown(bool send_rst) {
    _rst_flag = true;
    _receiver.stream_out().set_error();
    _sender.stream_in().set_error();
    if(send_rst) {
        if(_sender.segments_out().empty()) {
            _sender.send_empty_segment();
        }
        send_to_outbound(false, true);
    }
}

void TCPConnection::connect() {
    send_to_outbound(true);
}

TCPConnection::~TCPConnection() {
    try {
        if (active()) {
            cerr << "Warning: Unclean shutdown of TCPConnection\n";

            // Your code here: need to send a RST segment to the peer 
            unclean_shutdown(true);
        }
    } catch (const exception &e) {
        std::cerr << "Exception destructing TCP FSM: " << e.what() << std::endl;
    }
}

bool TCPConnection::in_syn_sent() {
    return _sender.next_seqno_absolute() > 0 && _sender.next_seqno_absolute() == _sender.bytes_in_flight();
}