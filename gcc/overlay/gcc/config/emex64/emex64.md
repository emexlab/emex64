;; SPDX-License-Identifier: GPL-3.0-or-later
;; Copyright (C) 2026 emexlab

(define_constants
  [(R0_REGNUM	0)
   (R1_REGNUM	1)
   (RR_REGNUM	10)
   (ER4_REGNUM	15)
   (ER5_REGNUM	16)
   (TP_REGNUM	26)
   (SP_REGNUM	27)
   (FP_REGNUM	28)
   (CF_REGNUM	29)
   (FR0_REGNUM	30)])

(define_c_enum "unspec" [
  UNSPEC_GOTPCREL	; [pc + sym@gotpcrel]
  UNSPEC_TPOFF		; sym@tpoff
  UNSPEC_GOTTPOFF	; [pc + sym@gottpoff]
  UNSPEC_TLSGD		; add r, pc, sym@tlsgd
  UNSPEC_XPAC
  UNSPEC_CPYM
  UNSPEC_MOVM
  UNSPEC_SETM
  UNSPEC_FENCE
  UNSPEC_FMIN
  UNSPEC_FMAX
])

(define_c_enum "unspecv" [
  UNSPECV_BLOCKAGE
  UNSPECV_PACRET
  UNSPECV_CAS
  UNSPECV_XCHG
  UNSPECV_LDOP
])

(include "predicates.md")
(include "constraints.md")

;; -------------------------------------------------------------------------
;; Iterators
;; -------------------------------------------------------------------------

(define_mode_iterator QHS [QI HI SI])
(define_mode_iterator QHSD [QI HI SI DI])
(define_mode_iterator HSD [HI SI DI])
(define_mode_iterator ANYF [SF DF])
(define_mode_iterator SD [SI DI])

(define_mode_attr ldst [(QI "b") (HI "w") (SI "d") (DI "q")])
(define_mode_attr bits [(QI "8") (HI "16") (SI "32") (DI "64")])
(define_mode_attr mask [(QI "0xff") (HI "0xffff") (SI "0xffffffff")])
;; stores print their immediate zero extended (shorter encodings)
(define_mode_attr stimm [(QI "B") (HI "W") (SI "D") (DI "")])

(define_code_iterator any_extend [sign_extend zero_extend])
(define_code_attr u [(sign_extend "") (zero_extend "u")])
(define_code_attr su [(sign_extend "s") (zero_extend "u")])

(define_mode_attr fs [(SF "s") (DF "")])
(define_mode_attr fldst [(SF "d") (DF "q")])
(define_mode_attr fcvt [(SF "s") (DF "d")])

;; -------------------------------------------------------------------------
;; Moves
;; -------------------------------------------------------------------------

(define_expand "mov<mode>"
  [(set (match_operand:QHSD 0 "nonimmediate_operand")
	(match_operand:QHSD 1 "general_operand"))]
  ""
{
  if (emex64_expand_move (<MODE>mode, operands))
    DONE;
})

(define_insn "*mov<mode>_insn"
  [(set (match_operand:QHS 0 "nonimmediate_operand" "=r,r,r,m,m")
	(match_operand:QHS 1 "general_operand"      " r,n,m,r,n"))]
  "register_operand (operands[0], <MODE>mode)
   || emex64_reg_or_const_int (operands[1], <MODE>mode)"
  "@
   mov\t%0, %1
   * return emex64_output_const_move (operands);
   ld<ldst>\t%0, %1
   st<ldst>\t%0, %1
   st<ldst>\t%0, %<stimm>1")

(define_insn "*movdi_insn"
  [(set (match_operand:DI 0 "nonimmediate_operand" "=r,r,r,r,m,m")
	(match_operand:DI 1 "emex64_movdi_src"      " r,n,s,m,r,n"))]
  "register_operand (operands[0], DImode)
   || emex64_reg_or_const_int (operands[1], DImode)"
  "@
   mov\t%0, %1
   * return emex64_output_const_move (operands);
   * return emex64_output_symbolic_move (operands);
   ldq\t%0, %1
   stq\t%0, %1
   stq\t%0, %1")

(define_expand "mov<mode>"
  [(set (match_operand:ANYF 0 "nonimmediate_operand")
	(match_operand:ANYF 1 "general_operand"))]
  ""
{
  if (emex64_expand_move (<MODE>mode, operands))
    DONE;
})

;; A single in an FP register is in its low 32 bits; mov copies all 64.
(define_insn "*mov<mode>_insn"
  [(set (match_operand:ANYF 0 "nonimmediate_operand" "=f,f,f,f,r,m,r,r,r,m,m")
	(match_operand:ANYF 1 "general_operand"      " f,r,F,m,f,f,r,F,m,r,F"))]
  "register_operand (operands[0], <MODE>mode)
   || register_operand (operands[1], <MODE>mode)
   || CONST_DOUBLE_P (operands[1])"
  "@
   mov\t%0, %1
   mov\t%0, %1
   mov\t%0, %F1
   ld<fldst>\t%0, %1
   mov\t%0, %1
   st<fldst>\t%0, %1
   mov\t%0, %1
   mov\t%0, %F1
   ld<fldst>\t%0, %1
   st<fldst>\t%0, %1
   st<fldst>\t%0, %F1")

;; TLS local exec: add r, tp, sym@tpoff
(define_insn "emex64_add_tpoff"
  [(set (match_operand:DI 0 "register_operand" "=r")
	(plus:DI (match_operand:DI 1 "register_operand" "r")
		 (match_operand:DI 2 "emex64_tpoff_operand" "")))]
  ""
  "add\t%0, %1, %2")

;; TLS general dynamic: the argument of __tls_get_addr
(define_insn "emex64_load_tlsgd"
  [(set (match_operand:DI 0 "register_operand" "=r")
	(unspec:DI [(match_operand:DI 1 "" "")] UNSPEC_TLSGD))]
  ""
  "add\t%0, pc, %1@tlsgd")

;; -------------------------------------------------------------------------
;; Extensions
;; -------------------------------------------------------------------------

(define_insn "zero_extend<QHS:mode><HSD:mode>2"
  [(set (match_operand:HSD 0 "register_operand" "=r,r")
	(zero_extend:HSD (match_operand:QHS 1 "nonimmediate_operand" "r,m")))]
  "GET_MODE_SIZE (<QHS:MODE>mode) < GET_MODE_SIZE (<HSD:MODE>mode)"
  "@
   and\t%0, %1, <QHS:mask>
   ld<QHS:ldst>\t%0, %1")

(define_insn "extend<QHS:mode><HSD:mode>2"
  [(set (match_operand:HSD 0 "register_operand" "=r,r")
	(sign_extend:HSD (match_operand:QHS 1 "nonimmediate_operand" "r,m")))]
  "GET_MODE_SIZE (<QHS:MODE>mode) < GET_MODE_SIZE (<HSD:MODE>mode)"
{
  return emex64_output_sign_extend (operands, <QHS:bits>);
})

;; -------------------------------------------------------------------------
;; Integer arithmetic (64 bits; narrower modes are widened by the
;; middle end)
;; -------------------------------------------------------------------------

;; Immediates are unsigned: a negative one takes all 64 bits. Adding -n is
;; sub n (and the other way around), with n in as few bits as it needs.
(define_insn "adddi3"
  [(set (match_operand:DI 0 "register_operand" "=r,r")
	(plus:DI (match_operand:DI 1 "register_operand" "%r,r")
		 (match_operand:DI 2 "emex64_arith_operand" "r,n")))]
  ""
{
  if (emex64_neg_imm_p (operands[2]))
    return "sub\t%0, %1, %N2";
  return "add\t%0, %1, %2";
})

(define_insn "subdi3"
  [(set (match_operand:DI 0 "register_operand" "=r,r,r")
	(minus:DI (match_operand:DI 1 "emex64_arith_operand" "r,r,n")
		  (match_operand:DI 2 "emex64_arith_operand" "r,n,r")))]
  ""
{
  if (emex64_neg_imm_p (operands[2]))
    return "add\t%0, %1, %N2";
  return "sub\t%0, %1, %2";
})

(define_insn "negdi2"
  [(set (match_operand:DI 0 "register_operand" "=r")
	(neg:DI (match_operand:DI 1 "register_operand" "r")))]
  ""
  "sub\t%0, 0, %1")

(define_insn "muldi3"
  [(set (match_operand:DI 0 "register_operand" "=r,r")
	(mult:DI (match_operand:DI 1 "register_operand" "%r,r")
		 (match_operand:DI 2 "emex64_arith_operand" "r,n")))]
  ""
  "mul\t%0, %1, %2")

(define_code_iterator any_div [div udiv umod])
(define_code_attr div_insn [(div "idiv") (udiv "div") (umod "mod")])
(define_code_attr div_name [(div "div") (udiv "udiv") (umod "umod")])

(define_insn "<div_name>di3"
  [(set (match_operand:DI 0 "register_operand" "=r,r,r")
	(any_div:DI (match_operand:DI 1 "emex64_arith_operand" "r,r,n")
		    (match_operand:DI 2 "emex64_arith_operand" "r,n,r")))]
  ""
  "<div_insn>\t%0, %1, %2")

(define_code_iterator any_logic [and ior xor])
(define_code_attr logic_insn [(and "and") (ior "or") (xor "xor")])

(define_insn "<code>di3"
  [(set (match_operand:DI 0 "register_operand" "=r,r")
	(any_logic:DI (match_operand:DI 1 "register_operand" "%r,r")
		      (match_operand:DI 2 "emex64_arith_operand" "r,n")))]
  ""
  "<logic_insn>\t%0, %1, %2")

(define_insn "one_cmpldi2"
  [(set (match_operand:DI 0 "register_operand" "=r")
	(not:DI (match_operand:DI 1 "register_operand" "0")))]
  ""
  "not\t%0")

(define_code_iterator any_shift [ashift ashiftrt lshiftrt])
(define_code_attr shift_insn [(ashift "shl") (ashiftrt "sar") (lshiftrt "shr")])
(define_code_attr shift_name [(ashift "ashl") (ashiftrt "ashr") (lshiftrt "lshr")])

(define_insn "<shift_name>di3"
  [(set (match_operand:DI 0 "register_operand" "=r,r,r")
	(any_shift:DI (match_operand:DI 1 "emex64_arith_operand" "r,r,n")
		      (match_operand:DI 2 "emex64_arith_operand" "r,n,r")))]
  ""
  "<shift_insn>\t%0, %1, %2")

(define_insn "rotrdi3"
  [(set (match_operand:DI 0 "register_operand" "=r,r")
	(rotatert:DI (match_operand:DI 1 "register_operand" "0,0")
		     (match_operand:DI 2 "emex64_arith_operand" "r,n")))]
  ""
  "ror\t%0, %2")

;; rotate left: ror by the negated count (ror takes the count mod 64).
;; Without it the generic expansion subtracts the count in its own (int)
;; mode, which has no pattern here, and crashes.
(define_expand "rotldi3"
  [(set (match_operand:DI 0 "register_operand")
	(rotate:DI (match_operand:DI 1 "register_operand")
		   (match_operand:DI 2 "emex64_arith_operand")))]
  ""
{
  rtx n = operands[2];
  if (CONST_INT_P (n))
    n = GEN_INT ((64 - (INTVAL (n) & 63)) & 63);
  else
    {
      n = gen_reg_rtx (DImode);
      emit_insn (gen_negdi2 (n, operands[2]));
    }
  emit_insn (gen_rotrdi3 (operands[0], operands[1], n));
  DONE;
})

(define_insn "bswapdi2"
  [(set (match_operand:DI 0 "register_operand" "=r")
	(bswap:DI (match_operand:DI 1 "register_operand" "0")))]
  ""
  "bswapq\t%0")

;; ISA v19
(define_insn "clzdi2"
  [(set (match_operand:DI 0 "register_operand" "=r")
	(clz:DI (match_operand:DI 1 "register_operand" "r")))]
  "TARGET_ISA19"
  "clz\t%0, %1")

(define_insn "ctzdi2"
  [(set (match_operand:DI 0 "register_operand" "=r")
	(ctz:DI (match_operand:DI 1 "register_operand" "r")))]
  "TARGET_ISA19"
  "ctz\t%0, %1")

(define_insn "popcountdi2"
  [(set (match_operand:DI 0 "register_operand" "=r")
	(popcount:DI (match_operand:DI 1 "register_operand" "r")))]
  "TARGET_ISA19"
  "popcnt\t%0, %1")

(define_insn "smuldi3_highpart"
  [(set (match_operand:DI 0 "register_operand" "=r")
	(truncate:DI
	  (lshiftrt:TI
	    (mult:TI (sign_extend:TI (match_operand:DI 1 "register_operand" "%r"))
		     (sign_extend:TI (match_operand:DI 2 "register_operand" "r")))
	    (const_int 64))))]
  "TARGET_ISA19"
  "mulh\t%0, %1, %2")

(define_insn "umuldi3_highpart"
  [(set (match_operand:DI 0 "register_operand" "=r")
	(truncate:DI
	  (lshiftrt:TI
	    (mult:TI (zero_extend:TI (match_operand:DI 1 "register_operand" "%r"))
		     (zero_extend:TI (match_operand:DI 2 "register_operand" "r")))
	    (const_int 64))))]
  "TARGET_ISA19"
  "umulh\t%0, %1, %2")

(define_expand "<u>mulditi3"
  [(set (match_operand:TI 0 "register_operand")
	(mult:TI (any_extend:TI (match_operand:DI 1 "register_operand"))
		 (any_extend:TI (match_operand:DI 2 "register_operand"))))]
  "TARGET_ISA19"
{
  rtx lo = gen_reg_rtx (DImode);
  rtx hi = gen_reg_rtx (DImode);
  emit_insn (gen_muldi3 (lo, operands[1], operands[2]));
  emit_insn (gen_<su>muldi3_highpart (hi, operands[1], operands[2]));
  emit_move_insn (gen_lowpart (DImode, operands[0]), lo);
  emit_move_insn (gen_highpart (DImode, operands[0]), hi);
  DONE;
})

;; -------------------------------------------------------------------------
;; Double word arithmetic (ISA v21)
;; -------------------------------------------------------------------------

;; addd, subd, muld, shld: the 64-bit instruction, its result cut to 32 bits
;; and zero extended; iaddd, isubd, imuld, ishld sign extended.  shrd, divd
;; and modd zero extend, sard and idivd sign extend.  A shift's count is
;; taken mod 64, as by shl/shr/sar.  That is an int operation and the
;; extension the middle end puts after it, in one instruction: these are
;; what combine makes of the two (in the shapes it may leave them in).

(define_code_iterator dw_ext [zero_extend sign_extend])
(define_code_attr dwi [(zero_extend "") (sign_extend "i")])
(define_code_iterator dw_op [plus minus mult])
(define_code_attr dw_insn [(plus "add") (minus "sub") (mult "mul")])

;; the extension of an operation in SImode
(define_insn "*<dwi><dw_insn>d"
  [(set (match_operand:DI 0 "register_operand" "=r,r")
	(dw_ext:DI
	  (dw_op:SI (match_operand:SI 1 "register_operand" "r,r")
		    (match_operand:SI 2 "emex64_arith_operand" "r,n"))))]
  "TARGET_ISA21"
{
  if (<dw_op:CODE> == PLUS && emex64_dw_neg_imm_p (operands[2]))
    return "<dwi>subd\t%0, %1, %n2";
  return "<dwi><dw_insn>d\t%0, %1, %d2";
})

;; a 64-bit operation with its high half masked off
(define_insn "*<dw_insn>d_and"
  [(set (match_operand:DI 0 "register_operand" "=r,r")
	(and:DI
	  (dw_op:DI (match_operand:DI 1 "register_operand" "r,r")
		    (match_operand:DI 2 "emex64_arith_operand" "r,n"))
	  (const_int 4294967295)))]
  "TARGET_ISA21"
{
  if (<dw_op:CODE> == PLUS && emex64_dw_neg_imm_p (operands[2]))
    return "subd\t%0, %1, %n2";
  return "<dw_insn>d\t%0, %1, %d2";
})

;; a 64-bit operation whose low half is sign extended
(define_insn "*i<dw_insn>d_subreg"
  [(set (match_operand:DI 0 "register_operand" "=r,r")
	(sign_extend:DI
	  (subreg:SI
	    (dw_op:DI (match_operand:DI 1 "register_operand" "r,r")
		      (match_operand:DI 2 "emex64_arith_operand" "r,n")) 0)))]
  "TARGET_ISA21"
{
  if (<dw_op:CODE> == PLUS && emex64_dw_neg_imm_p (operands[2]))
    return "isubd\t%0, %1, %n2";
  return "i<dw_insn>d\t%0, %1, %d2";
})

;; shift left
(define_insn "*<dwi>shld_<mode>"
  [(set (match_operand:DI 0 "register_operand" "=r,r")
	(dw_ext:DI
	  (ashift:SI (match_operand:SI 1 "register_operand" "r,r")
		     (match_operand:SD 2 "emex64_arith_operand" "r,n"))))]
  "TARGET_ISA21"
  "<dwi>shld\t%0, %1, %2")

(define_insn "*shld_and"
  [(set (match_operand:DI 0 "register_operand" "=r,r")
	(and:DI
	  (ashift:DI (match_operand:DI 1 "register_operand" "r,r")
		     (match_operand:DI 2 "emex64_arith_operand" "r,n"))
	  (const_int 4294967295)))]
  "TARGET_ISA21"
  "shld\t%0, %1, %2")

(define_insn "*ishld_subreg"
  [(set (match_operand:DI 0 "register_operand" "=r,r")
	(sign_extend:DI
	  (subreg:SI
	    (ashift:DI (match_operand:DI 1 "register_operand" "r,r")
		       (match_operand:DI 2 "emex64_arith_operand" "r,n")) 0)))]
  "TARGET_ISA21"
  "ishld\t%0, %1, %2")

;; shift right: of the zero extended (shrd) or sign extended (sard) int
(define_insn "*shrd_<mode>"
  [(set (match_operand:DI 0 "register_operand" "=r,r")
	(zero_extend:DI
	  (lshiftrt:SI (match_operand:SI 1 "register_operand" "r,r")
		       (match_operand:SD 2 "emex64_arith_operand" "r,n"))))]
  "TARGET_ISA21"
  "shrd\t%0, %1, %2")

(define_insn "*shrd_zext"
  [(set (match_operand:DI 0 "register_operand" "=r,r")
	(lshiftrt:DI
	  (zero_extend:DI (match_operand:SI 1 "register_operand" "r,r"))
	  (match_operand:DI 2 "emex64_arith_operand" "r,n")))]
  "TARGET_ISA21"
  "shrd\t%0, %1, %2")

(define_insn "*sard_<mode>"
  [(set (match_operand:DI 0 "register_operand" "=r,r")
	(sign_extend:DI
	  (ashiftrt:SI (match_operand:SI 1 "register_operand" "r,r")
		       (match_operand:SD 2 "emex64_arith_operand" "r,n"))))]
  "TARGET_ISA21"
  "sard\t%0, %1, %2")

(define_insn "*sard_sext"
  [(set (match_operand:DI 0 "register_operand" "=r,r")
	(ashiftrt:DI
	  (sign_extend:DI (match_operand:SI 1 "register_operand" "r,r"))
	  (match_operand:DI 2 "emex64_arith_operand" "r,n")))]
  "TARGET_ISA21"
  "sard\t%0, %1, %2")

;; division: of two zero extended ints the 64-bit result is the 32-bit one;
;; of two sign extended ones it is not (the smallest by -1), so idivd is
;; only the division in SImode with its own extension
(define_code_iterator dw_udiv [udiv umod])
(define_code_attr dw_udiv_insn [(udiv "divd") (umod "modd")])

(define_insn "*<dw_udiv_insn>_zext"
  [(set (match_operand:DI 0 "register_operand" "=r")
	(dw_udiv:DI
	  (zero_extend:DI (match_operand:SI 1 "register_operand" "r"))
	  (zero_extend:DI (match_operand:SI 2 "register_operand" "r"))))]
  "TARGET_ISA21"
  "<dw_udiv_insn>\t%0, %1, %2")

(define_insn "*<dw_udiv_insn>"
  [(set (match_operand:DI 0 "register_operand" "=r,r")
	(zero_extend:DI
	  (dw_udiv:SI (match_operand:SI 1 "register_operand" "r,r")
		      (match_operand:SI 2 "emex64_arith_operand" "r,n"))))]
  "TARGET_ISA21"
  "<dw_udiv_insn>\t%0, %1, %d2")

(define_insn "*idivd"
  [(set (match_operand:DI 0 "register_operand" "=r,r")
	(sign_extend:DI
	  (div:SI (match_operand:SI 1 "register_operand" "r,r")
		  (match_operand:SI 2 "emex64_arith_operand" "r,n"))))]
  "TARGET_ISA21"
  "idivd\t%0, %1, %d2")

;; 128-bit add and subtract: C clear, then adc/sbb on each half
(define_insn "addti3"
  [(set (match_operand:TI 0 "register_operand" "=&r")
	(plus:TI (match_operand:TI 1 "register_operand" "%r")
		 (match_operand:TI 2 "register_operand" "r")))
   (clobber (reg:CC CF_REGNUM))]
  "TARGET_ISA19"
  "mov\tcf, 0\;adc\t%0, %1, %2\;adc\t%H0, %H1, %H2")

(define_insn "subti3"
  [(set (match_operand:TI 0 "register_operand" "=&r")
	(minus:TI (match_operand:TI 1 "register_operand" "r")
		  (match_operand:TI 2 "register_operand" "r")))
   (clobber (reg:CC CF_REGNUM))]
  "TARGET_ISA19"
  "mov\tcf, 0\;sbb\t%0, %1, %2\;sbb\t%H0, %H1, %H2")

;; -------------------------------------------------------------------------
;; Floating point (ISA v18)
;; -------------------------------------------------------------------------

(define_code_iterator fbinop [plus minus mult div smin smax])
(define_code_attr fbin_insn [(plus "fadd") (minus "fsub") (mult "fmul")
			     (div "fdiv") (smin "fmin") (smax "fmax")])
(define_code_attr fbin_name [(plus "add") (minus "sub") (mult "mul")
			     (div "div") (smin "smin") (smax "smax")])

(define_insn "<fbin_name><mode>3"
  [(set (match_operand:ANYF 0 "register_operand" "=f")
	(fbinop:ANYF (match_operand:ANYF 1 "register_operand" "f")
		     (match_operand:ANYF 2 "register_operand" "f")))]
  "TARGET_HARD_FLOAT"
  "<fbin_insn><fs>\t%0, %1, %2")

(define_insn "fmin<mode>3"
  [(set (match_operand:ANYF 0 "register_operand" "=f")
	(unspec:ANYF [(match_operand:ANYF 1 "register_operand" "f")
		      (match_operand:ANYF 2 "register_operand" "f")]
		     UNSPEC_FMIN))]
  "TARGET_HARD_FLOAT"
  "fmin<fs>\t%0, %1, %2")

(define_insn "fmax<mode>3"
  [(set (match_operand:ANYF 0 "register_operand" "=f")
	(unspec:ANYF [(match_operand:ANYF 1 "register_operand" "f")
		      (match_operand:ANYF 2 "register_operand" "f")]
		     UNSPEC_FMAX))]
  "TARGET_HARD_FLOAT"
  "fmax<fs>\t%0, %1, %2")

(define_insn "fma<mode>4"
  [(set (match_operand:ANYF 0 "register_operand" "=f")
	(fma:ANYF (match_operand:ANYF 1 "register_operand" "f")
		  (match_operand:ANYF 2 "register_operand" "f")
		  (match_operand:ANYF 3 "register_operand" "f")))]
  "TARGET_HARD_FLOAT"
  "fmadd<fs>\t%0, %1, %2, %3")

(define_code_iterator funop [sqrt neg abs])
(define_code_attr fun_insn [(sqrt "fsqrt") (neg "fneg") (abs "fabs")])

(define_insn "<code><mode>2"
  [(set (match_operand:ANYF 0 "register_operand" "=f")
	(funop:ANYF (match_operand:ANYF 1 "register_operand" "f")))]
  "TARGET_HARD_FLOAT"
  "<fun_insn><fs>\t%0, %1")

(define_insn "extendsfdf2"
  [(set (match_operand:DF 0 "register_operand" "=f")
	(float_extend:DF (match_operand:SF 1 "register_operand" "f")))]
  "TARGET_HARD_FLOAT"
  "fcvtds\t%0, %1")

(define_insn "truncdfsf2"
  [(set (match_operand:SF 0 "register_operand" "=f")
	(float_truncate:SF (match_operand:DF 1 "register_operand" "f")))]
  "TARGET_HARD_FLOAT"
  "fcvtsd\t%0, %1")

(define_insn "floatdi<mode>2"
  [(set (match_operand:ANYF 0 "register_operand" "=f")
	(float:ANYF (match_operand:DI 1 "register_operand" "r")))]
  "TARGET_HARD_FLOAT"
  "fcvt<fcvt>i\t%0, %1")

(define_insn "floatunsdi<mode>2"
  [(set (match_operand:ANYF 0 "register_operand" "=f")
	(unsigned_float:ANYF (match_operand:DI 1 "register_operand" "r")))]
  "TARGET_HARD_FLOAT"
  "fcvt<fcvt>u\t%0, %1")

(define_insn "fix_trunc<mode>di2"
  [(set (match_operand:DI 0 "register_operand" "=r")
	(fix:DI (match_operand:ANYF 1 "register_operand" "f")))]
  "TARGET_HARD_FLOAT"
  "fcvti<fcvt>\t%0, %1")

(define_insn "fixuns_trunc<mode>di2"
  [(set (match_operand:DI 0 "register_operand" "=r")
	(unsigned_fix:DI (match_operand:ANYF 1 "register_operand" "f")))]
  "TARGET_HARD_FLOAT"
  "fcvtu<fcvt>\t%0, %1")

;; -------------------------------------------------------------------------
;; Compares and branches
;; -------------------------------------------------------------------------

(define_insn "*cmpdi"
  [(set (reg:CC CF_REGNUM)
	(compare:CC (match_operand:DI 0 "emex64_arith_operand" "r,r,n")
		    (match_operand:DI 1 "emex64_arith_operand" "r,n,r")))]
  ""
  "cmp\t%0, %1")

(define_insn "*cmp<mode>"
  [(set (reg:CCFP CF_REGNUM)
	(compare:CCFP (match_operand:ANYF 0 "register_operand" "f")
		      (match_operand:ANYF 1 "register_operand" "f")))]
  "TARGET_HARD_FLOAT"
  "fcmp<fs>\t%0, %1")

(define_expand "cbranchdi4"
  [(set (pc)
	(if_then_else (match_operator 0 "comparison_operator"
			[(match_operand:DI 1 "emex64_arith_operand")
			 (match_operand:DI 2 "emex64_arith_operand")])
		      (label_ref (match_operand 3 ""))
		      (pc)))]
  ""
{
  emex64_expand_cbranch (operands, DImode);
  DONE;
})

(define_expand "cbranch<mode>4"
  [(set (pc)
	(if_then_else (match_operator 0 "comparison_operator"
			[(match_operand:ANYF 1 "register_operand")
			 (match_operand:ANYF 2 "register_operand")])
		      (label_ref (match_operand 3 ""))
		      (pc)))]
  "TARGET_HARD_FLOAT"
{
  emex64_expand_cbranch (operands, <MODE>mode);
  DONE;
})

;; branch on cf (be, bne, blt, bgt, ble, bge; bbnz/bbz cf, mask for the
;; unordered conditions)
(define_insn "*branch_cf"
  [(set (pc)
	(if_then_else (match_operator 0 "emex64_cf_comparison"
			[(match_operand 1 "emex64_cf_operand" "")
			 (const_int 0)])
		      (label_ref (match_operand 2 "" ""))
		      (pc)))]
  ""
{
  return emex64_output_cf_branch (operands[0], operands[2], false);
})

(define_insn "*branch_cf_reversed"
  [(set (pc)
	(if_then_else (match_operator 0 "emex64_cf_comparison"
			[(match_operand 1 "emex64_cf_operand" "")
			 (const_int 0)])
		      (pc)
		      (label_ref (match_operand 2 "" ""))))]
  ""
{
  return emex64_output_cf_branch (operands[0], operands[2], true);
})

;; bz / bnz reg, label
(define_insn "*branch_zero"
  [(set (pc)
	(if_then_else (match_operator 0 "emex64_eq_ne_operator"
			[(match_operand:DI 1 "register_operand" "r")
			 (const_int 0)])
		      (label_ref (match_operand 2 "" ""))
		      (pc)))]
  ""
{
  return GET_CODE (operands[0]) == EQ ? "bz\t%1, %l2@pcrel" : "bnz\t%1, %l2@pcrel";
})

(define_insn "*branch_zero_reversed"
  [(set (pc)
	(if_then_else (match_operator 0 "emex64_eq_ne_operator"
			[(match_operand:DI 1 "register_operand" "r")
			 (const_int 0)])
		      (pc)
		      (label_ref (match_operand 2 "" ""))))]
  ""
{
  return GET_CODE (operands[0]) == EQ ? "bnz\t%1, %l2@pcrel" : "bz\t%1, %l2@pcrel";
})

;; bbz / bbnz reg, mask, label
(define_insn "*branch_mask"
  [(set (pc)
	(if_then_else (match_operator 0 "emex64_eq_ne_operator"
			[(and:DI (match_operand:DI 1 "register_operand" "r")
				 (match_operand:DI 2 "const_int_operand" "n"))
			 (const_int 0)])
		      (label_ref (match_operand 3 "" ""))
		      (pc)))]
  ""
{
  return GET_CODE (operands[0]) == EQ ? "bbz\t%1, %2, %l3@pcrel"
				      : "bbnz\t%1, %2, %l3@pcrel";
})

(define_insn "*branch_mask_reversed"
  [(set (pc)
	(if_then_else (match_operator 0 "emex64_eq_ne_operator"
			[(and:DI (match_operand:DI 1 "register_operand" "r")
				 (match_operand:DI 2 "const_int_operand" "n"))
			 (const_int 0)])
		      (pc)
		      (label_ref (match_operand 3 "" ""))))]
  ""
{
  return GET_CODE (operands[0]) == EQ ? "bbnz\t%1, %2, %l3@pcrel"
				      : "bbz\t%1, %2, %l3@pcrel";
})

(define_insn "*branch_bit"
  [(set (pc)
	(if_then_else (match_operator 0 "emex64_eq_ne_operator"
			[(zero_extract:DI (match_operand:DI 1 "register_operand" "r")
					  (const_int 1)
					  (match_operand:DI 2 "const_int_operand" "n"))
			 (const_int 0)])
		      (label_ref (match_operand 3 "" ""))
		      (pc)))]
  "UINTVAL (operands[2]) < 64"
{
  operands[2] = GEN_INT (HOST_WIDE_INT_1U << UINTVAL (operands[2]));
  return GET_CODE (operands[0]) == EQ ? "bbz\t%1, %2, %l3@pcrel"
				      : "bbnz\t%1, %2, %l3@pcrel";
})

(define_insn "*branch_bit_reversed"
  [(set (pc)
	(if_then_else (match_operator 0 "emex64_eq_ne_operator"
			[(zero_extract:DI (match_operand:DI 1 "register_operand" "r")
					  (const_int 1)
					  (match_operand:DI 2 "const_int_operand" "n"))
			 (const_int 0)])
		      (pc)
		      (label_ref (match_operand 3 "" ""))))]
  "UINTVAL (operands[2]) < 64"
{
  operands[2] = GEN_INT (HOST_WIDE_INT_1U << UINTVAL (operands[2]));
  return GET_CODE (operands[0]) == EQ ? "bbnz\t%1, %2, %l3@pcrel"
				      : "bbz\t%1, %2, %l3@pcrel";
})

;; compare and branch on any integer condition (v20), cf untouched; each
;; has its inverse, so a reversed one is a single instruction too
(define_insn "*branch_cb"
  [(set (pc)
	(if_then_else (match_operator 0 "emex64_int_comparison_operator"
			[(match_operand:DI 1 "emex64_arith_operand" "r,r,n")
			 (match_operand:DI 2 "emex64_arith_operand" "r,n,r")])
		      (label_ref (match_operand 3 "" ""))
		      (pc)))]
  "TARGET_ISA20"
{
  return emex64_output_cbranch (operands, false);
})

(define_insn "*branch_cb_reversed"
  [(set (pc)
	(if_then_else (match_operator 0 "emex64_int_comparison_operator"
			[(match_operand:DI 1 "emex64_arith_operand" "r,r,n")
			 (match_operand:DI 2 "emex64_arith_operand" "r,n,r")])
		      (pc)
		      (label_ref (match_operand 3 "" ""))))]
  "TARGET_ISA20"
{
  return emex64_output_cbranch (operands, true);
})

;; cblt a, b, label (v19): compare and branch, cf untouched.  Reversed it
;; becomes cmp and bge/ble, hence the clobber.
(define_insn "*branch_cblt"
  [(set (pc)
	(if_then_else (match_operator 0 "emex64_lt_gt_operator"
			[(match_operand:DI 1 "emex64_arith_operand" "r,r,n")
			 (match_operand:DI 2 "emex64_arith_operand" "r,n,r")])
		      (label_ref (match_operand 3 "" ""))
		      (pc)))
   (clobber (reg:CC CF_REGNUM))]
  "TARGET_ISA19"
{
  return GET_CODE (operands[0]) == LT ? "cblt\t%1, %2, %l3@pcrel"
				      : "cblt\t%2, %1, %l3@pcrel";
})

(define_insn "*branch_cblt_reversed"
  [(set (pc)
	(if_then_else (match_operator 0 "emex64_lt_gt_operator"
			[(match_operand:DI 1 "emex64_arith_operand" "r,r,n")
			 (match_operand:DI 2 "emex64_arith_operand" "r,n,r")])
		      (pc)
		      (label_ref (match_operand 3 "" ""))))
   (clobber (reg:CC CF_REGNUM))]
  "TARGET_ISA19"
{
  return GET_CODE (operands[0]) == LT ? "cmp\t%1, %2\;bge\t%l3@pcrel"
				      : "cmp\t%1, %2\;ble\t%l3@pcrel";
})

;; set a register to a condition (0 or 1) without branching, from cf
(define_expand "cstoredi4"
  [(set (match_operand:DI 0 "register_operand")
	(match_operator:DI 1 "comparison_operator"
	  [(match_operand:DI 2 "emex64_arith_operand")
	   (match_operand:DI 3 "emex64_arith_operand")]))]
  ""
{
  emex64_expand_cstore (operands, DImode);
  DONE;
})

(define_expand "cstore<mode>4"
  [(set (match_operand:DI 0 "register_operand")
	(match_operator:DI 1 "comparison_operator"
	  [(match_operand:ANYF 2 "register_operand")
	   (match_operand:ANYF 3 "register_operand")]))]
  "TARGET_HARD_FLOAT"
{
  emex64_expand_cstore (operands, <MODE>mode);
  DONE;
})

(define_insn "*scc"
  [(set (match_operand:DI 0 "register_operand" "=r")
	(match_operator:DI 1 "emex64_cf_comparison"
	  [(match_operand 2 "emex64_cf_operand" "")
	   (const_int 0)]))]
  ""
{
  return emex64_output_scc (operands);
})

;; -------------------------------------------------------------------------
;; Jumps
;; -------------------------------------------------------------------------

(define_insn "jump"
  [(set (pc) (label_ref (match_operand 0 "" "")))]
  ""
  "b\t%l0@pcrel")

(define_insn "indirect_jump"
  [(set (pc) (match_operand:DI 0 "register_operand" "r"))]
  ""
  "b\t%0")

;; Jump tables hold 32-bit offsets from the table; b [table + offset]
;; jumps to the sum.
(define_expand "tablejump"
  [(use (match_operand:SI 0 "register_operand"))
   (use (label_ref (match_operand 1 "")))]
  ""
{
  rtx off = gen_reg_rtx (DImode);
  rtx base = gen_reg_rtx (DImode);
  emit_insn (gen_extendsidi2 (off, operands[0]));
  emit_move_insn (base, gen_rtx_LABEL_REF (Pmode, operands[1]));
  emit_jump_insn (gen_tablejump_internal (base, off, operands[1]));
  DONE;
})

(define_insn "tablejump_internal"
  [(set (pc) (plus:DI (match_operand:DI 0 "register_operand" "r")
		      (match_operand:DI 1 "register_operand" "r")))
   (use (label_ref (match_operand 2 "" "")))]
  ""
  "b\t[%0 + %1]")

;; -------------------------------------------------------------------------
;; Calls
;; -------------------------------------------------------------------------

(define_expand "call"
  [(call (match_operand 0 "")
	 (match_operand 1 ""))]
  ""
{
  emex64_expand_call (NULL_RTX, operands[0], operands[1]);
  DONE;
})

(define_expand "call_value"
  [(set (match_operand 0 "")
	(call (match_operand 1 "")
	      (match_operand 2 "")))]
  ""
{
  emex64_expand_call (operands[0], operands[1], operands[2]);
  DONE;
})

(define_insn "*call_insn"
  [(call (mem:QI (match_operand:DI 0 "emex64_call_operand" "r,S"))
	 (match_operand 1 "" ""))]
  ""
  "@
   bl\t%0
   bl\t%0@plt")

(define_insn "*call_value_insn"
  [(set (match_operand 0 "" "")
	(call (mem:QI (match_operand:DI 1 "emex64_call_operand" "r,S"))
	      (match_operand 2 "" "")))]
  ""
  "@
   bl\t%1
   bl\t%1@plt")

;; -------------------------------------------------------------------------
;; Prologue, epilogue, returns
;; -------------------------------------------------------------------------

(define_expand "prologue"
  [(const_int 0)]
  ""
{
  emex64_expand_prologue ();
  DONE;
})

(define_expand "epilogue"
  [(const_int 0)]
  ""
{
  emex64_expand_epilogue ();
  DONE;
})

;; ret: sp = fp, pop fp and the return address.  reta also checks the
;; return address pacret signed.
(define_insn "emex64_return"
  [(return)]
  "reload_completed"
{
  return emex64_sign_return_address_p () ? "reta" : "ret";
})

;; pac-ret: sign the return address at [fp + 16], with fp as the modifier
(define_insn "emex64_pacret"
  [(set (mem:DI (plus:DI (reg:DI FP_REGNUM) (const_int 16)))
	(unspec_volatile:DI [(mem:DI (plus:DI (reg:DI FP_REGNUM) (const_int 16)))
			     (reg:DI FP_REGNUM)]
			    UNSPECV_PACRET))]
  ""
  "pacret")

(define_insn "emex64_xpac"
  [(set (match_operand:DI 0 "register_operand" "=r")
	(unspec:DI [(match_operand:DI 1 "register_operand" "r")] UNSPEC_XPAC))]
  "TARGET_PAUTH"
  "xpac\t%0, %1")

;; __builtin_eh_return: the handler is in the return address slot, the
;; stack adjustment (from the CFA) in er5.  Load the target's fp from its
;; slot like ret would, set sp and jump.
(define_expand "eh_return"
  [(use (match_operand 0 "general_operand"))]
  ""
{
  emex64_expand_eh_return (operands[0]);
  DONE;
})

(define_insn "emex64_eh_return_internal"
  [(return)
   (use (reg:DI ER5_REGNUM))
   (clobber (reg:DI ER4_REGNUM))]
  "reload_completed"
  "ldq\ter4, [fp + 16]\;add\ter5, er5, fp\;add\ter5, er5, 16\;ldq\tfp, [fp + 8]\;mov\tsp, er5\;b\ter4")

(define_insn "blockage"
  [(unspec_volatile [(const_int 0)] UNSPECV_BLOCKAGE)]
  ""
  "")

(define_insn "nop"
  [(const_int 0)]
  ""
  "nop")

;; -------------------------------------------------------------------------
;; Block operations: cpym (memcpy), movm (memmove), setm (memset)
;; -------------------------------------------------------------------------

(define_expand "cpymemdi"
  [(use (match_operand:BLK 0 "memory_operand"))
   (use (match_operand:BLK 1 "memory_operand"))
   (use (match_operand:DI 2 "general_operand"))
   (use (match_operand:SI 3 "const_int_operand"))]
  ""
{
  rtx dst = force_reg (Pmode, XEXP (operands[0], 0));
  rtx src = force_reg (Pmode, XEXP (operands[1], 0));
  rtx len = force_reg (DImode, operands[2]);
  emit_insn (gen_emex64_cpym (replace_equiv_address (operands[0], dst),
			      replace_equiv_address (operands[1], src),
			      dst, src, len));
  DONE;
})

(define_insn "emex64_cpym"
  [(set (match_operand:BLK 0 "memory_operand" "=m")
	(unspec:BLK [(match_operand:BLK 1 "memory_operand" "m")
		     (match_operand:DI 2 "register_operand" "r")
		     (match_operand:DI 3 "register_operand" "r")
		     (match_operand:DI 4 "register_operand" "r")]
		    UNSPEC_CPYM))]
  ""
  "cpym\t%2, %3, %4")

(define_expand "movmemdi"
  [(use (match_operand:BLK 0 "memory_operand"))
   (use (match_operand:BLK 1 "memory_operand"))
   (use (match_operand:DI 2 "general_operand"))
   (use (match_operand:SI 3 "const_int_operand"))]
  ""
{
  rtx dst = force_reg (Pmode, XEXP (operands[0], 0));
  rtx src = force_reg (Pmode, XEXP (operands[1], 0));
  rtx len = force_reg (DImode, operands[2]);
  emit_insn (gen_emex64_movm (replace_equiv_address (operands[0], dst),
			      replace_equiv_address (operands[1], src),
			      dst, src, len));
  DONE;
})

(define_insn "emex64_movm"
  [(set (match_operand:BLK 0 "memory_operand" "=m")
	(unspec:BLK [(match_operand:BLK 1 "memory_operand" "m")
		     (match_operand:DI 2 "register_operand" "r")
		     (match_operand:DI 3 "register_operand" "r")
		     (match_operand:DI 4 "register_operand" "r")]
		    UNSPEC_MOVM))]
  ""
  "movm\t%2, %3, %4")

(define_expand "setmemdi"
  [(use (match_operand:BLK 0 "memory_operand"))
   (use (match_operand:DI 1 "general_operand"))
   (use (match_operand:QI 2 "nonmemory_operand"))
   (use (match_operand:SI 3 "const_int_operand"))]
  ""
{
  rtx dst = force_reg (Pmode, XEXP (operands[0], 0));
  rtx len = force_reg (DImode, operands[1]);
  rtx val = force_reg (DImode, convert_to_mode (DImode, operands[2], 1));
  emit_insn (gen_emex64_setm (replace_equiv_address (operands[0], dst),
			      dst, val, len));
  DONE;
})

(define_insn "emex64_setm"
  [(set (match_operand:BLK 0 "memory_operand" "=m")
	(unspec:BLK [(match_operand:DI 1 "register_operand" "r")
		     (match_operand:DI 2 "register_operand" "r")
		     (match_operand:DI 3 "register_operand" "r")]
		    UNSPEC_SETM))]
  ""
  "setm\t%1, %2, %3")

;; -------------------------------------------------------------------------
;; Atomics: cas and xchg on aligned 64-bit words; narrower ones are a
;; cas loop on the word around them
;; -------------------------------------------------------------------------

(define_expand "mem_thread_fence"
  [(match_operand:SI 0 "const_int_operand")]
  ""
{
  if (!is_mm_relaxed (memmodel_from_int (INTVAL (operands[0]))))
    {
      rtx mem = gen_rtx_MEM (BLKmode, gen_rtx_SCRATCH (Pmode));
      MEM_VOLATILE_P (mem) = 1;
      emit_insn (gen_emex64_fence (mem));
    }
  DONE;
})

;; a host atomic, so a full barrier; [sp] is a free slot
(define_insn "emex64_fence"
  [(set (match_operand:BLK 0 "" "")
	(unspec:BLK [(match_dup 0)] UNSPEC_FENCE))
   (clobber (reg:CC CF_REGNUM))]
  ""
  "xchg\tcf, [sp]")

(define_expand "atomic_compare_and_swap<mode>"
  [(match_operand:SI 0 "register_operand")	;; bool out
   (match_operand:QHSD 1 "register_operand")	;; old value out
   (match_operand:QHSD 2 "memory_operand")	;; memory
   (match_operand:QHSD 3 "emex64_reg_or_const_int") ;; expected
   (match_operand:QHSD 4 "emex64_reg_or_const_int") ;; desired
   (match_operand:SI 5 "const_int_operand")	;; is_weak
   (match_operand:SI 6 "const_int_operand")	;; success model
   (match_operand:SI 7 "const_int_operand")]	;; failure model
  ""
{
  emex64_expand_atomic_cas (operands);
  DONE;
})

;; cas old, [addr], new: if [addr] == old, [addr] = new and cf = Z; old
;; gets what was there
(define_insn "emex64_cas"
  [(set (match_operand:DI 0 "register_operand" "=r")
	(match_operand:DI 1 "memory_operand" "+Q"))
   (set (match_dup 1)
	(unspec_volatile:DI [(match_operand:DI 2 "register_operand" "0")
			     (match_operand:DI 3 "register_operand" "r")]
			    UNSPECV_CAS))
   (set (reg:CC CF_REGNUM)
	(unspec_volatile:CC [(const_int 0)] UNSPECV_CAS))]
  ""
  "cas\t%0, %1, %3")

;; ldadd/ldand/ldor/ldxor d, [addr], v (v20; on double words ldaddd, ...):
;; d = [addr], [addr] = d op v, a full barrier like cas and xchg
(define_code_iterator any_atomic [plus and ior xor])
(define_code_attr atomic_optab [(plus "add") (and "and") (ior "or") (xor "xor")])
(define_mode_attr amo [(SI "d") (DI "")])

(define_insn "atomic_fetch_<atomic_optab><mode>"
  [(set (match_operand:SD 0 "register_operand" "=r")
	(match_operand:SD 1 "memory_operand" "+Q"))
   (set (match_dup 1)
	(unspec_volatile:SD
	  [(any_atomic:SD (match_dup 1)
			  (match_operand:SD 2 "register_operand" "r"))
	   (match_operand:SI 3 "const_int_operand" "")]
	  UNSPECV_LDOP))]
  "TARGET_ISA20"
  "ld<atomic_optab><amo>\t%0, %1, %2")

;; x - v is x + -v
(define_expand "atomic_fetch_sub<mode>"
  [(match_operand:SD 0 "register_operand")
   (match_operand:SD 1 "memory_operand")
   (match_operand:SD 2 "register_operand")
   (match_operand:SI 3 "const_int_operand")]
  "TARGET_ISA20"
{
  rtx neg = expand_simple_unop (<MODE>mode, NEG, operands[2], NULL_RTX, 0);
  neg = force_reg (<MODE>mode, neg);
  emit_insn (gen_atomic_fetch_add<mode> (operands[0], operands[1], neg,
					 operands[3]));
  DONE;
})

(define_insn "atomic_exchangedi"
  [(set (match_operand:DI 0 "register_operand" "=r")
	(match_operand:DI 1 "memory_operand" "+Q"))
   (set (match_dup 1)
	(unspec_volatile:DI [(match_operand:DI 2 "register_operand" "0")
			     (match_operand:SI 3 "const_int_operand" "")]
			    UNSPECV_XCHG))]
  ""
  "xchg\t%0, %1")
