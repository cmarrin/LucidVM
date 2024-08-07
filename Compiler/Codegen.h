/*-------------------------------------------------------------------------
    This source file is a part of Clover
    For the latest info, see https://github.com/cmarrin/Clover
    Copyright (c) 2021-2022, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

// Lucid code generator
//

#pragma once

#include "AST.h"
#include "Defines.h"
#include "Symbol.h"

namespace lucid {

//*********************************
//
//  Class: Codegen
//
//*********************************

/*

Input is an AST, which represents a statement. Output is virtual machine code.
The VM is register based. There is an accumulator (8, 16 or 32 bit) which
holds signed or unsigned integers or a single precision float.

NOTE:   I'm considering having a 16 bit fixed point float (8:8 format) to save
        space. If I do that it might be a good idea to have a 32 bit fixed
        format and get rid of native floating point support entirely. But
        in tests on Arduino and ESP, I'm not sure if my fixed point library
        is any better than the floating point libraries on those platforms.
        
NOTE:   I'm also considering a bool type. It would be 8 bits, so maybe not
        worth it. Maybe I'll just add true and false keywords that evaluate
        to uint8, 1 and 0.
*/

class Codegen {
public:
    Codegen(std::vector<uint8_t>* code) : _code(code) { }
  	
    bool processAST(const ASTPtr&);

private:
    bool processNextASTNode(const ASTPtr&, bool isLHS);

    // The ExprStack
    //
    // This is a stack of the operators being processed. Values can be:
    //
    //      Id      - string id
    //      Scalae  - ineger or float constant
    //      Dot     - index into a struct.

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

    std::vector<uint8_t>* _code;
};

}
