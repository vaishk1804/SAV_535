# Matrix Multiply Benchmark
# 7x7 matrices
#
# A at 300..348 : values 1..49
# B at 400..448 : all ones
# C at 500..548 : result
# Checksum of C stored at memory[700] = 8575
#
# Total working set = 49 + 49 + 49 = 147 words > L2 capacity of 128 words
#
# Address formula uses row*7.
# Since there is no MUL-immediate, row*7 is computed as:
# row*7 = (row << 3) - row

main:
    ADDI R10, R0, 300      # A base
    ADDI R11, R0, 400      # B base
    ADDI R12, R0, 500      # C base
    ADDI R13, R0, 7        # N = 7
    ADDI R14, R0, 49       # total elements = 49
    ADDI R15, R0, 3        # shift amount for *8

# ---------------- Initialize A = 1..49 ----------------
    ADDI R1, R0, 0         # idx = 0
    ADDI R2, R0, 1         # value = 1
    ADD  R3, R10, R0       # ptr = A base

init_a_loop:
    BLT  R1, R14, init_a_body
    BEQ  R0, R0, init_b_start

init_a_body:
    SW   R2, R3, 0
    ADDI R3, R3, 1
    ADDI R2, R2, 1
    ADDI R1, R1, 1
    BEQ  R0, R0, init_a_loop

# ---------------- Initialize B = all ones ----------------
init_b_start:
    ADDI R1, R0, 0         # idx = 0
    ADDI R2, R0, 1         # constant one
    ADD  R3, R11, R0       # ptr = B base

init_b_loop:
    BLT  R1, R14, init_b_body
    BEQ  R0, R0, multiply_start

init_b_body:
    SW   R2, R3, 0
    ADDI R3, R3, 1
    ADDI R1, R1, 1
    BEQ  R0, R0, init_b_loop

# ---------------- Matrix multiply C = A x B ----------------
multiply_start:
    ADDI R1, R0, 0         # i = 0

outer_i:
    BLT  R1, R13, body_i
    BEQ  R0, R0, checksum_start

body_i:
    ADDI R2, R0, 0         # j = 0

outer_j:
    BLT  R2, R13, body_j
    ADDI R1, R1, 1
    BEQ  R0, R0, outer_i

body_j:
    ADDI R4, R0, 0         # sum = 0
    ADDI R3, R0, 0         # k = 0

inner_k:
    BLT  R3, R13, multiply_step
    BEQ  R0, R0, store_c

multiply_step:
    # addrA = A + (i*7) + k
    SLL  R5, R1, R15       # i*8
    SUB  R5, R5, R1        # i*7
    ADD  R6, R10, R5
    ADD  R6, R6, R3
    LW   R7, R6, 0         # a

    # addrB = B + (k*7) + j
    SLL  R5, R3, R15       # k*8
    SUB  R5, R5, R3        # k*7
    ADD  R6, R11, R5
    ADD  R6, R6, R2
    LW   R8, R6, 0         # b

    MUL  R9, R7, R8
    ADD  R4, R4, R9

    ADDI R3, R3, 1
    BEQ  R0, R0, inner_k

store_c:
    # addrC = C + (i*7) + j
    SLL  R5, R1, R15       # i*8
    SUB  R5, R5, R1        # i*7
    ADD  R6, R12, R5
    ADD  R6, R6, R2
    SW   R4, R6, 0

    ADDI R2, R2, 1
    BEQ  R0, R0, outer_j

# ---------------- Checksum of output matrix ----------------
checksum_start:
    ADDI R1, R0, 0         # idx = 0
    ADDI R4, R0, 0         # checksum = 0
    ADD  R6, R12, R0       # ptr = C base

checksum_loop:
    BLT  R1, R14, checksum_body
    BEQ  R0, R0, finish

checksum_body:
    LW   R7, R6, 0
    ADD  R4, R4, R7
    ADDI R6, R6, 1
    ADDI R1, R1, 1
    BEQ  R0, R0, checksum_loop

finish:
    ADDI R5, R0, 700
    SW   R4, R5, 0
    HALT