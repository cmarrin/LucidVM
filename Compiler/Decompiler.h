/*-------------------------------------------------------------------------
    This source file is a part of Clover
    For the latest info, see https://github.com/cmarrin/Clover
    Copyright (c) 2021-2022, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

// Decompile executable into source string
//

#pragma once

#include "Compiler.h"

namespace lucid {

class Decompiler
{
public:
    enum class Error {
        None,
        InvalidSignature,
        InvalidOp,
        PrematureEOF,
    };
    
    Decompiler(const std::vector<uint8_t>* in, std::string* out, const AnnotationList& annotations)
        : _in(in)
        , _out(out)
        , _annotations(annotations)
    {
        _it = _in->begin();
    }
    
    bool decompile();
    
    Error error() const { return _error; }

private:
    void constants();
    void commands();
    void statement();
    
    uint32_t getUInt32()
    {
        if (_in->end() - _it < 4) {
            _error = Error::PrematureEOF;
            throw true;
        }
        
        // Little endian
        return uint32_t(*_it++) | (uint32_t(*_it++) << 8) | 
                (uint32_t(*_it++) << 16) | (uint32_t(*_it++) << 24);
    }
    
    uint16_t getUInt16()
    {
        if (_in->end() - _it < 2) {
            _error = Error::PrematureEOF;
            throw true;
        }
        
        // Little endian
        return uint32_t(*_it++) | (uint32_t(*_it++) << 8);
    }
    
    uint16_t getUInt8()
    {
        if (_in->end() - _it < 1) {
            _error = Error::PrematureEOF;
            throw true;
        }
        

        return *_it++;
    }
    
    std::string regString(uint8_t r);
    std::string colorString(uint8_t c);
    
    void doIndent()
    {
        for (uint32_t i = 0; i < _indent; ++i) {
            _out->append("    ");
        }
    }
    
    void decIndent()
    {
        if (_indent == 0) {
            _out->append("*** Error, tried to indent past 0!!!\n");
        } else {
            _indent--;
        }
    }

    void incIndent()
    {
        _indent++;
    }
    
    uint16_t addr() const { return (_it - _in->begin()); }
    
    void outputAddr()
    {
        _out->append("[");
        _out->append(std::to_string(addr() - 1));
        _out->append("] ");
    }

    Error _error = Error::None;
    const std::vector<uint8_t>* _in;
    std::vector<uint8_t>::const_iterator _it;
    std::string* _out;
    int32_t _indent = 4;
    uint16_t _codeOffset = 0; // Used by Call
    const AnnotationList& _annotations;
    int _annotationIndex = 0;
};

}
