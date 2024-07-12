/*-------------------------------------------------------------------------
    This source file is a part of Lucid
    For the latest info, see https://github.com/cmarrin/Lucid
    Copyright (c) 2021-2024, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

// Interpreter base class
//
#pragma once

namespace lucid {

// Stack grows down from the top. Heap grows up from the bottom
class Memory
{
  public:
    enum class Error {
        None,
        InternalError,
        StackOverrun,
        StackUnderrun,
        StackOutOfRange,
    };
    
    class Stack
    {
      public:
        Stack(uint8_t* mem, uint32_t size) : _mem(mem), _size(size), _sp(size) { }
        
        void setHeapEnd(int32_t addr) { _heapEnd = addr; }
        
        const uint8_t& abs(uint32_t addr) const { return _mem[addr]; }
        uint8_t& abs(uint32_t addr) { return _mem[addr]; }

        void push(uint32_t v, uint8_t count)
        {
            assert(count < 4);
            switch (count) {
                case 3:
                case 2: push(uint8_t(v >> 24));
                        push(uint8_t(v >> 16));
                case 1: push(uint8_t(v >> 8));
                case 0: push(uint8_t(v));
            }
        }
                
        uint32_t pop(uint8_t count)
        {
            assert(count < 4);
            
            uint32_t v = 0;
            
            switch (count) {
                case 3:
                case 2: v |= uint32_t(pop()) << 24;
                        v |= uint32_t(pop()) << 16;
                case 1: v |= uint32_t(pop()) << 8;
                case 0: v |= uint32_t(pop());
            }
            return v;
        }
        
        void drop(uint8_t n) { ensureCount(n); _sp += n; }
        
        void swap(uint8_t count)
        {
            assert(count < 4);
            
            uint32_t a = pop(count);
            uint32_t b = pop(count);
            push(a, count);
            push(b, count);
        }
        
        void dup(uint8_t count)
        {
            uint32_t v = pop(count);
            push(v, count);
            push(v, count);
        }

        bool empty() const { return _sp == _size; }
        Memory::Error error() const { return _error; }
        
        const uint16_t& sp() const { return _sp; }
        uint16_t& sp() { return _sp; }
                
        void ensurePush(uint8_t n = 1) const
        {
            if (_sp >= _size || _sp < 0) {
                _error = Error::InternalError;
            }
            if (_sp - n < _heapEnd) {
                _error = Error::StackOverrun;
            }
        }
        
      private:
        void push(uint8_t v) { ensurePush(1); _mem[--_sp] = v; }
        uint8_t pop() { ensureCount(1); return _mem[_sp++]; }

        static uint32_t floatToInt(float f) { return *(reinterpret_cast<uint32_t*>(&f)); }
        
        void ensureCount(uint8_t n) const
        {
            if (_sp < 0) {
                _error = Error::InternalError;
            }
            if (_sp + n > _size) {
                _error = Error::StackUnderrun;
            }
        }
        
        uint8_t* _mem = nullptr;
        int32_t _size = 0;
        uint16_t _sp = 0;
        uint16_t _heapEnd = 0;
        mutable Error _error = Error::None;
    };

    Memory(uint8_t* mem, uint32_t memSize) : _stack(mem, memSize) { }
    
    Error error() const { return _stack.error(); }
    const Stack& stack() const { return _stack; }
    Stack& stack() { return _stack; }
    void setAbs(uint32_t addr, uint32_t v, uint8_t count)
    {
        assert(count < 4);
        switch (count) {
            case 3:
            case 2: _stack.abs(addr++) = uint8_t(v >> 24);
                    _stack.abs(addr++) = uint8_t(v >> 16);
            case 1: _stack.abs(addr++) = uint8_t(v >> 8);
            case 0: _stack.abs(addr++) = uint8_t(v);
        }
    }
    
    uint32_t getAbs(uint32_t addr, uint8_t count)
    {
        assert(count < 4);
        
        uint32_t v = 0;
        
        switch (count) {
            case 3:
            case 2: v |= uint32_t(_stack.abs(addr++)) >> 24;
                    v |= uint32_t(_stack.abs(addr++)) >> 16;
            case 1: v |= uint32_t(_stack.abs(addr++)) >> 8;
            case 0: v |= uint32_t(_stack.abs(addr++));
        }
        
        return v;
    }
    
  private:
    Stack _stack;
};

}
