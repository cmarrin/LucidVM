* 6809 assembly generated from Clover source

    include BOSS9.inc
    org $200
�    LEAX 0,?
    PSHS X
    PULS D
    STD 1,U
    LDA #$00
    PSHS A
    PULS A
    STA 2,U
    LDA 2,U
    PSHS A
    LDA #$04
    PSHS A
    LDA 1,S
    CMPA 0,S
    LEAS 2,S
    BLT L4
    CLRA
    BRA L2
L4
    LDA #1
L5
    PSHS A
PT    LDD 1,U
    PSHS D
    LDA 2,U
    PSHS A
0    LEAX String+$1e
    PSHS X
X�V�    LDD #$0000
    PSHS D
6