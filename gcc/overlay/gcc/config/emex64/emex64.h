/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2026 emexlab
 */

#ifndef GCC_EMEX64_H
#define GCC_EMEX64_H

#define EMEX64_ISA_LEVEL                                                                    \
          (emex64_cpu == EMEX64_CPU_V17 ? 17                                                \
        :  emex64_cpu == EMEX64_CPU_V18 ? 18                                                \
        :  emex64_cpu == EMEX64_CPU_V19 || emex64_cpu == EMEX64_CPU_EMEX64E ? 19            \
        : (emex64_cpu == EMEX64_CPU_V20 || emex64_cpu == EMEX64_CPU_EMEX64E_V20) ? 20 : 21)

#define TARGET_FPU      (EMEX64_ISA_LEVEL >= 18)
#define TARGET_ISA19    (EMEX64_ISA_LEVEL >= 19)
#define TARGET_ISA20    (EMEX64_ISA_LEVEL >= 20)
#define TARGET_ISA21    (EMEX64_ISA_LEVEL >= 21)
#define TARGET_PAUTH    (emex64_cpu == EMEX64_CPU_GENERIC       \
                      || emex64_cpu == EMEX64_CPU_EMEX64E       \
                      || emex64_cpu == EMEX64_CPU_EMEX64E_V20   \
                      || emex64_cpu == EMEX64_CPU_EMEX64E_V21)
#define TARGET_HARD_FLOAT (!TARGET_SOFT_FLOAT)

#ifndef EMEX64_CPU_DEFAULT
#define EMEX64_CPU_DEFAULT EMEX64_CPU_GENERIC
#endif /* EMEX64_CPU_DEFAULT */

#ifndef EMEX64E_TRIPLE
#define EMEX64E_TRIPLE 0
#endif /* EMEX64E_TRIPLE */

#define TARGET_CPU_CPP_BUILTINS()                                           \
    do {                                                                    \
        builtin_define("__emex64__");                                       \
        builtin_define("__EMEX64__");                                       \
        builtin_define("__la64__");                                         \
        builtin_assert("cpu=emex64");                                       \
        builtin_assert("machine=emex64");                                   \
        builtin_define_with_int_value("__emex64_isa__", EMEX64_ISA_LEVEL);  \
        if(TARGET_PAUTH)                                                    \
        {                                                                   \
            builtin_define("__emex64_pauth__");                             \
        }                                                                   \
        if(EMEX64E_TRIPLE)                                                  \
        {                                                                   \
            builtin_define("__emex64e__");                                  \
            builtin_define("__la64e__");                                    \
        }                                                                   \
        if(TARGET_HARD_FLOAT)                                               \
        {                                                                   \
            builtin_define("__emex64_fpu__");                               \
        }                                                                   \
        else                                                                \
        {                                                                   \
            builtin_define("__emex64_soft_float__");                        \
            builtin_define("__SOFTFP__");                                   \
        }                                                                   \
        builtin_define("__REGISTER_PREFIX__=");                             \
  } while(0)

#define TARGET_SUPPORTS_WIDE_INT 1

#define BITS_BIG_ENDIAN 0
#define BYTES_BIG_ENDIAN 0
#define WORDS_BIG_ENDIAN 0
#define UNITS_PER_WORD 8
#define MIN_UNITS_PER_WORD 8
#define POINTER_SIZE 64
#define Pmode DImode
#define FUNCTION_MODE QImode

#define PARM_BOUNDARY 64
#define STACK_BOUNDARY 64
#define PREFERRED_STACK_BOUNDARY 64
#define FUNCTION_BOUNDARY 8
#define BIGGEST_ALIGNMENT 128
#define EMPTY_FIELD_BOUNDARY 32
#define STRUCTURE_SIZE_BOUNDARY 8
#define STRICT_ALIGNMENT 0
#define MAX_FIXED_MODE_SIZE 128

#define SHORT_TYPE_SIZE 16
#define INT_TYPE_SIZE 32
#define LONG_TYPE_SIZE 64
#define LONG_LONG_TYPE_SIZE 64
#define DEFAULT_SIGNED_CHAR 1

#undef SIZE_TYPE
#define SIZE_TYPE "long unsigned int"
#undef PTRDIFF_TYPE
#define PTRDIFF_TYPE "long int"
#undef WCHAR_TYPE
#define WCHAR_TYPE "int"
#undef WCHAR_TYPE_SIZE
#define WCHAR_TYPE_SIZE 32

#define PROMOTE_MODE(MODE, UNSIGNEDP, TYPE)                             \
  do                                                                    \
  {                                                                     \
    if(GET_MODE_CLASS(MODE) == MODE_INT && GET_MODE_SIZE(MODE) < 4)     \
    {                                                                   \
      (MODE) = DImode;                                                  \
    }                                                                   \
  } while (0)

#define WORD_REGISTER_OPERATIONS 1
#define LOAD_EXTEND_OP(MODE) ZERO_EXTEND
#define SHORT_IMMEDIATES_SIGN_EXTEND 1
#define MOVE_MAX 8
#define SLOW_BYTE_ACCESS 0
#define CASE_VECTOR_MODE SImode
#define CASE_VECTOR_PC_RELATIVE 1
#define STORE_FLAG_VALUE 1
#define CLZ_DEFINED_VALUE_AT_ZERO(MODE, VALUE) ((MODE) == DImode ? ((VALUE) = 64, 2) : 0)
#define CTZ_DEFINED_VALUE_AT_ZERO(MODE, VALUE) ((MODE) == DImode ? ((VALUE) = 64, 2) : 0)

#define R0_REGNUM   0
#define R1_REGNUM	1
#define R5_REGNUM	5
#define R6_REGNUM	6
#define R9_REGNUM	9
#define RR_REGNUM	10
#define ER0_REGNUM	11
#define ER4_REGNUM	15
#define ER5_REGNUM	16
#define ER6_REGNUM	17
#define ER7_REGNUM	18
#define TP_REGNUM	26
#define SP_REGNUM	27
#define FP_REGNUM	28
#define CF_REGNUM	29
#define FR0_REGNUM	30
#define FR7_REGNUM	37
#define FR15_REGNUM	45
#define AP_REGNUM	46
#define SFP_REGNUM	47
#define FIRST_PSEUDO_REGISTER 48

#define GP_REG_P(R) ((unsigned)(R) <= FP_REGNUM)
#define FP_REG_P(R) ((unsigned)(R) - FR0_REGNUM <= FR15_REGNUM - FR0_REGNUM)

#define REGISTER_NAMES {                                        \
    "r0",  "r1",  "r2",  "r3",  "r4",  "r5",  "r6",  "r7",      \
    "r8",  "r9",  "rr",  "er0", "er1", "er2", "er3", "er4",     \
    "er5", "er6", "er7", "er8", "er9", "er10","er11","er12",    \
    "er13","er14","er15","sp",  "fp",  "cf",  "fr0", "fr1",     \
    "fr2", "fr3", "fr4", "fr5", "fr6", "fr7", "fr8", "fr9",     \
    "fr10","fr11","fr12","fr13","fr14","fr15","?ap", "?sfp" }

#define ADDITIONAL_REGISTER_NAMES { { "tp", TP_REGNUM } }

#define FIXED_REGISTERS {           \
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,   \
    0,                              \
    0, 0, 0, 0, 0, 0, 0, 0,         \
    0, 0, 0, 0, 0, 0, 0, 1,         \
    1, 1, 1,                        \
    0, 0, 0, 0, 0, 0, 0, 0,         \
    0, 0, 0, 0, 0, 0, 0, 0,         \
    1, 1 }

#define CALL_USED_REGISTERS {       \
    1, 1, 1, 1, 1, 1, 0, 0, 0, 0,   \
    1,                              \
    1, 1, 1, 1, 1, 1, 1, 1,         \
    0, 0, 0, 0, 0, 0, 0, 1,         \
    1, 1, 1,                        \
    1, 1, 1, 1, 1, 1, 1, 1,         \
    0, 0, 0, 0, 0, 0, 0, 0,         \
    1, 1 }

#define REG_ALLOC_ORDER {               \
    10, 11, 12, 13, 14, 15, 16, 17, 18, \
    5, 4, 3, 2, 1, 0,                   \
    6, 7, 8, 9,                         \
    19, 20, 21, 22, 23, 24, 25,         \
    30, 31, 32, 33, 34, 35, 36, 37,     \
    38, 39, 40, 41, 42, 43, 44, 45,     \
    26, 27, 28, 29, 46, 47 }

enum reg_class
{
    NO_REGS,
    GENERAL_REGS,
    FP_REGS,
    CC_REGS,
    ALL_REGS,
    LIM_REG_CLASSES
};

#define N_REG_CLASSES (int) LIM_REG_CLASSES
#define GENERAL_REGS GENERAL_REGS

#define REG_CLASS_NAMES \
    { "NO_REGS", "GENERAL_REGS", "FP_REGS", "CC_REGS", "ALL_REGS" }

#define REG_CLASS_CONTENTS {    \
    { 0x00000000, 0x00000000 }, \
    { 0x1fffffff, 0x0000c000 }, \
    { 0xc0000000, 0x00003fff }, \
    { 0x20000000, 0x00000000 }, \
    { 0xffffffff, 0x0000ffff } }

#define REGNO_REG_CLASS(R)                          \
    ((R) == CF_REGNUM ? CC_REGS                     \
    : FP_REG_P(R) ? FP_REGS                         \
    : (R) < FIRST_PSEUDO_REGISTER ? GENERAL_REGS    \
    : NO_REGS)

#define BASE_REG_CLASS GENERAL_REGS
#define INDEX_REG_CLASS GENERAL_REGS
#define REGNO_OK_FOR_BASE_P(R) emex64_regno_ok_for_base_p((R), true)
#define REGNO_OK_FOR_INDEX_P(R) emex64_regno_ok_for_base_p((R), true)
#define MAX_REGS_PER_ADDRESS 2

#define STACK_GROWS_DOWNWARD 1
#define FRAME_GROWS_DOWNWARD 1
#define STACK_POINTER_REGNUM SP_REGNUM
#define HARD_FRAME_POINTER_REGNUM FP_REGNUM
#define FRAME_POINTER_REGNUM SFP_REGNUM
#define ARG_POINTER_REGNUM AP_REGNUM
#define STATIC_CHAIN_REGNUM ER6_REGNUM
#define FIRST_PARM_OFFSET(FNDECL) 0
#define STACK_POINTER_OFFSET 8
#define ACCUMULATE_OUTGOING_ARGS 1
#define ARG_POINTER_CFA_OFFSET(FNDECL) (-8)

#define ELIMINABLE_REGS                                     \
    { { ARG_POINTER_REGNUM, HARD_FRAME_POINTER_REGNUM },    \
      { FRAME_POINTER_REGNUM, HARD_FRAME_POINTER_REGNUM } }

#define INITIAL_ELIMINATION_OFFSET(FROM, TO, OFFSET) \
  (OFFSET) = emex64_initial_elimination_offset((FROM), (TO))

#define DYNAMIC_CHAIN_ADDRESS(FRAME) plus_constant (Pmode,(FRAME), 8)
#define RETURN_ADDR_RTX(COUNT, FRAME) emex64_return_addr((COUNT), (FRAME))

#define INCOMING_RETURN_ADDR_RTX \
    gen_rtx_MEM(Pmode, plus_constant(Pmode, stack_pointer_rtx, 16))
#define INCOMING_FRAME_SP_OFFSET 16
#define DWARF_FRAME_RETURN_COLUMN 0
#define DWARF_FRAME_REGISTERS 48
#define DEBUGGER_REGNO(R)           \
    ((R) <= RR_REGNUM ? (R) + 5     \
    : (R) <= TP_REGNUM ? (R) + 5    \
    : (R) == SP_REGNUM ? 1          \
    : (R) == FP_REGNUM ? 2          \
    : (R) == CF_REGNUM ? 3          \
    : FP_REG_P (R) ? (R) + 2        \
    : INVALID_REGNUM)
#define DWARF_FRAME_REGNUM(R) DEBUGGER_REGNO (R)

#define EH_RETURN_DATA_REGNO(N) ((N) < 2 ? (unsigned) (N) : INVALID_REGNUM)
#define EH_RETURN_STACKADJ_RTX gen_rtx_REG (Pmode, ER5_REGNUM)
#define ASM_PREFERRED_EH_DATA_FORMAT(CODE, GLOBAL) \
    (((GLOBAL) ? DW_EH_PE_indirect : 0) | DW_EH_PE_pcrel | DW_EH_PE_sdata4)
#define EH_TABLES_CAN_BE_READ_ONLY 1

#define CUMULATIVE_ARGS struct emex64_args
struct emex64_args { int gprs; int fprs; };
#define INIT_CUMULATIVE_ARGS(CUM, FNTYPE, LIBNAME, FNDECL, N_NAMED_ARGS) \
    ((CUM).gprs = 0, (CUM).fprs = 0)
#define FUNCTION_ARG_REGNO_P(R) \
    ((R) <= R5_REGNUM || (TARGET_HARD_FLOAT && (R) >= FR0_REGNUM && (R) <= FR7_REGNUM))
#define EXIT_IGNORE_STACK 1
#define FUNCTION_PROFILER(FILE, LABELNO) \
    fprintf ((FILE), "\tbl\tmcount@plt\n")
#define NO_PROFILE_COUNTERS 1

#define TRAMPOLINE_SIZE 32
#define TRAMPOLINE_ALIGNMENT 64

#define BRANCH_COST(SPEED_P, PREDICTABLE_P) 2
#define LOGICAL_OP_NON_SHORT_CIRCUIT 0
#define NO_FUNCTION_CSE 1

#define SELECT_CC_MODE(OP, X, Y) \
    (GET_MODE_CLASS (GET_MODE (X)) == MODE_FLOAT ? CCFPmode : CCmode)
#define REVERSIBLE_CC_MODE(MODE) 1
#define REVERSE_CONDITION(CODE, MODE)                               \
    ((MODE) == CCFPmode ? reverse_condition_maybe_unordered (CODE)  \
    : reverse_condition (CODE))

#define ASM_COMMENT_START ";"
#undef ASM_APP_ON
#define ASM_APP_ON ""
#undef ASM_APP_OFF
#define ASM_APP_OFF ""
#define GLOBAL_ASM_OP "\t.globl\t"
#define TEXT_SECTION_ASM_OP "\t.text"
#define DATA_SECTION_ASM_OP "\t.data"
#undef BSS_SECTION_ASM_OP
#define BSS_SECTION_ASM_OP "\t.bss"
#undef LOCAL_LABEL_PREFIX
#define LOCAL_LABEL_PREFIX "."
#undef USER_LABEL_PREFIX
#define USER_LABEL_PREFIX ""

#undef ASM_OUTPUT_LABELREF
#define ASM_OUTPUT_LABELREF(FILE, NAME) emex64_output_labelref ((FILE), (NAME))

#undef ASM_OUTPUT_ALIGN
#define ASM_OUTPUT_ALIGN(FILE, LOG)                         \
  do {                                                      \
    if((LOG) != 0)                                          \
    {                                                       \
      fprintf((FILE), "\t.p2align\t%d\n", (int) (LOG));     \
    }                                                       \
  } while (0)

#define ASM_OUTPUT_ADDR_VEC_ELT(FILE, VALUE) \
    fprintf((FILE), "\t.quad\t.L%d\n", (VALUE))
#define ASM_OUTPUT_ADDR_DIFF_ELT(FILE, BODY, VALUE, REL) \
    fprintf((FILE), "\t.long\t.L%d-.L%d\n", (VALUE), (REL))
#define JUMP_TABLES_IN_TEXT_SECTION 0
#define DEFAULT_PCC_STRUCT_RETURN 0

#endif /* GCC_EMEX64_H */
