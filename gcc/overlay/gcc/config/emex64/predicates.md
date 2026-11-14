;; SPDX-License-Identifier: GPL-3.0-or-later
;; Copyright (C) 2026 emexlab

;; Every instruction takes a 64-bit immediate where it takes a register.
(define_predicate "emex64_arith_operand"
  (ior (match_operand 0 "register_operand")
       (match_code "const_int")))

(define_predicate "emex64_reg_or_const_int"
  (ior (match_operand 0 "register_operand")
       (match_code "const_int")))

;; A symbol or label, perhaps plus a constant (mov r0, sym / add r0, pc,
;; sym@pcrel).
(define_predicate "emex64_symbolic_operand"
  (and (match_code "symbol_ref,label_ref,const")
       (match_test "emex64_symbolic_operand_p (op)")
       (match_test "!flag_pic || emex64_local_symbolic_p (op)")))

(define_predicate "emex64_movdi_src"
  (ior (match_operand 0 "general_operand")
       (match_operand 0 "emex64_symbolic_operand")))

(define_predicate "emex64_call_operand"
  (ior (match_code "symbol_ref")
       (match_operand 0 "register_operand")))

;; A tp offset: (const (unspec [sym] UNSPEC_TPOFF)), perhaps plus a constant.
;; Nothing else: (const (plus sym n)) would come out as an absolute address.
(define_predicate "emex64_tpoff_operand"
  (and (match_code "const")
       (match_test "emex64_tpoff_const_p (op)")))

;; The compare flags.
(define_predicate "emex64_cf_operand"
  (and (match_code "reg")
       (match_test "REGNO (op) == CF_REGNUM")))

;; Conditions a branch or a set can test in cf.
(define_predicate "emex64_cf_comparison"
  (match_code "eq,ne,lt,gt,le,ge,unordered,ordered,uneq,unlt,ungt,unle,unge,ltgt"))

(define_predicate "emex64_eq_ne_operator"
  (match_code "eq,ne"))

(define_predicate "emex64_lt_gt_operator"
  (match_code "lt,gt"))

;; the integer conditions a v20 compare and branch has (some with the
;; operands swapped)
(define_predicate "emex64_int_comparison_operator"
  (match_code "eq,ne,lt,ge,gt,le,ltu,geu,gtu,leu"))
