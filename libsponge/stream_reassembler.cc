#include "stream_reassembler.hh"

// Dummy implementation of a stream reassembler.

// For Lab 1, please replace with a real implementation that passes the
// automated checks run by `make check_lab1`.

// You will need to add private members to the class declaration in `stream_reassembler.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

StreamReassembler::StreamReassembler(const size_t capacity) : _output(capacity), _capacity(capacity) , _buffer(capacity, -1), _hold(capacity, false) {}

//! \details This function accepts a substring (aka a segment) of bytes,
//! possibly out-of-order, from the logical stream, and assembles any newly
//! contiguous substrings and writes them into the output stream in order.
void StreamReassembler::push_substring(const string &data, const size_t index, const bool eof) {
    DUMMY_CODE(data, index, eof);
    size_t update = _output.bytes_read() - _unread;
    if(update) {
        for(size_t i = 0; i < _capacity; i++) {
            if(i+update < _capacity) {
                _hold[i] = _hold[i+update];
                _buffer[i] = _buffer[i+update];
            } else {
                _hold[i] = false;
            }
        }    
    }
    _unread = _output.bytes_read();
    for(size_t i = 0; index + i < _unread + _capacity && i < data.length(); i++) {
        if(index+i >= _unread){
            _buffer[index+i-_unread] = data[i];
            _hold[index+i-_unread] = true;
        }
    }
    string assembled{};
    for(size_t i = _output.bytes_written() - _unread; i < _capacity; i++) {
        if(_hold[i]) {
            _hold[i] = false;
            assembled += _buffer[i];
        } else {
            break;
        }
    }
    _output.write(assembled);
    if(_unread + _capacity >= data.length() + index  && eof){
        _eof = true;
        _end_pos = index + data.length();
    }
    if(_eof && _output.bytes_written() == _end_pos) {
        _output.end_input();
    }
}

size_t StreamReassembler::unassembled_bytes() const { 
    return this->count_buffer(); 
}

bool StreamReassembler::empty() const { 
    return this->count_buffer() == 0; 
}

size_t StreamReassembler::count_buffer () const {
    int ret = 0;
    for(auto v:_hold) {
        if(v) {
            ret ++;
        }
    }
    return ret;
}
