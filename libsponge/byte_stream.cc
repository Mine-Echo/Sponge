#include "byte_stream.hh"

// Dummy implementation of a flow-controlled in-memory byte stream.

// For Lab 0, please replace with a real implementation that passes the
// automated checks run by `make check_lab0`.

// You will need to add private members to the class declaration in `byte_stream.hh`

using namespace std;

ByteStream::ByteStream(const size_t capacity) : _capacity(capacity) {
    _buffer.resize(capacity);
}

size_t ByteStream::write(const string &data) {
    size_t real_len = data.size() > remaining_capacity() ? remaining_capacity() : data.size();
    _bytes_written += real_len;
    size_t tail = (_head + _size) % _capacity;
    if(tail + real_len <= _capacity) {
        copy(data.begin(), data.begin() + real_len, _buffer.begin() + tail);
    }
    else {
        copy(data.begin(), data.begin() + (_capacity - _head - _size), _buffer.begin() + tail);
        copy(data.begin() + (_capacity - _head - _size), data.begin() + real_len, _buffer.begin());
    }
    _size += real_len;
    return real_len;
}

//! \param[in] len bytes will be copied from the output side of the buffer
string ByteStream::peek_output(const size_t len) const {
    size_t real_len = len > _size ? _size : len;
    if(_head + real_len <= _capacity) {
        return string(_buffer.begin() + _head, _buffer.begin() + _head + real_len);
    }
    else {
        string ret(_buffer.begin() + _head, _buffer.end());
        ret.append(_buffer.begin(), _buffer.begin() + real_len - (_capacity - _head));
        return ret;
    }
}

//! \param[in] len bytes will be removed from the output side of the buffer
void ByteStream::pop_output(const size_t len) {
    size_t real_len = len > _size ? _size : len;
    _bytes_read += real_len;
    _head += real_len;
    _head %= _capacity;
    _size -= real_len;
}

//! Read (i.e., copy and then pop) the next "len" bytes of the stream
//! \param[in] len bytes will be popped and returned
//! \returns a string
std::string ByteStream::read(const size_t len) {
    string ret = peek_output(len);
    pop_output(len);
    return ret;
}

void ByteStream::end_input() { _input_end = true; }

bool ByteStream::input_ended() const { return _input_end; }

size_t ByteStream::buffer_size() const { return _size; }

bool ByteStream::buffer_empty() const { return !_size; }

bool ByteStream::eof() const { return input_ended() && buffer_empty(); }

size_t ByteStream::bytes_written() const { return _bytes_written; }

size_t ByteStream::bytes_read() const { return _bytes_read; }

size_t ByteStream::remaining_capacity() const { return _capacity - _size; }
