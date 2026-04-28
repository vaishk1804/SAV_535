# Matrix Multiply Benchmark
# 2x2 matrix multiply: C = A x B
#
# A at 300..303:
# [1 2
#  3 4]
#
# B at 310..313:
# [5 6
#  7 8]
#
# Expected C at 320..323:
# [19 22
#  43 50]

main:
    ADDI R10, R0, 300
    ADDI R11, R0, 310
    ADDI R12, R0, 320

    ADDI R1, R0, 1
    SW   R1, R10, 0
    ADDI R1, R0, 2
    SW   R1, R10, 1
    ADDI R1, R0, 3
    SW   R1, R10, 2
    ADDI R1, R0, 4
    SW   R1, R10, 3

    ADDI R1, R0, 5
    SW   R1, R11, 0
    ADDI R1, R0, 6
    SW   R1, R11, 1
    ADDI R1, R0, 7
    SW   R1, R11, 2
    ADDI R1, R0, 8
    SW   R1, R11, 3

    ADDI R1, R0, 0

outer_i:
    ADDI R13, R0, 2
    BLT  R1, R13, body_i
    HALT

body_i:
    ADDI R2, R0, 0

outer_j:
    ADDI R13, R0, 2
    BLT  R2, R13, body_j
    ADDI R1, R1, 1
    BEQ  R0, R0, outer_i

body_j:
    ADDI R3, R0, 0
    ADDI R4, R0, 0

inner_k:
    ADDI R13, R0, 2
    BLT  R4, R13, multiply_step
    BEQ  R0, R0, store_c

multiply_step:
    ADD  R5, R1, R1
    ADD  R6, R10, R5
    ADD  R6, R6, R4
    LW   R7, R6, 0

    ADD  R8, R4, R4
    ADD  R9, R11, R8
    ADD  R9, R9, R2
    LW   R13, R9, 0

    MUL  R14, R7, R13
    ADD  R3, R3, R14

    ADDI R4, R4, 1
    BEQ  R0, R0, inner_k

store_c:
    ADD  R5, R1, R1
    ADD  R6, R12, R5
    ADD  R6, R6, R2
    SW   R3, R6, 0

    ADDI R2, R2, 1
    BEQ  R0, R0, outer_j