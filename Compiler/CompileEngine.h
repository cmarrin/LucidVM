/*-------------------------------------------------------------------------
    This source file is a part of Clover
    For the latest info, see https://github.com/cmarrin/Clover
    Copyright (c) 2021-2022, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

// Clover compiler
//
// A simple imperative language which generates code that can be 
// executed by the Interpreter
//

#pragma once

#include "Compiler.h"
#include "Function.h"
#include "Defines.h"
#include "Scanner.h"
#include "Struct.h"
#include "Symbol.h"
#include "AST.h"
#include <cstdint>
#include <istream>
#include <variant>

namespace lucid {

//*********************************
//
//  Class: CompileEngine
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
    { import } { struct } [ <type> <id> ] ;

import:
    'import' <id> [ 'as' <id> ] ;
    
struct:
    'struct' <id> '{' { structEntry ';' } '}' ;
    
structEntry:
    constant | struct | varStatement | function | init  ;

constant:
    'const' type <id> '=' value ';' ;
    
varStatement:
    type [ '*' ] var ';' ;

var:
    <id> [ '[' <integer> ']' ] [ '=' initializer ] ;
    
initializer:
    arithmeticExpression | '{' [ initializer { ',' initializer } ] '}'

function:
    'function' [ <type> ] <id> '(' formalParameterList ')' '{' { statement } '}' ;

init:
    'initialize' '(' ')' '{' { statement } '}' ;
    
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
    ;
    
*/

static constexpr uint8_t StructTypeStart = 0x80; // Where struct types start

class CompileEngine : public Compiler {
public:
  	CompileEngine(std::istream* stream, AnnotationList* annotations)
        : _scanner(stream, annotations)
    { }

    void addNative(const char* name, uint8_t nativeId, Type type, const SymbolList& locals)
    {
        _functions.emplace_back(name, nativeId, type, locals);
    }

    bool program();

    Compiler::Error error() const { return _error; }
    Token expectedToken() const { return _expectedToken; }
    const std::string& expectedString() const { return _expectedString; }
    uint32_t lineno() const { return _scanner.lineno(); }
    uint32_t charno() const { return _scanner.charno(); }

protected:
    bool statement();
    bool function();
    bool type(Type&);
  
    bool varStatement();
    bool var(Type, bool isPointer);
    bool init();

    bool value(int32_t& i, Type);

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
    bool assignmentExpression() { return arithmeticExpression(1, ArithType::Assign); }
    bool arithmeticExpression(uint8_t minPrec = 1, ArithType = ArithType::Op);
    bool unaryExpression();
    bool postfixExpression();
    bool primaryExpression();

    bool formalParameterList();
    bool argumentList(const Function& fun);
    
    virtual bool isReserved(Token token, const std::string str, Reserved&);

    bool findSymbol(const std::string&, Symbol&);
    uint8_t findInt(int32_t);
    uint8_t findFloat(float);

    // The expect methods validate the passed param and if
    // there is no match, the passed error is saved and
    // throw is called. The first version also retires the
    // current token.
    void expect(Token token, const char* str = nullptr);
    void expect(bool passed, Compiler::Error error);
    void expectWithoutRetire(Token token);
    bool match(Reserved r);
    bool match(Token r);
    void ignoreNewLines();
    
    // These methods check to see if the next token is of the
    // appropriate type. Some versions just return true or
    // false, others also return details about the token
    bool identifier(std::string& id, bool retire = true);
    bool integerValue(int32_t& i);
    bool floatValue(float& f);
    bool stringValue(std::string&);
    bool reserved();
    bool reserved(Reserved &r);
    
    bool addASTNode(std::shared_ptr<ASTNode> node)
    {
        if (!_currentNodeStack.back()->addNode(node)) {
            return false;
        }
        _currentNodeStack.push_back(node);
        return true;
    }
    
    bool popASTNode()
    {
        if (_currentNodeStack.empty()) {
            return false;
        }
        _currentNodeStack.pop_back();
        return true;
    }
    
    const Function& handleFunctionName();

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
    
    Function& currentFunction()
    {
        expect(!_functions.empty(), Compiler::Error::InternalError);
        return _functions.back();
    }
    
    void annotate()
    {
        if (_scanner.annotation() == -1) {
            //_scanner.setAnnotation(int32_t(_rom8.size()));
        }
    }
    
    uint8_t allocNativeId() { return _nextNativeId++; }
        
    bool findFunction(const std::string&, Function&);

    Compiler::Error _error = Compiler::Error::None;
    Token _expectedToken = Token::None;
    std::string _expectedString;
    
    // The ExprStack
    //
    // This is a stack of the operators being processed. Values can be:
    //
    //      Id      - string id
    //      Float   - float constant
    //      Int     - int32_t constant
    //      Dot     - r0 contains a ref. entry is an index into a Struct.

    // ExprAction indicates what to do with the top entry on the ExprStack during baking
    //
    //      Right       - Entry is a RHS, so it can be a float, int or id and the value 
    //                    is loaded into r0
    //      Left        - Entry is a LHS, so it must be an id, value in r0 is stored at the id
    //      Function    - Entry is the named function which has already been emitted so value
    //                    is the return value in r0
    //      Ref         - Value left in r0 must be an address to a value (Const or RAM)
    //      Deref       = Value must be a Struct entry for the value in _stackEntry - 1
    //      Dot         - Dot operator. TOS must be a struct id, TOS-1 must be a ref with
    //                    a type. Struct id must be a member of the type of the ref.
    //                    pop the two 
    //
    enum class ExprAction { Left, Right, Ref, Index, Offset };
    bool isExprFunction();
    uint8_t elementSize(Type);

    struct Constant
    {
        Constant(Type t, const std::string& id, int32_t value) : _type(t), _id(id), _value(value) { }
        
        Type _type = Type::None;
        std::string _id;
        int32_t _value = 0;        // Value can be float, fixed, signed or unsigned int. Cast as needed
    };
    
    bool findConstant(const std::string& id, int32_t& value)
    {
        const auto& it = find_if(_constants.begin(), _constants.end(),
                [id](const Constant& c) { return c._id == id; });

        if (it != _constants.end()) {
            value = it->_value;
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
    
    bool structFromType(Type, Struct&);
    void findStructElement(Type, const std::string& id, uint8_t& index, Type&);

    Struct& currentStruct()
    {
        expect(!_structStack.empty(), Compiler::Error::InternalError);
        return _structs[_structStack.back()];
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

    std::vector<Constant> _constants;
    std::vector<Function> _functions;
    std::vector<Struct> _structs;
    std::vector<uint32_t> _structStack;
    std::vector<Symbol> _builtins;

    std::vector<std::shared_ptr<ASTNode>> _currentNodeStack;
    
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
