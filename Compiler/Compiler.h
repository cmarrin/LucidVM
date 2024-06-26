/*-------------------------------------------------------------------------
    This source file is a part of Clover
    For the latest info, see https://github.com/cmarrin/Clover
    Copyright (c) 2021-2022, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#pragma once

#include <cstdint>
#include <istream>
#include <vector>

#include "NativeModule.h"
#include "Defines.h"
#include "Scanner.h"

namespace lucid {

struct OpData
{
    OpData() { }
    OpData(std::string str, Op op, OpParams par) : _str(str), _op(op), _par(par) { }
    std::string _str;
    Op _op = Op::Return;
    OpParams _par = OpParams::None;
};

class Compiler
{
public:
    enum class Error {
        None,
        UnrecognizedLanguage,
        ExpectedToken,
        ExpectedKeyword,
        ExpectedType,
        ExpectedValue,
        ExpectedString,
        ExpectedRef,
        ExpectedOpcode,
        ExpectedEnd,
        ExpectedIdentifier,
        ExpectedExpr,
        ExpectedArgList,
        ExpectedFormalParams,
        ExpectedFunction,
        ExpectedLHSExpr,
        ExpectedStructType,
        ExpectedVar,
        AssignmentNotAllowedHere,
        InvalidStructId,
        InvalidParamCount,
        UndefinedIdentifier,
        ParamOutOfRange,
        JumpTooBig,
        IfTooBig,
        ElseTooBig,
        StringTooLong,
        TooManyConstants,
        TooManyVars,
        DefOutOfRange,
        ExpectedDef,
        NoMoreTemps,
        TempNotAllocated,
        InternalError,
        StackTooBig,
        MismatchedType,
        WrongNumberOfArgs,
        WrongType,
        OnlyAllowedInLoop,
        DuplicateIdentifier,
        ExecutableTooBig,
        InitializerNotAllowed,
        ConstMustBeSimpleType,
    };
    
    Compiler() { }
    
    bool compile(std::istream*, 
                 std::vector<uint8_t>& executable, uint32_t maxExecutableSize,
                 const std::vector<NativeModule*>&,
                 AnnotationList* annotations = nullptr);

    Error error() const { return _error; }
    Token expectedToken() const { return _expectedToken; }
    const std::string& expectedString() const { return _expectedString; }
    uint32_t lineno() const { return _lineno; }
    uint32_t charno() const { return _charno; }        

private:
    Error _error = Error::None;
    Token _expectedToken = Token::None;
    std::string _expectedString;
    uint32_t _lineno;
    uint32_t _charno;
};

}
