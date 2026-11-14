;; SPDX-License-Identifier: GPL-3.0-or-later
;; Copyright (C) 2026 emexlab

(define_register_constraint "f" "TARGET_HARD_FLOAT ? FP_REGS : NO_REGS"
    "A floating point register (fr0-fr15).")

(define_constraint "S"
    "A symbol, the target of a call."
    (match_code "symbol_ref"))

(define_constraint "Z"
    "The integer 0."
    (and (match_code "const_int")
        (match_test "ival == 0")))

(define_memory_constraint "Q"
    "A memory operand addressed by a register alone (cas, xchg)."
    (and (match_code "mem")
        (match_test "REG_P (XEXP (op, 0))")))
