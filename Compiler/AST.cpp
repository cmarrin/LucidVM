/*-------------------------------------------------------------------------
    This source file is a part of Lucid
    For the latest info, see https://github.com/cmarrin/Lucid
    Copyright (c) 2021-2024, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#include <map>

#include "AST.h"

using namespace lucid;

std::map<Type, uint8_t> typeToSizeBits {
    { Type::Int8,   0x00 },
    { Type::UInt8,  0x00 },
    { Type::Int16,  0x01 },
    { Type::UInt16, 0x01 },
    { Type::Int32,  0x02 },
    { Type::UInt32, 0x02 },
    { Type::Float,  0x03 },
};

// Operator map. Maps Operators (contained in the AST OpNode) to opcodes
std::map<Operator, std::pair<Op, Op>> opMap {
    { Operator::Equal   , { Op::NOP   , Op::NOP    } }, // OpNode knows if this is an assignment.
    { Operator::AddSto  , { Op::ADD   , Op::ADD    } }, // so all the ...Sto operators (including
    { Operator::SubSto  , { Op::SUB   , Op::SUB    } }, // Equal) just need the binary operation
    { Operator::MulSto  , { Op::IMUL  , Op::UMUL   } }, // they need to perform. For equal there
    { Operator::DivSto  , { Op::IDIV  , Op::UDIV   } }, // is no additional operation, so it is
    { Operator::AndSto  , { Op::AND   , Op::AND    } }, //
    { Operator::OrSto   , { Op::OR    , Op::OR     } }, //
    { Operator::XorSto  , { Op::XOR   , Op::XOR    } }, //
    { Operator::LOr     , { Op::NOP   , Op::NOP    } }, // Logical AND, OR and NOT don't have opcodes.
    { Operator::LAnd    , { Op::NOP   , Op::NOP    } }, // They are short-circuited at compile time.
    { Operator::LNot    , { Op::NOP   , Op::NOP    } },
    { Operator::Or      , { Op::OR    , Op::XOR    } },
    { Operator::Xor     , { Op::XOR   , Op::XOR    } },
    { Operator::And     , { Op::AND   , Op::AND    } },
    { Operator::EQ      , { Op::EQ    , Op::EQ     } },
    { Operator::NE      , { Op::NE    , Op::NE     } },
    { Operator::LT      , { Op::LT    , Op::LO     } },
    { Operator::GT      , { Op::GT    , Op::HI     } },
    { Operator::GE      , { Op::GE    , Op::HS     } },
    { Operator::LE      , { Op::LE    , Op::LS     } },
    { Operator::Plus    , { Op::ADD   , Op::ADD    } },
    { Operator::Minus   , { Op::SUB   , Op::SUB    } },
    { Operator::Mul     , { Op::IMUL  , Op::UMUL   } },
    { Operator::Div     , { Op::IDIV  , Op::UDIV   } },
    { Operator::Inc     , { Op::NOP   , Op::NOP    } },
    { Operator::Dec     , { Op::NOP   , Op::NOP    } },
    { Operator::BNot    , { Op::NOT   , Op::NOT    } },
    { Operator::ArrIdx  , { Op::NOP   , Op::NOP    } },
    { Operator::Dot     , { Op::NOP   , Op::NOP    } },
    { Operator::AddrOf  , { Op::NOP   , Op::NOP    } },
    { Operator::Deref   , { Op::NOP   , Op::NOP    } },
    { Operator::UMinus  , { Op::NEG   , Op::NEG    } },
};

// This version is for opcodes where the lower 2 bits hold type info and the following
// bytes hold values or addresses
static void emitCode(std::vector<uint8_t>& code, Op opcode, Type type, Index index, int32_t value)
{
    code.push_back(uint8_t(opcode) | typeToSizeBits[type]);
    
    uint8_t extra = uint8_t(index);
    uint8_t addedBytes = 0;
    
    if (value >= -16 && value <= 15) {
        extra |= uint8_t(value << 3);
    } else if (value >= -2048 && value <= 2047) {
        extra |= uint8_t(value << 5);
        addedBytes = 1;
    } else if (value >= -32768 && value <= 32767) {
        extra |= 0x08;
        addedBytes = 2;
    } else if (value >= -8388608 && value <= 8388607) {
        extra |= 0x10;
        addedBytes = 3;
    } else {
        extra |= 0x18;
        addedBytes = 4;
    }
    
    code.push_back(extra);
    if (addedBytes > 3) {
        code.push_back(uint8_t(value >> 24));
    }
    if (addedBytes > 2) {
        code.push_back(uint8_t(value >> 16));
    }
    if (addedBytes > 1) {
        code.push_back(uint8_t(value >> 8));
    }
    if (addedBytes > 0) {
        code.push_back(uint8_t(value));
    }
}

// This version is for opcodes where the LSB indicates 8 or 16 bit values that follow
static void emitCode(std::vector<uint8_t>& code, Op opcode, int32_t value)
{
    if (value <= 127 && value >= -128) {
        // 8 bit case
        code.push_back(uint8_t(opcode));
        code.push_back(value);
    } else {
        // 16 bit case
        code.push_back(uint8_t(opcode) | 0x01);
        code.push_back(value >> 8);
        code.push_back(value);
    }
}

// This version is for opcodes where the value is in the lower 4 bits
static void emitCodeShort(std::vector<uint8_t>& code, Op opcode, int32_t value)
{
    code.push_back(uint8_t(opcode) | (value & 0x0f));
}

void VarNode::addCode(std::vector<uint8_t>& code, bool isLHS) const
{
    // If isLHS is true, code generated will be a Ref
    Op op = isLHS ? Op::PUSHREF : Op::PUSH;
    
    // FIXME: We assume we're indexing from U, is this always true? What about using Y as the 'this' ptr?
    // FIXME: Right now _symbol->addr() is a uint16_t. We need positive addrs for args and negative
    // addrs for locals.
    emitCode(code, op, _symbol->type(), Index::U, _symbol->addr());
}

void ConstantNode::addCode(std::vector<uint8_t>& code, bool isLHS) const
{
    assert(!isLHS);
    
    // FIXME: What about Type::Int and Type::Float? These are generic int and float literals
    // and we should auto convert them to the right type?
    assert(_type != Type::None);
        
    emitCode(code, Op::PUSH, _type, Index::K, _i);
}

void StringNode::addCode(std::vector<uint8_t>& code, bool isLHS) const
{
    code.push_back(uint8_t(Op::PUSHS));
    code.push_back(_string.size());
    for (const char& c : _string) {
        code.push_back(c);
    }
}

void OpNode::addCode(std::vector<uint8_t>& code, bool isLHS) const
{
    code.push_back(uint8_t(_op));
}

void DotNode::addCode(std::vector<uint8_t>& code, bool isLHS) const
{
    // FIXME: Implement
}

void FunctionCallNode::addCode(std::vector<uint8_t>& code, bool isLHS) const
{
    // Add a function call. args will already be pushed
    int32_t addr = _function->addr();
    if (_function->isNative()) {
        if (addr <= 15) {
            code.push_back(uint8_t(Op::NCALLS) | addr);
        } else {
            if (addr <= 255) {
                code.push_back(uint8_t(Op::NCALL));
                code.push_back(addr);
            } else {
                code.push_back(uint8_t(Op::NCALL) | 0x01);
                code.push_back(addr >> 8);
                code.push_back(addr);
            }
        }
    } else {
        // FIXME: How does addr get set for a function call? 2 pass? Fixup?
        // FIXME: Implement CALL
    }
}

