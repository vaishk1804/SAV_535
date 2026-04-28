# Exchange Sort Benchmark
# Sorts 4 integers in ascending order
# Initial array: [9, 3, 7, 1]
# Final array:   [1, 3, 7, 9]

main:
    ADDI R10, R0, 200

    ADDI R1, R0, 9
    SW   R1, R10, 0

    ADDI R1, R0, 3
    SW   R1, R10, 1

    ADDI R1, R0, 7
    SW   R1, R10, 2

    ADDI R1, R0, 1
    SW   R1, R10, 3

    ADDI R1, R0, 0
    ADDI R2, R0, 3

outer_loop:
    BLT  R1, R2, outer_body
    HALT

outer_body:
    ADDI R3, R0, 0
    SUB  R4, R2, R1
    ADD  R5, R10, R0

inner_loop:
    BLT  R3, R4, compare_pair
    ADDI R1, R1, 1
    BEQ  R0, R0, outer_loop

compare_pair:
    LW   R6, R5, 0
    LW   R7, R5, 1

    BLT  R7, R6, do_swap
    BEQ  R0, R0, no_swap

do_swap:
    SW   R7, R5, 0
    SW   R6, R5, 1

no_swap:
    ADDI R3, R3, 1
    ADDI R5, R5, 1
    BEQ  R0, R0, inner_loop