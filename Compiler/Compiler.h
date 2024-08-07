/*-------------------------------------------------------------------------
    This source file is a part of Clover
    For the latest info, see https://github.com/cmarrin/Lucid
    Copyright (c) 2021-2024, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

// Lucid compiler
//
// A simple imperative language which generates code that can be 
// executed by the Interpreter
//

#pragma once

#include <cstdint>
#include <istream>
#include <variant>

#include "AST.h"
#include "Function.h"
#include "Defines.h"
#include "Module.h"
#include "Scanner.h"
#include "Struct.h"

namespace lucid {

//*********************************
//
//  Class: Compiler
//
//*********************************

/*

A program consists of zero or more imports followed by
zero or more structs, optionally followed by an
instantiation of a struct. There can only be one
such instantiation in a program but it can appear in
the main module or any imported module. On start this
struct is instantiated and its ctor is called.

BNF:

program:
    { import } { constant } struct ;

import:
    'import' <id> [ 'as' <id> ] ;
    
struct:
    'struct' <id> '{' { structEntry ';' } '}' ;
    
structEntry:
    struct | varStatement | function | init  ;

constant:
    'const' type <id> '=' value ';' ;
    
varStatement:
    type [ '*' ] var ';' ;

var:
    <id> [ '[' <integer> ']' ] [ '=' initializer ] ;
    
initializer:
    arithmeticExpression | '{' [ initializer { ',' initializer } ] '}'

function:
    'function' [ <type> ] <id> '(' formalParameterList ')' compoundStatement ;

init:
    'initialize' '(' ')' compoundStatement ;
    
// <id> is a struct name
type:
      'float'
    | 'hfloat'
    | 'int8_t'
    | 'uint8_t'
    | 'int16_t'
    | 'uint16_t'
    | 'int32_t'
    | 'uint32_t'
    | <id>
    ;
    
statement:
      compoundStatement
    | ifStatement
    | forStatement
    | whileStatement
    | loopStatement
    | returnStatement
    | jumpStatement
    | varStatement
    | expressionStatement
    ;
  
compoundStatement:
    '{' { statement } '}' ;

ifStatement:
    'if' '(' arithmeticExpression ')' statement ['else' statement ] ;

forStatement:
    'for' '(' [ [ <type> ] identifier '=' arithmeticExpression ] ';'
            [ arithmeticExpression ] ';' [ arithmeticExpression ] ')' statement ;
    
whileStatement:
    'while' '(' arithmeticExpression ')' statement ;

loopStatement:
    'loop' statement ;

returnStatement:
      'return' [ arithmeticExpression ] ';' ;
      
switchStatement:
    'switch' '(' arithmeticExpression ')' '{' { caseClause } '}' ;

caseClause:
    ('case' arithmeticExpression | 'default) ':' statement ;

jumpStatement:
      'break' ';'
    | 'continue' ';'
    ;

expressionStatement:
    arithmeticExpression ';' ;
    
arithmeticExpression:
      unaryExpression
    | unaryExpression operator arithmeticExpression

unaryExpression:
      postfixExpression
    | '-' unaryExpression
    | '~' unaryExpression
    | '!' unaryExpression
    | '++' unaryExpression
    | '--' unaryExpression
    | '&' unaryExpression
    ;

postfixExpression:
      primaryExpression
    | postfixExpression '(' argumentList ')'
    | postfixExpression '[' arithmeticExpression ']'
    | postfixExpression '.' identifier
    | postfixExpression '++'
    | postfixExpression '--'
    ;

primaryExpression:
      '(' arithmeticExpression ')'
    | <id>
    | <float>
    | <integer>
    ;
    
formalParameterList:
      (* empty *)
    | type ['*'] identifier { ',' type identifier }
    ;

argumentList:
        (* empty *)
      | arithmeticExpression { ',' arithmeticExpression }
      ;

operator: (* operator   precedence   association *)
               '='     (*   1          Right    *)
    |          '+='    (*   1          Right    *)
    |          '-='    (*   1          Right    *)
    |          '*='    (*   1          Right    *)
    |          '/='    (*   1          Right    *)
    |          '&='    (*   1          Right    *)
    |          '|='    (*   1          Right    *)
    |          '^='    (*   1          Right    *)
    |          '||'    (*   2          Left     *)
    |          '&&'    (*   3          Left     *)
    |          '|'     (*   4          Left     *)
    |          '^'     (*   5          Left     *)
    |          '&'     (*   6          Left     *)
    |          '=='    (*   7          Left     *)
    |          '!='    (*   7          Left     *)
    |          '<'     (*   8          Left     *)
    |          '>'     (*   8          Left     *)
    |          '>='    (*   8          Left     *)
    |          '<='    (*   8          Left     *)
    |          '+'     (*   10         Left     *)
    |          '-'     (*   10         Left     *)
    |          '*'     (*   11         Left     *)
    |          '/'     (*   11         Left     *)
    |          '%'     (*   11         Left     *)
    ;
    
*/

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

static constexpr uint8_t StructTypeStart = 0x80; // Where struct types start

class Compiler {
public:
  	Compiler(std::istream* stream, AnnotationList* annotations)
        : _scanner(stream, annotations)
    { }

    bool compile(std::vector<uint8_t>& executable, uint32_t maxExecutableSize,
                 const std::vector<Module*>&);

//    void addNative(const char* name, uint8_t nativeId, Type type, const SymbolList& locals)
//    {
//        _functions.emplace_back(name, nativeId, type, locals);
//    }

    bool program();

    Error error() const { return _error; }
    Token expectedToken() const { return _expectedToken; }
    const std::string& expectedString() const { return _expectedString; }
    uint32_t lineno() const { return _scanner.lineno(); }
    uint32_t charno() const { return _scanner.charno(); }

    const std::vector<StructPtr>& structs() const { return _structs; }
    
    const StructPtr structFromType(Type type) const
    {
        uint8_t i = uint8_t(type);
        if (i < StructTypeStart) {
            return nullptr;
        }
        i -= StructTypeStart;
        if (i >= _structTypes.size()) {
            return nullptr;
        }
        return _structTypes[i];
    }
    
protected:
    bool statement();
    bool function();
    bool type(Type&);
  
    bool varStatement();
    bool var(Type, bool isPointer);
    bool init();

    bool value(uint32_t& i, Type);

private:
    bool import();
    bool strucT();
    
    bool structEntry();
    bool constant();
    
    bool compoundStatement();
    bool ifStatement();
    bool forStatement();
    bool whileStatement();
    bool loopStatement();
    bool returnStatement();
    bool jumpStatement();
    bool expressionStatement();
    
    enum class ArithType { Assign, Op };
    
    ASTPtr expression();
    ASTPtr arithmeticExpression(const ASTPtr& lhs, uint8_t minPrec = 1);
    ASTPtr unaryExpression();
    ASTPtr postfixExpression();
    ASTPtr primaryExpression();

    bool formalParameterList();
    bool argumentList(Function* fun);
    
    virtual bool isReserved(Token token, const std::string str, Reserved&);

    StructPtr addStruct(const std::string& name, Type type)
    {
        _structs.push_back(std::make_shared<Struct>(name, type));
        return _structs.back();
    }

    StructPtr findStruct(const std::string&);
    SymbolPtr findSymbol(const std::string&);
    ModulePtr findModule(const std::string&);
    uint8_t findInt(int32_t);
    uint8_t findFloat(float);

    // The expect methods validate the passed param and if
    // there is no match, the passed error is saved and
    // throw is called. The first version also retires the
    // current token.
    void expect(Token token, const char* str = nullptr);
    void expect(bool passed, Error error);
    void expectWithoutRetire(Token token);
    bool match(Reserved r);
    bool match(Token r);
    void ignoreNewLines();
    
    // These methods check to see if the next token is of the
    // appropriate type. Some versions just return true or
    // false, others also return details about the token
    bool identifier(std::string& id, bool retire = true);
    bool integerValue(uint32_t& i);
    bool floatValue(float& f);
    bool stringValue(std::string&);
    bool reserved();
    bool reserved(Reserved &r);
    
    uint16_t sizeInBytes(Type type) const;
    
    struct Def
    {
        Def() { }
        Def(std::string name, uint8_t value)
            : _name(name)
            , _value(value)
        { }
        std::string _name;
        uint8_t _value;
    };
    
    Function* currentFunction()
    {
        expect(_inFunction && _currentFunction, Error::InternalError);
        return _currentFunction;
    }
    
    void annotate()
    {
        if (_scanner.annotation() == -1) {
            //_scanner.setAnnotation(int32_t(_rom8.size()));
        }
    }
    
    uint8_t allocNativeId() { return _nextNativeId++; }

    Error _error = Error::None;
    Token _expectedToken = Token::None;
    std::string _expectedString;
    
    struct Constant
    {
        Constant(Type t, const std::string& id, int32_t value) : _type(t), _id(id), _value(value) { }
        
        Type _type = Type::None;
        std::string _id;
        int32_t _value = 0;        // Value can be float, fixed, signed or unsigned int. Cast as needed
    };
    
    bool findConstant(const std::string& id, Type& t, uint32_t& v)
    {
        const auto& it = find_if(_constants.begin(), _constants.end(),
                [id](const Constant& c) { return c._id == id; });

        if (it != _constants.end()) {
            t = it->_type;
            v = it->_value;
            return true;
        }
        return false;
    }
    
    struct ParamEntry
    {
        ParamEntry(const std::string& name, Type type)
            : _name(name)
            , _type(type)
        { }
        std::string _name;
        Type _type;
    };
    
    bool structFromType(Type, StructPtr&);

    StructPtr currentStruct()
    {
        expect(!_structStack.empty(), Error::InternalError);
        return _structStack.back();
    }

    struct JumpEntry
    {
        enum class Type { Start, Continue, Break };
        
        JumpEntry(Type type, uint16_t addr) : _type(type), _addr(addr) { }
        
        Type _type;
        uint16_t _addr;
    };

    void enterJumpContext() { _jumpList.emplace_back(); }
    void exitJumpContext(uint16_t startAddr, uint16_t contAddr, uint16_t breakAddr);
    void addJumpEntry(Op, JumpEntry::Type);
    
    Scanner _scanner;

    bool _inFunction = false;
    Function* _currentFunction = nullptr;
    
    uint8_t _nextStructType = StructTypeStart;
    std::vector<StructPtr> _structTypes; // array of structs, ordered by (type-StructTypeStart)

    uint32_t _entryStructIndex;
    
    std::vector<Constant> _constants;
    std::vector<StructPtr> _structs;
    std::vector<StructPtr> _structStack;
    std::vector<ModulePtr> _modules;
    
    // The jump list is an array of arrays of JumpEntries. The outermost array
    // is a stack of active looping statements (for, loop, etc.). Each of these
    // has an array of break or continue statements that need to be resolved
    // when the looping statement ends.
    std::vector<std::vector<JumpEntry>> _jumpList;

    uint8_t _nextNativeId = 0;
    uint16_t _nextMem = 0; // next available location in mem
    uint16_t _localHighWaterMark = 0;
};

}
