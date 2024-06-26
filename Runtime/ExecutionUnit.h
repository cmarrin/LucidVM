/*-------------------------------------------------------------------------
    This source file is a part of Clover
    For the latest info, see https://github.com/cmarrin/Clover
    Copyright (c) 2021-2022, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

// Interpreter base class
//
#pragma once

#include "Opcodes.h"

#include <stdlib.h>
#include <string.h>

namespace lucid {

static constexpr uint8_t MaxStackSize = 128;    // Could be 255 but let's avoid excessive 
                                                // memory usage
static constexpr uint8_t StackOverhead = 64;    // Amount added to var mem high water mark
                                                // for stack growth.
static constexpr uint8_t MaxTempSize = 32;      // Allocator uses a uint32_t map. That would 
                                                // need to be changed to increase this.
static constexpr uint8_t ParamsSize = 16;       // Constrained by the 4 bit field with the index
static constexpr uint16_t ConstOffset = 10;

//
// Core Native Functions
//

class ExecutionUnit;

class CompileEngine;

class ExecutionUnit
{
public:
    enum class Error {
        None,
        CmdNotFound,
        UnexpectedOpInIf,
		InvalidOp,
        OnlyMemAddressesAllowed,
        AddressOutOfRange,
        ExpectedSetFrame,
        InvalidModuleOp,
        InvalidNativeFunction,
        NotEnoughArgs,
        WrongNumberOfArgs,
        StackOverrun,
        StackUnderrun,
        StackOutOfRange,
    };

    ExecutionUnit(NativeModule** mod = nullptr, uint32_t modSize = 0);
    ~ExecutionUnit();
    
    bool init(const char* cmd, const uint8_t* buf, uint8_t size);
    int32_t loop();

    Error error() const { return _error; }
    
    // Returns -1 if error was not at any pc addr
    int16_t errorAddr() const { return _errorAddr; }
    
	// Return a float with a random number between min and max.
	// The random function takes ints. Multiply inputs by 1000 then divide the result
    // by the same to get a floating point result. That effectively makes the range
    // +/-2,000,000.
	static float random(float min, float max)
	{
		return float(::random(int32_t(min * 1000), int32_t(max * 1000))) / 1000;
	}
	
	static int32_t random(int32_t min, int32_t max)
	{
		int32_t r = ::random(min, max);
		return r;
	}

    uint32_t stackLocal(uint16_t addr) const { return _stack.local(addr); }

    int32_t animate(uint32_t index);
    uint8_t param(uint32_t i) const { return (i >= ParamsSize) ? 0 : _params[i]; }
    void initArray(uint32_t addr, uint32_t value, uint32_t count);

    void setError(Error error) { _error = error; }

    virtual uint8_t rom(uint16_t i) const = 0;
    virtual void log(const char* s) const = 0;

private:
    // Address:
    //
    // opcodes with ids have 8 bit addresses. If an address is < GlobalStart
    // it is a constant. If it is < LocalStart it is a global address. Otherwise
    // it's a local address. There are two types of local addresses. Those
    // in the opcode are on the stack and relative to the current bp. But when
    // a local address is pushed on the stack (as a 32 bit value) it is
    // "baked" into an absolute stack address so it can be passed as a param.
    // This Address class encapsulates all this. It is an enum with the address
    // type and an 8 bit value that is the actual offset in the area of memory
    // described by the address type. Unlike the id, this offset of always
    // zero based.
    //
    class Address
    {
    public:
        enum class Type : uint8_t { None, Const, Global, LocalRel, LocalAbs };

        Address() { }

        static Address fromId(uint16_t id)
        {
            Address addr;
            if (id < GlobalStart) {
                addr._type = Type::Const;
                addr._addr = id;
            } else if (id < LocalStart) {
                addr._type = Type::Global;
                addr._addr = id & 0x3f;
            } else {
                addr._type = Type::LocalRel;
                addr._addr = id & 0x3f;
            }
            return addr;
        }
        
        static Address fromVar(uint32_t v)
        {
            Address addr;
            addr._type = Type(v >> 8);
            addr._addr = uint8_t(v);
            return addr;
        }
        
        static Address fromLocalAbs(uint8_t a)
        {
            Address addr;
            addr._type = Type::LocalAbs;
            addr._addr = a;
            return addr;
        }

        uint32_t toVar() { return (uint32_t(_type) << 8) | _addr; }

        uint8_t addr() const { return _addr; }
        Type type() const { return _type; }
        
    private:
        Type _type = Type::None;
        uint8_t _addr = 0;
    };

    class Stack
    {
    public:
        void alloc(uint16_t size)
        {
            if (_stack) {
                delete [ ] _stack;
            }
            _size = size;
            _stack = new uint32_t [size];
            
            _sp = 0;
            _bp = 0;
            _error = Error::None;
        }
            
        void push(uint32_t v) { ensurePush(); _stack[_sp++] = v; }
        void push(Address addr) { ensurePush(); _stack[_sp++] = addr.toVar(); }
        uint32_t pop(uint8_t n = 1) { ensureCount(n); _sp -= n; return _stack[_sp]; }
        Address popAddr(uint8_t n = 1) { ensureCount(n); _sp -= n; return Address::fromVar(_stack[_sp]); }
        void swap()
        {
            ensureCount(2); 
            uint32_t t = _stack[_sp - 1];
            _stack[_sp - 1] = _stack[_sp - 2];
            _stack[_sp - 2] = t;
        }
        
        const uint32_t& top(uint8_t rel = 0) const { ensureRel(rel); return _stack[_sp - rel - 1]; }
        uint32_t& top(uint8_t rel = 0) { ensureRel(rel); return _stack[_sp - rel - 1]; }
        const uint32_t& local(uint16_t addr) const { ensureLocal(addr); return get(addr + _bp); }
        uint32_t& local(uint16_t addr) { ensureLocal(addr); return get(addr + _bp); }
        const uint32_t& absolute(uint16_t addr) const { ensureCount(addr); return get(addr); }
        uint32_t& absolute(uint16_t addr) { ensureCount(addr); return get(addr); }

        Address toAbsAddress(uint16_t id) const
        {
            Address addr = Address::fromId(id);
            if (addr.type() == Address::Type::LocalRel) {
                addr = Address::fromLocalAbs(addr.addr() + _bp);
            }
            return addr;
        }

        bool empty() const { return _sp == 0; }
        Error error() const { return _error; }
                
        bool setFrame(uint8_t params, uint8_t locals)
        {
            int16_t savedPC = pop();
            _sp += locals;
            push(savedPC);
            push(_bp);
            int16_t newBP = _sp - params - locals - 2;
            if (newBP < 0) {
                _error = Error::NotEnoughArgs;
                return false;
            }
            
            if (newBP > _sp) {
                _error = Error::StackOutOfRange;
                return false;
            }
            _bp = newBP;
            return true;
        }

        int16_t restoreFrame(uint32_t returnValue)
        {
            int16_t savedBP = pop();
            int16_t pc = pop();
            _sp = _bp;

            if (savedBP > _sp) {
                _error = Error::StackOutOfRange;
                return false;
            }
            _bp = savedBP;
            push(returnValue);
            return pc;
        }

    private:
        void ensureCount(uint8_t n) const
        {
            if (_sp < n) {
                // Set the error, but let the exec catch it
                _error = Error::StackUnderrun;
            }
        }
        
        void ensureRel(uint8_t rel) const
        {
            int16_t addr = _sp - rel - 1;
            if (addr < 0 || addr >= int16_t(_size)) {
                // Set the error, but let the exec catch it
                _error = Error::StackOutOfRange;
            }
        }
        
        void ensureLocal(uint8_t i) const
        {
            int16_t addr = _bp + i;
            if (addr < 0 || addr >= _sp) {
                // Set the error, but let the exec catch it
                _error = Error::StackOutOfRange;
            }
        }
        
        void ensurePush() const
        {
            if (_sp >= _size) {
                // Set the error, but let the exec catch it
                _error = Error::StackOverrun;
            }
        }
        
        uint32_t& get(uint16_t addr) const
        {
            if (int16_t(addr) >= _size) {
                // Set the error, but let the exec catch it
                _error = Error::StackOverrun;
            }
            return _stack[addr];
        }
        
        uint32_t* _stack = nullptr;
        int16_t _size = 0;
        int16_t _sp = 0;
        int16_t _bp = 0;
        mutable Error _error = Error::None;
    };

    int32_t execute(uint16_t addr);
    
    // Index is in bytes
    uint8_t getUInt8ROM(uint16_t index) const
    {
        return rom(index);
    }
    
    uint16_t getUInt16ROM(uint16_t index) const
    {
        // Little endian
        return uint32_t(getUInt8ROM(index)) | 
               (uint32_t(getUInt8ROM(index + 1)) << 8);
    }

    uint16_t getId(uint8_t i)
    {
        return uint16_t(getUInt8ROM(_pc++)) | (uint16_t(i) << 8);
    }
    
    uint16_t getAbsTarg(uint8_t i) { return getId(i); }
    
    int16_t getRelTarg(uint8_t i)
    {
        uint16_t targ = getId(i);
        if (targ & 0x800) {
            targ |= 0xf000;
        }
        return int16_t(targ);
    }
    
    uint8_t getConst() { return getUInt8ROM(_pc++); }
    uint8_t getSz() { return getUInt8ROM(_pc++); }

    uint8_t getI()
    {
        uint8_t b = getUInt8ROM(_pc++);
        return b & 0x0f;
    }
    
    float loadFloat(Address addr, uint8_t index = 0)
    {
        uint32_t i = loadInt(addr, index);
        float f;
        memcpy(&f, &i, sizeof(float));
        return f;
    }
    
    uint32_t loadInt(Address addr, uint8_t index = 0)
    {
        switch(addr.type()) {
            case Address::Type::Const: {
                uint16_t a = ((addr.addr() + index) * 4) + ConstOffset;

                // Little endian
                uint32_t u = uint32_t(getUInt8ROM(a)) |
                            (uint32_t(getUInt8ROM(a + 1)) << 8) |
                            (uint32_t(getUInt8ROM(a + 2)) << 16) |
                            (uint32_t(getUInt8ROM(a + 3)) << 24);

                return u;
            }
            case Address::Type::Global:
                return _global[addr.addr() + index];
            case Address::Type::LocalRel:
                return _stack.local(addr.addr() + index);
            case Address::Type::LocalAbs:
                return _stack.absolute(addr.addr() + index);
            default:
                _error = Error::AddressOutOfRange;
                return 0;
        }
    }
        
    void storeFloat(Address addr, float v) { storeFloat(addr, 0, v); }
    
    void storeFloat(Address addr, uint8_t index, float v) { storeInt(addr, index, floatToInt(v)); }
    
    void storeInt(Address addr, uint32_t v) { storeInt(addr, 0, v); }
    
    void storeInt(Address addr, uint8_t index, uint32_t v)
    {
        switch(addr.type()) {
            case Address::Type::Const: {
                // Only Global or Local
                return;
            }
            case Address::Type::Global: {
                uint32_t a = uint32_t(addr.addr()) + uint32_t(index);
                _global[a] = v;
                return;
            }
            case Address::Type::LocalRel: {
                uint32_t a = uint32_t(addr.addr()) + uint32_t(index);
                _stack.local(a) = v;
                return;
            }
            case Address::Type::LocalAbs: {
                uint32_t a = uint32_t(addr.addr()) + uint32_t(index);
                _stack.absolute(a) = v;
                return;
            }
            default:
                _error = Error::AddressOutOfRange;
                return;
        }
    }
    
    bool log(const char* fmt, uint8_t numArgs);
    
    bool isNextOpcodeSetFrame() const
    {
        return Op(getUInt8ROM(_pc) & 0xf0) == Op::SetFrameS;
    }
    
    Error _error = Error::None;
    int16_t _errorAddr = -1;
    
    uint8_t _params[ParamsSize];
    uint8_t _paramsSize = 0;

    uint32_t* _global = nullptr;
    uint16_t _globalSize = 0;
    
    int16_t _pc = 0;
    Stack _stack;
    
    NativeModule** _nativeModules = nullptr;
    uint8_t _nativeModulesSize = 0;
    
    uint8_t _numParams = 0;
    uint16_t _initStart = 0;
    uint16_t _loopStart = 0;
    uint16_t _codeOffset = 0; // Used by Call to go to the correct absolute address
    
    char* _stringBuf = nullptr;
    uint8_t _stringSize = 0;
};

}
