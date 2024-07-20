#include "stream_reassembler.hh"

#include <cassert>
// #include <iostream>
// Dummy implementation of a stream reassembler.

// For Lab 1, please replace with a real implementation that passes the
// automated checks run by `make check_lab1`.

// You will need to add private members to the class declaration in `stream_reassembler.hh`

using namespace std;

StreamReassembler::StreamReassembler(const size_t capacity) : _output(capacity), _capacity(capacity) {}

//! \details This function accepts a substring (aka a segment) of bytes,
//! possibly out-of-order, from the logical stream, and assembles any newly
//! contiguous substrings and writes them into the output stream in order.
void StreamReassembler::push_substring(const string &data, const size_t index, const bool eof) {
    // 大于容量不接受
    if (index >= _capacity - _output.buffer_size() + _next_assembled_idx) {
        return;
    }
    // 删除被覆盖的strs
    auto delete_iter = _unassemble_strs.lower_bound(index);
    while (delete_iter != _unassemble_strs.end()) {
        if (delete_iter->first >= index && delete_iter->first + delete_iter->second.size() <= index + data.size()) {
            _unassembled_bytes_num -= delete_iter->second.size();
            delete_iter = _unassemble_strs.erase(delete_iter);
        } else
            break;
    }
    size_t new_index = index;
    string new_data(data);
    // 头部与已装载重叠，截断
    if (new_index < _next_assembled_idx) {
        // 如果完全已装载，丢弃
        if (new_index + new_data.size() <= _next_assembled_idx) {
            return;
        } else {
            new_data = new_data.substr(_next_assembled_idx - new_index);
            new_index = _next_assembled_idx;
        }
    }
    // 检测该串被覆盖的情况
    auto iter = _unassemble_strs.upper_bound(new_index);
    if (iter != _unassemble_strs.begin()) {
        iter--;
        if (iter->first <= new_index && iter->first + iter->second.size() >= new_index + new_data.size()) {
            return;
        }
    }
    // 头部与现有的strs重叠部分截断
    iter = _unassemble_strs.upper_bound(new_index);
    if (iter != _unassemble_strs.begin()) {
        iter--;
        if (iter->first < new_index && iter->first + iter->second.size() > new_index &&
            iter->first + iter->second.size() < new_index + new_data.size()) {
            new_data = new_data.substr(iter->first + iter->second.size() - new_index);
            new_index = iter->first + iter->second.size();
        }
    }
    // 尾部与现有的strs重叠部分截断
    iter = _unassemble_strs.upper_bound(new_index);
    if (iter != _unassemble_strs.end()) {
        if (iter->first < new_index + new_data.size() &&
            iter->first + iter->second.size() > new_index + new_data.size()) {
            new_data = new_data.substr(0, iter->first - new_index);
        }
    }
    // 添加字节数
    _unassembled_bytes_num += new_data.size();
    _unassemble_strs.insert(pair<size_t, string>{new_index, new_data});
    // 检查是否能装配，可以则直接装配
    for (auto it = _unassemble_strs.begin(); it != _unassemble_strs.end();) {
        if (it->first == _next_assembled_idx) {
            size_t byte_written = _output.write(it->second);
            _next_assembled_idx += byte_written;
            _unassembled_bytes_num -= byte_written;
            if (byte_written == it->second.size()) {
                it = _unassemble_strs.erase(it);
            } else {
                _unassemble_strs.insert(
                    pair<size_t, string>(_next_assembled_idx, it->second.substr(_next_assembled_idx - it->first)));
                it = _unassemble_strs.erase(it);
                break;
            }
        } else {
            break;
        }
    }
    // 结束标记
    if (eof) {
        _eof = true;
        _eof_idx = new_index + new_data.size();
    }
    // 结束
    if (_eof && _next_assembled_idx == _eof_idx) {
        _output.end_input();
    }

    // cout<<"000000"<<endl;
    // for (auto i = _unassemble_strs.begin(); i != _unassemble_strs.end(); i++) {
    //     cout << i->first << " ";
    //     for (long unsigned int j = 0; j < i->second.size(); j++) {
    //         cout << static_cast<int> ( i->second.at(j) );
    //     }
    //     cout<< endl;
    // }
}

size_t StreamReassembler::unassembled_bytes() const { return _unassembled_bytes_num; }

bool StreamReassembler::empty() const { return _unassembled_bytes_num == 0; }
