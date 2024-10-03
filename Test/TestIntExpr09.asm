* 6809 assembly generated from Clover source

    include BOSS9.inc
    org $200

    LEAS -3,S
    TFR S,Y
    LEAS -2,S
    JSR Test_main
    LEAS 2,S
    JMP exit

Test_showIntResults
    PSHS U
    TFR S,U
    ; /*-------------------------------------------------------------------------
    ;     This source file is a part of Clover
    ;     For the latest info, see https://github.com/cmarrin/Clover
    ;     Copyright (c) 2021-2022, Chris Marrin
    ;     All rights reserved.
    ;     Use of this source code is governed by the MIT license that can be
    ;     found in the LICENSE file.
    ; -------------------------------------------------------------------------*/
    ; 
    ; // Test constants, vars, operands and expressions
    ; 
    ; struct Test
    ; {
    ; 
    ; const uint8_t TestSizeDef = 12;
    ; const int16_t TestIntConst = 0xfc;
    ; 
    ; const int8_t testIntTable[ ] = { 1, 2, 3, 7 };
    ; 
    ; int16_t testIntGlobal;
    ; 
    ; uint8_t errors;
    ; 
    ; function showIntResults(uint8_t testNo, int16_t exp, int16_t act)
    ; {
    ;     core.printf("%10s-> %2hhi: ", " ", testNo);
    ;     if (exp != act) {
    LDA 6,U
    TFR A,B
    CLRA
    ADDD #0
    PSHS D
    LDD #String+$0
    PSHS D
    LDD #String+$2
    PSHS D
    JSR printf
    LDD 7,U
    CMPD 9,U
    BEQ L1
    ;         core.printf("     FAIL: exp %i, got %i\n", exp, act);
    ;         errors++;
    LDD 9,U
    PSHS D
    LDD 7,U
    PSHS D
    LDD #String+$11
    PSHS D
    JSR printf
    ;     } else {
    LEAX 2,Y
    LDA 0,X
    PSHS A
    ADDA #1
    STA 0,X
    BRA L2
L1
    ;         core.printf("     Pass\n");
    ;     }
    PSHS X
    LDD #String+$2c
    PSHS D
    JSR printf
L2
    TFR U,S
    PULS U
    RTS

Test_main
    PSHS U
    TFR S,U
    ; }
    ; 
    ; function int16_t main()
    ; {
    ;     errors = 0;
    ;     
    LDA #0
    STA 2,Y
    ;     testIntGlobal = 42;
    ; 
    LDD #42
    STD 0,Y
    ; //    core.printf("\nTest Int Exprs\n");
    ; //
    ; //    core.printf("\n  Int vals\n");
    ; //    showIntResults(1, 12, TestSizeDef);
    ; //    showIntResults(2, 0xfc, TestIntConst);
    ; //    showIntResults(3, 7, testIntTable[3]);
    ; //    
    ; //    core.printf("\n  Int ops\n");
    ; //    showIntResults(4, 294, TestIntConst + testIntGlobal);
    ; //    showIntResults(5, 210, TestIntConst - testIntGlobal);
    ; //    showIntResults(6, 10584, TestIntConst * testIntGlobal);
    ; //    showIntResults(7, 6, TestIntConst / testIntGlobal);
    ; //    
    ; //    showIntResults(8, -42, -testIntGlobal);
    ; //    showIntResults(9, 0, !TestIntConst);
    ; //    showIntResults(10, 0xffffff03, ~TestIntConst);
    ; //    
    ;     showIntResults(11, 0, TestIntConst < testIntGlobal);
    ; //    showIntResults(12, 0, TestIntConst <= testIntGlobal);
    LDD #252
    CMPD 0,Y
    BGE L3
    LDA #1
    BRA L4
L3
    CLRA
L4
    TFR A,B
    SEX
    PSHS D
    LDD #0
    PSHS D
    LDA #11
    PSHS A
    LEAS -2,S
    JSR Test_showIntResults
    LEAS 7,S
    ; //    showIntResults(13, 0, TestIntConst == testIntGlobal);
    ; //    showIntResults(14, 1, TestIntConst != testIntGlobal);
    ; //    showIntResults(15, 1, TestIntConst >= testIntGlobal);
    ; //    showIntResults(16, 1, TestIntConst > testIntGlobal);
    ; //    
    ; //    int8_t i = 20;
    ; //    int8_t j = i++;
    ; //    j = ++i;
    ; //
    ; //    showIntResults(17, 22, i);
    ; //    j = i--;
    ; //    j = --i;
    ; //    showIntResults(18, 20, i);
    ; //    showIntResults(19, 20, j);
    ; //
    ; //    core.printf("\n  op assign\n");
    ; //    i += 1;
    ; //    showIntResults(20, 21, i);
    ; //    i -= 1;
    ; //    showIntResults(21, 20, i);
    ; //    i *= 5;
    ; //    showIntResults(22, 100, i);
    ; //    i /= 5;
    ; //    showIntResults(23, 20, i);
    ; //    
    ; //    core.printf("\n  Int exprs\n");
    ; //    showIntResults(24, 2984, int16_t(testIntTable[1]) + int16_t(TestSizeDef) * TestIntConst - testIntGlobal);
    ; //    showIntResults(25, 2940, int16_t(testIntTable[1] + TestSizeDef) * (TestIntConst - testIntGlobal));
    ; //
    ; //    core.printf("\nDone.%40s%s\n\n", " ", errors ? "FAILED" : "Passed");
    ;     return errors;
    ; }
    LDA 2,Y
    TFR A,B
    SEX
    TFR U,S
    PULS U
    RTS

Constants
    FCB $01,$02,$03,$07

String
    FCC " "
    FCB $00
    FCC "%10s-> %2hhi: "
    FCB $00
    FCC "     FAIL: exp %i, got %i"
    FCB $0a
    FCB $00
    FCC "     Pass"
    FCB $0a
    FCB $00


    end $200
