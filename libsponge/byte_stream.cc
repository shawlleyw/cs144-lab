#include "byte_stream.hh"

// Dummy implementation of a flow-controlled in-memory byte stream.

// For Lab 0, please replace with a real implementation that passes the
// automated checks run by `make check_lab0`.

// You will need to add private members to the class declaration in `byte_stream.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

ByteStream::ByteStream(const size_t capacity) { 
    DUMMY_CODE(capacity); 
    this->_capacity = capacity;
    this->_hold = 0;
    this->_total_read = 0;
    this->_total_write = 0;
    this->_error = false;
    this->_eof = false;
}

size_t ByteStream::write(const string &data) {
    DUMMY_CODE(data);
    size_t write_now = data.length();
    size_t ret = 0;
    if (this->_capacity - this->_hold >= write_now) {
        this->_hold += write_now;
        ret = write_now;
    } else {
        ret = this->_capacity - this->_hold;
        this->_hold = this->_capacity;
    }
    this->_buffer += data.substr(0, ret);
    this->_total_write += ret;
    return ret;
}

//! \param[in] len bytes will be copied from the output side of the buffer
string ByteStream::peek_output(const size_t len) const {
    DUMMY_CODE(len);
    return this->_buffer.substr(0, len);
}

//! \param[in] len bytes will be removed from the output side of the buffer
void ByteStream::pop_output(const size_t len) { 
    DUMMY_CODE(len); 
    this->_buffer.erase(0, len);
    this->_hold -= len;
    this->_total_read += len;
}

//! Read (i.e., copy and then pop) the next "len" bytes of the stream
//! \param[in] len bytes will be popped and returned
//! \returns a string
std::string ByteStream::read(const size_t len) {
    DUMMY_CODE(len);
    string ret = this->_buffer.substr(0, len);
    this->_buffer.erase(0, len);
    this->_total_read += ret.length();
    this->_hold -= ret.length();
    return ret;
}

void ByteStream::end_input() {
    this->_eof = true;
}

bool ByteStream::input_ended() const { 
    return this->_eof; 
}

size_t ByteStream::buffer_size() const { 
    return this->_hold; 
}

bool ByteStream::buffer_empty() const { 
    return this->_hold == 0; 
}

bool ByteStream::eof() const { 
    return this->input_ended() && this->_total_read == this->_total_write; 
}

size_t ByteStream::bytes_written() const { 
    return this->_total_write; 
}

size_t ByteStream::bytes_read() const { 
    return this->_total_read; 
}

size_t ByteStream::remaining_capacity() const {
    return this->_capacity - this->_hold; 
}
