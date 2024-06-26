/*-------------------------------------------------------------------------
    This source file is a part of Clover
    For the latest info, see https://github.com/cmarrin/Clover
    Copyright (c) 2021-2022, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#include "Decompiler.h"

#include "CompileEngine.h"

using namespace lucid;

bool Decompiler::decompile()
{
    // Output everything before the first addr
    _annotationIndex = 0;
    for ( ; _annotationIndex < _annotations.size(); ++_annotationIndex) {
        if (_annotations[_annotationIndex].first != -1) {
            break;
        }
        
        _out->append("//    ");
        _out->append(_annotations[_annotationIndex].second);
    }
    
    try {
        // Make sure we start with 'lucd'
        if (getUInt8() != 'l' || getUInt8() != 'u' || getUInt8() != 'c' || getUInt8() != 'd') {
            _error = Error::InvalidSignature;
            return false;
        }
    
        constants();
        commands();
    }
    catch(...) {
        return false;
    }
    
    return true;
}

void
Decompiler::constants()
{
    doIndent();
    incIndent();
    _out->append("const\n");
    
    uint8_t size = getUInt16();
    _it += 4;
    
    if (size == 0) {
        return;
    }
        
    doIndent();
    incIndent();
    _out->append("const\n");
    
    for (uint8_t i = 0; i < size; ++i) {
        doIndent();
        _out->append("[");
        _out->append(std::to_string(i));
        _out->append("] = ");
        _out->append(std::to_string(getUInt32()));
        _out->append("\n");
    }
    
    _out->append("\n");
    decIndent();

}

void
Decompiler::commands()
{
    struct Entry
    {
        Entry(const std::string& cmd, uint8_t params, uint16_t init, uint16_t loop)
            : _cmd(cmd)
            , _params(params)
            , _init(init)
            , _loop(loop)
        { }
        
        std::string _cmd;
        uint8_t _params;
        uint16_t _init, _loop;
    };
    
    std::vector<Entry> entries;

    // Accumulate all Command entries
    while(1) {
        std::string cmd;
        cmd += getUInt8();
        if (cmd[0] == '\0') {
            break;
        }

        for (int i = 1; i < 7; ++i) {
            cmd += getUInt8();
        }
        entries.emplace_back(cmd, getUInt8(), getUInt16(), getUInt16());
    }

    // Save start of code address for Call
    _codeOffset = _it - _in->begin();
    
    // Output the function code
    _out->append("functions\n");
    incIndent();

    while(_it != _in->end()) {
        statement();
    }
    _out->append("\n");

    for (auto& entry : entries) {
        doIndent();
        incIndent();
        _out->append("command \"");
        _out->append(entry._cmd);
        _out->append("\" ");
        _out->append(std::to_string(entry._params));
        _out->append(" ");
        _out->append(std::to_string(entry._init + _codeOffset));
        _out->append(" ");
        _out->append(std::to_string(entry._loop + _codeOffset));
        _out->append("\n");
        decIndent();
    }
}

std::string
Decompiler::regString(uint8_t r)
{
    switch(r) {
        case 0: return "r0";
        case 1: return "r1";
        case 2: return "r2";
        case 3: return "r3";
        default: return "*** ERR ***";
    }
}

std::string
Decompiler::colorString(uint8_t r)
{
    switch(r) {
        case 0: return "c0";
        case 1: return "c1";
        case 2: return "c2";
        case 3: return "c3";
        default: return "*** ERR ***";
    }
}

void
Decompiler::statement()
{
    uint16_t a = addr() - _codeOffset;
    if (!_annotations.empty() && (_annotations[_annotationIndex].first == -1 || _annotations[_annotationIndex].first < a)) {
        for ( ; _annotationIndex < _annotations.size(); ) {
            _out->append("//    ");
            _out->append(_annotations[_annotationIndex++].second);
            if (_annotations[_annotationIndex].first != -1) {
                break;
            }
        }
    }
    
    uint8_t opInt = getUInt8();

    uint8_t index = 0;
    
    if (opInt >= ExtOpcodeStart) {
        // Get index from lowest 4 bits
        index = opInt & 0x0f;
        opInt &= 0xf0;
    }
    
//    OpData opData;
//    if (!CompileEngine::opDataFromOp(Op(opInt), opData)) {
//        _error = Error::InvalidOp;
//        throw true;
//    }
//
//    // Add blank like before if
//    if (opData._op == Op::If) {
//        _out->append("\n");
//    }
//
//    doIndent();
//
//    outputAddr();
//    _out->append(opData._str);
//    _out->append(" ");
//    
//    // Get params
//    uint8_t id;
//    uint8_t rdrsi;
//    
//    switch(opData._par) {
//        case OpParams::None:
//            break;
//        case OpParams::Id:
//            _out->append("[");
//            _out->append(std::to_string((uint16_t(index) << 8) | uint16_t(getUInt8())));
//            _out->append("]");
//            break;
//        case OpParams::I:
//            rdrsi = getUInt8();
//            _out->append(std::to_string(rdrsi & 0x0f));
//            break;
//        case OpParams::Index:
//            _out->append(std::to_string(index));
//            break;
//        case OpParams::Const:
//            _out->append(std::to_string(getUInt8()));
//            break;
//        case OpParams::AbsTarg: {
//            uint16_t targ = (uint16_t(index) << 8) | uint16_t(getUInt8());
//            _out->append("[");
//            _out->append(std::to_string(targ + _codeOffset));
//            _out->append("]");
//            break;
//        }
//        case OpParams::RelTarg: {
//            int16_t targ = (int16_t(index) << 8) | int16_t(getUInt8());
//            if (targ & 0x800) {
//                targ |= 0xf000;
//            }
//            _out->append("[");
//            _out->append(std::to_string(targ));
//            _out->append("]");
//            break;
//        }
//        case OpParams::P_L:
//            id = getUInt8();
//            _out->append(std::to_string(index));
//            _out->append(" ");
//            _out->append(std::to_string(id));
//            break;
//        case OpParams::Idx_Len_S: {
//            _out->append(std::to_string(index));
//            _out->append(" \"");
//            
//            uint8_t len = getUInt8();
//            while(len-- > 0) {
//                uint8_t c = getUInt8();
//                if (c >= 0x20) {
//                    (*_out) += char(c);
//                } else {
//                    (*_out) += '\\';
//                    if (c == '\n') {
//                        (*_out) += 'n';
//                    } else {
//                        (*_out) += 'x';
//                        uint8_t d = c >> 4;
//                        (*_out) += char((d > 9) ? (d + 'a' - 10) : (d + '0'));
//                        d = c & 0x0f;
//                        (*_out) += char((d > 9) ? (d + 'a' - 10) : (d + '0'));
//                    }
//                }
//            }
//            _out->append("\"");
//            break;
//        }
//    }
    
    _out->append("\n");
}
