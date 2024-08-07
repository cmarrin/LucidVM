/*-------------------------------------------------------------------------
    This source file is a part of Lucid
    For the latest info, see https://github.com/cmarrin/LucidVM
    Copyright (c) 2021-2022, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#pragma once

#include <stdint.h>
#include <stdlib.h>
#include <memory>

namespace lucid {

#ifdef ARDUINO
    #include <Arduino.h>
    static inline String to_string(int32_t v) { return String(v); }
    static inline String to_string(float v) { return String(v); }
#else
    //#include <string>
    
    static inline void randomSeed(uint32_t s) { srand(s); }
    static inline int32_t random(int32_t min, int32_t max)
    {
        if (min >= max) {
            return max;
        }
        int r = rand() % (max - min);
        return r + min;
    }
    
    template<class T> 
    const T& min(const T& a, const T& b)
    {
        return (b < a) ? b : a;
    }

    template<class T> 
    const T& max(const T& a, const T& b)
    {
        return (b > a) ? b : a;
    }
    
    //using String = std::string;
    //static inline std::string to_string(int32_t v) { return std::to_string(v); }
    //static inline std::string to_string(float v) { return std::to_string(v); }
#endif

static inline float intToFloat(uint32_t i)
{
    float f;
    memcpy(&f, &i, sizeof(float));
    return f;
}

static inline uint32_t floatToInt(float f)
{
    uint32_t i;
    memcpy(&i, &f, sizeof(float));
    return i;
}

// PointerSize is needed to know how much space is taken
// by pointers on the stack and to compute function and
// struct variable sizes.
static constexpr uint8_t PointerSize = 2;

/*

Machine Code for LucidVM

See README.md for details.

Opcodes:
    Param nomenclature:
        
        int8    - Byte after opcode. Int constant (-128 to 127)
        uint8   - Byte after opcode. Int constant (0 to 255)
        int16   - 2 bytes after opcode. Int constant (-32768 to 32767)
        uint16  - 2 bytes after opcode. Int constant (0 to 65535)
        int32   - 4 bytes after opcode. Int constant (-2^31 to 2^31-1)
        uint32  - 4 bytes after opcode. Int constant (0 to 2^32-1)
        half    - 2 bytes after opcode. 16 bit fixed point number (8:8 - +/-127.99)
        float   - 4 bytes after opcode. Floating point number in Float class format
        str     - Byte after opcode is length, followed by length bytes
        
        ref     - Reference to a value on the stack or a constant.

    Stack is 8 bit. 16 and 32 bit values are stored in consecutive locations (HI to LO)
    
    Stack machine
    
    Example:
    
    int16 b = 10;
    int16 c = 20;
    int16 a = (b + 5) * (c + 6);
    
    PUSHREF b,bp
    PUSH2   #10
    DEREF2
    PUSHREF c,bp
    PUSH2   #20
    DEREF2
    PUSHREF a,BP
    PUSH2   b,BP
    PUSH2   #5
    ADD2
    PUSH2   c,BP
    PUSH2   #6
    ADD2
    IMUL2
    DEREF2
    
    - Values are 8, 16 or 32 bit
    - Has a base pointer (BP) register pointing to local vars and args
    - Has a stack pointer that grows down
    - Has a pc
    
    All addresses can be 2 bytes for a 64KB range or 4 bytes for a 2^32 byte range.
    This is determined at compile time. Also at compile time you can specify
    float support (16 or 32 bit) or not and max integer width (16 or 32 bits).
    
    - Address mode
        The byte following the opcode indicates the addressing mode. Bits1:0 indicate
        how the addressing value is used:
                        
            00 - Immediate. Value is the signed operand
            01 - X. Add signed value to X to get EA
            10 - Y. Add signed value to Y to get EA
            11 - U. Add signed value to U to get EA
                        
        if bit 2 is 0 then bits 7:3 are a signed offset from -16 to 16. If bit 2 is 1
        then bits 4:3 are the number of bytes that follow, from 1 (00) to 4 (11). If
        1 byte follows then bits 7:5 are appended as the MSB of a 13 bit signed integer
        from -1024 to 1023. If 2, 3 or 4 bytes follow no appending is done.

    Stack frame and Base pointer
    
    Stack grows down so when you push a value the current SP points to it. On a call
    args are pushed from left to right so rightmost param is at the lowest address.
    The caller then pushes the return address and calls the function.
    
    The first instruction of a function must be ENTER. This has a 4 bit (0-15), 1 or
    2 byte operand which is the number of bytes of local storage needed. The ENTER
    instruction pushes the current BP, sets BP = SP and subtracts the number of
    local bytes from SP. When indexing from BP, positive offsets starting at 4
    address the args. Locals are addressed with negative offsets starting at -1.
    
    On return, the callee sets SP = BP, pops the stack into BP and performs a
    return operation. The caller then adds the number of bytes of args to SP.
    
    PUSHREF         - push EA of value (must be X, Y, or BP addressing mode)
    DEREF<1,2,4>    - Value is on TOS, EA is TOS+1, store value at EA
    PUSH<1,2,4>     - Push value (immediate or indexed from X, y or BP)
    
    DUP<1,2,4>      - Duplicate TOS
    DROP<1,2,4>     - Pop TOS
    SWAP<1,2,4>     - Exchange TOS and TOS+1
    
    ADD<1,2,4,F>    - Add TOS to TOS+1, result on TOS
    SUB<1,2,4,F>    - Sub TOS+1 from TOS, result on TOS
    UMUL<1,2,4>     - Unsigned multiple TOS+1 and TOS, result on TOS
    IMUL<1,2,4,F>   - Signed multiple TOS+1 and TOS, result on TOS
    UDIV<1,2,4>     - Unsigned divide TOS+1 by TOS, result on TOS
    IMUL<1,2,4,F>   - Signed divide TOS+1 by TOS, result on TOS
    OR<1,2,4>       - Bitwise or TOS and TOS+1, result on TOS
    XOR<1,2,4>      - Bitwise xor TOS and TOS+1, result on TOS
    AND<1,2,4>      - Bitwise and TOS and TOS+1, result on TOS
    
    NOT<1,2,4>      - Bitwise inversion of TOS, result on TOS
    NEG<1,2,4,F>    - Negate TOS, result on TOS

    LE<1,2,4,F>     - Test if signed TOS+1 is less than or equal to signed TOS, bool on TOS
    LS<1,2,4,F>     - Test if unsigned TOS+1 is less than or equal to unsigned TOS, bool on TOS
    LT<1,2,4,F>     - Test if signed TOS+1 is less than signed TOS, bool on TOS
    LO<1,2,4,F>     - Test if unsigned TOS+1 is less than unsigned TOS, bool on TOS
    EQ<1,2,4,F>     - Test if TOS+1 is equal to TOS, bool on TOS
    NE<1,2,4,F>     - Test if TOS+1 is not equal to TOS, bool on TOS
    GT<1,2,4,F>     - Test if signed TOS+1 is greater than signed TOS, bool on TOS
    HI<1,2,4,F>     - Test if unsigned TOS+1 is greater than unsigned TOS, bool on TOS
    GE<1,2,4,F>     - Test if signed TOS+1 is greater than or equal to signed TOS, bool on TOS
    HS<1,2,4,F>     - Test if unsigned TOS+1 is greater than or equal to unsigned TOS, bool on TOS

    IF<S,L>         - IF TOS is true, branch to 8 or 16 bit relative address in follow byte(s)
    BRA<S,L>        - Jump to 8 or 16 bit relative address in follow byte(s)
    CALL<S,L>       - Call function at 8 or 16 bit relative address in follow byte(s), push PC
    CALLNATIVE      - Call native function with id in next byte
    ENTER<I,S,L>    - Do enter operations using local count in lower 4 bits of opcode, or
                      following 8 or 16 bits
    RET             - Do leave operations and pop PC
    
    PREINC<1,2,4,F> - Takes an index byte. Like PUSH, but increments the value at EA and the pushed value
    PREDEC<1,2,4,F> - Takes an index byte. Like PUSH, but decrements the value at EA and the pushed value
    POSTINC<1,2,4,F>- Takes an index byte. Like PUSH, but increments the value at EA and pushes the unincremented value
    POSTDEC<1,2,4,F>- Takes an index byte. Like PUSH, but decrements the value at EA and pushes the undecremented value
*/

static constexpr uint16_t MaxIdSize = 4096;
static constexpr uint16_t ConstStart = 0x00;
static constexpr uint16_t ConstSize = 2048; // Max possible size
static constexpr uint16_t GlobalStart = ConstStart + ConstSize;
static constexpr uint16_t GlobalSize = 1024;
static constexpr uint16_t LocalStart = GlobalStart + GlobalSize;
static constexpr uint16_t LocalSize = MaxIdSize - LocalStart;
static constexpr uint8_t ExtOpcodeStart = 0x40;

static constexpr uint8_t WidthOpcodesStart = 0x10;
static constexpr uint8_t WidthOpcodesEnd   = 0x8f;
static constexpr uint8_t OffsetStart       = 0x90;
static constexpr uint8_t OffsetEnd         = 0x9f;
static constexpr uint8_t EnterShortStart   = 0xf0;
static constexpr uint8_t EnterShortEnd     = 0xff;

enum class Op: uint8_t {
    NOP     = 0x00,
    PUSHREF = 0x01,

// Bits 1:0 is the width of the data: 00 - 1 byte, 01 - 2 bytes, 10 - 3 bytes, 11 float
    DEREF   = 0x10,
    PUSH    = 0x14,
    DUP     = 0x18,
    DROP    = 0x1c,
    SWAP    = 0x20,
    
    ADD     = 0x24,
    SUB     = 0x28,
    IMUL    = 0x2c,
    UMUL    = 0x30,
    IDIV    = 0x34,
    UDIV    = 0x38,
    
    AND     = 0x3c,
    OR      = 0x40,
    XOR     = 0x44,
    NOT     = 0x48,
    NEG     = 0x4c,

    PREINC  = 0x50,
    PREDEC  = 0x54,
    POSTINC = 0x58,
    POSTDEC = 0x5c,
    
    LE      = 0x60,
    LS      = 0x64,
    LT      = 0x68,
    LO      = 0x6c,
    GE      = 0x70,
    HS      = 0x74,
    GT      = 0x78,
    HI      = 0x7c,
    EQ      = 0x80,
    NE      = 0x84,
    
// LSB = 0, following param is 8 bit. LSB = 1, following param is 16 bit
    IF      = 0x90,
    BRA     = 0x92,
    CALL    = 0x94,
    ENTER   = 0x96,
    RET     = 0x98,

// These versions use the lower 4 bits of the opcode as a param (0-15)
    RETS    = 0xe0,
    ENTERS  = 0xf0,
};

enum class OpParams : uint8_t {
    None,       // No params
    Id,         // b+1 = <id>
    I,          // b+1[3:0] = <int> (0-3)
    Index,      // b[3:0] = <int> (0-15)
    Const,      // b+1 = 0-255
    AbsTarg,    // Lower 4 bits of opcode (bits 11:8) | byte after opcode
                // (bits 7:0). 12 bit absolute address (0 to 4095).
    RelTarg,    // Lower 4 bits of opcode (bits 11:8) | byte after opcode
                // (bits 7:0). 12 bit relative address (-2048 to 2047).
    P_L,        // b[3:0] = num params (0-15), b+1 = num locals (0-255)
    Idx_Len_S, // b[3:0] = <int> (0-15), b+1 = <int>, followed by Sz string bytes
};

// Built-in types are 0x00-StructTypeStart-1, custom types are StructTypeStart-0xff
enum class Type : uint8_t {
    None = 0,
    Float = 1,
    Fixed = 2,
    
    Int8 = 10,
    UInt8 = 11,
    Int16 = 12,
    UInt16 = 13,
    Int32 = 14,
    UInt32 = 15,
    Int = 16, // Unspecified size
    UInt = 17, // Unspecified size
    String = 18,
    Function = 19,
    
    Struct = 20,
    
    Ptr = 30
};

enum class Index : uint8_t { K = 0x00, X = 0x01, Y = 0x02, U = 0x03 };

enum class Reserved {
    None,
    Struct,
    Const,
    Import,
    As,
    Function,
    Initialize,
    Return,
    Break,
    Continue,
    End,
    Loop,
    While,
    For,
    If,
    Else,
    Float,
    Fixed,
    Int8,
    UInt8,
    Int16,
    UInt16,
    Int32,
    UInt32,
};

// These must match the values for Token
enum class Operator : uint8_t {
    Equal   = '=',
    AddSto  = 0xa0,
    SubSto  = 0xa1,
    MulSto  = 0xa2,
    DivSto  = 0xa3,
    ModSto  = 0xa4,
    AndSto  = 0xa5,
    OrSto   = 0xa6,
    XorSto  = 0xa7,
    LOr     = 0xa8,
    LAnd    = 0xa9,
    Or      = '|',
    Xor     = '^',
    And     = '&',
    EQ      = 0xab,
    NE      = 0xac,
    LT      = '<',
    GT      = '>',
    GE      = 0xad,
    LE      = 0xaa,
    Plus    = '+',
    Minus   = '-',
    Mul     = '*',
    Div     = '/',
    Mod     = '%',
    Inc     = 0xae,
    Dec     = 0xaf,
    BNot    = '~',
    LNot    = '!',
    ArrIdx  = '[',
    Dot     = '.',
    
    // '&', '*' and '-' are used as unary operators. Represent those here.
    AddrOf  = 0xe0,
    Deref   = 0xe1,
    UMinus  = 0xe2,
};

// Native functions
//
// These are implemented in the ExecutionUnit and are recognized by the Compiler.
// CallNative op has an operand which is the Native enum value. All params are
// passed on the stack and must be the expected size and type. Return value is
// sent back as an int32_t but can represent any type of value as defined in the
// native function signature
//
// Functions:
//
//      void    print(string)       - prints the passed string to the console
//      string  int8ToString(int8)  - return passed int8 value converted to string

enum class NativeId : uint8_t {
    None            = 0,
    Print           = 1,
    Int8ToString    = 2,
    UInt8ToString   = 3,
    Int16ToString   = 4,
    UInt16ToString  = 5,
    Int32ToString   = 6,
    UInt32ToString  = 7,
    FloatToString   = 8,
};

class ASTNode;
using ASTPtr = std::shared_ptr<ASTNode>;

class Symbol;
using SymbolPtr = std::shared_ptr<Symbol>;
using SymbolList = std::vector<Symbol>;
}
