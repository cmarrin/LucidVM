/*-------------------------------------------------------------------------
    This source file is a part of Lucid
    For the latest info, see https://github.com/cmarrin/LucidVM
    Copyright (c) 2021-20224, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

// base compiler internals

#pragma once

#include <string>
#include <vector>

#include "Defines.h"

namespace lucid {

// The Function class contains the name and return type, if any.
// It also has a list of the symbols for each arg and local.
// These are in the same list, first args then locals. When the
// function is called the args are pushed followed by the return
// address. On entry the function executes the setFrame instruction,
// which must be the first instruction of every function. This
// pushes the current BP and then computes the new BP to be the
// start of the args. Since there are 2 pointers between the
// end of the args and the locals, the addresses of each local
// is adjusted to account for this. The size of a pointer is
// available in the PointerSize const in Defines.h.

class Function
{
public:
    // Default ctor is used for initialize method. If the name is empty and type is None, it's initialize
    Function() { }
    
    Function(const std::string& name, Type returnType = Type::None)
        : _name(name)
        , _returnType(returnType)
        , _native(false)
    { }

    // Used to create built-in native functions
    Function(const char* name, NativeId nativeId, Type returnType, const SymbolList& locals);

    const std::string& name() const { return _name; }
    Type returnType() const { return _returnType; }
    uint16_t argSize() const { return _argSize; }
    uint16_t localSize() const { return _localHighWaterMark; }
    const SymbolPtr& local(uint8_t i) const { return _locals[i]; }
    const ASTPtr& astNode() const { return _astNode; }

    void addASTNode(const ASTPtr& node);

    uint32_t numLocals() const { return uint32_t(_locals.size()); }
    void pruneLocals(uint32_t n);
    
    bool isNative() const { return _native; }

    bool addArg(const std::string& name, Type type, uint16_t size, bool isPtr)
    {
        if (!addLocal(name, type, size, isPtr)) {
            return false;
        }
        _argSize += size;
        return true;
    }
    
    bool addLocal(const std::string& name, Type type, uint16_t size, bool ptr)
    {
        // Check for duplicates
        if (findLocal(name)) {
            return false;
        }
        _locals.push_back(std::make_shared<Symbol>(name, type, size, _localSize, ptr));
        _localSize += size;
        if (_localHighWaterMark < _localSize) {
            _localHighWaterMark = _localSize;
        }
        return true;
    }

    SymbolPtr findLocal(const std::string& s) const;

private:
    std::string _name;
    ASTPtr _astNode;
    std::vector<SymbolPtr> _locals;
    uint16_t _argSize = 0;// Size in bytes of all args
    uint16_t _localSize = 0; // Size in bytes of all locals, including args
    Type _returnType = Type::None;
    bool _native = false;
    uint8_t _localHighWaterMark = 0;
};

}
