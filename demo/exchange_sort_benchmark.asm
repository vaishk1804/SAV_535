# Exchange Sort Benchmark
# Dataset size: 40 integers
# Array stored at memory 200..239
# Initial values: 40, 39, ..., 1
# Expected sorted result: 1, 2, ..., 40
# Checksum stored at memory[250] = 820
#
# Working set = 40 words > L1 capacity of 32 words

main:
    ADDI R10, R0, 200      # base address
    ADDI R11, R0, 40       # n = 40
    ADDI R1,  R0, 0        # i = 0
    ADDI R8,  R0, 40       # current init value
    ADD  R5,  R10, R0      # ptr = base

init_loop:
    BLT  R1, R11, init_body
    BEQ  R0, R0, sort_start

init_body:
    SW   R8, R5, 0
    ADDI R5, R5, 1
    ADDI R8, R8, -1
    ADDI R1, R1, 1
    BEQ  R0, R0, init_loop

sort_start:
    ADDI R1, R0, 0         # outer i = 0
    ADDI R2, R0, 39        # n - 1

outer_loop:
    BLT  R1, R2, outer_body
    BEQ  R0, R0, checksum_start

outer_body:
    ADDI R3, R0, 0         # j = 0
    SUB  R4, R2, R1        # limit = (n-1) - i
    ADD  R5, R10, R0       # ptr = base

inner_loop:
    BLT  R3, R4, compare_pair
    ADDI R1, R1, 1
    BEQ  R0, R0, outer_loop

compare_pair:
    LW   R6, R5, 0         # a
    LW   R7, R5, 1         # b

    BLT  R7, R6, do_swap
    BEQ  R0, R0, no_swap

do_swap:
    SW   R7, R5, 0
    SW   R6, R5, 1

no_swap:
    ADDI R3, R3, 1
    ADDI R5, R5, 1
    BEQ  R0, R0, inner_loop

checksum_start:
    ADDI R1, R0, 0         # count = 0
    ADDI R9, R0, 0         # checksum = 0
    ADD  R5, R10, R0       # ptr = base

checksum_loop:
    BLT  R1, R11, checksum_body
    BEQ  R0, R0, finish

checksum_body:
    LW   R6, R5, 0
    ADD  R9, R9, R6
    ADDI R5, R5, 1
    ADDI R1, R1, 1
    BEQ  R0, R0, checksum_loop

finish:
    ADDI R12, R0, 250
    SW   R9, R12, 0
    HALT