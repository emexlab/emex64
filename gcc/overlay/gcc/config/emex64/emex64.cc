/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2026 emexlab
 */

#define IN_TARGET_CODE 1

#include "config.h"
#include "system.h"
#include "coretypes.h"
#include "backend.h"
#include "target.h"
#include "rtl.h"
#include "tree.h"
#include "stringpool.h"
#include "attribs.h"
#include "df.h"
#include "regs.h"
#include "memmodel.h"
#include "emit-rtl.h"
#include "diagnostic-core.h"
#include "output.h"
#include "stor-layout.h"
#include "varasm.h"
#include "calls.h"
#include "expr.h"
#include "builtins.h"
#include "optabs.h"
#include "explow.h"
#include "alias.h"
#include "real.h"
#include "targhooks.h"
#include "insn-config.h"
#include "recog.h"
#include "tm_p.h"
#include "tm-constrs.h"
#include "opts.h"
#include "rtl-iter.h"
#include "cfgrtl.h"

#include "target-def.h"
static int emex64_pac_ret;

#ifndef EMEX64_DEFAULT_PAC_RET
#define EMEX64_DEFAULT_PAC_RET 0
#endif /* EMEX64_DEFAULT_PAC_RET */

struct GTY(()) machine_function
{
    HOST_WIDE_INT saved_size;
    HOST_WIDE_INT local_size;
    HOST_WIDE_INT outgoing_size;
    HOST_WIDE_INT args_copy_size;
    HOST_WIDE_INT total_size;
    bool sign_return;
};

static struct machine_function *emex64_init_machine_status(void)
{
    return ggc_cleared_alloc<machine_function> ();
}

/* -------------------------------------------------------------------------
   Options
   ------------------------------------------------------------------------- */

static void emex64_option_override(void)
{
    init_machine_status = emex64_init_machine_status;
    
    if(emex64_arch != EMEX64_CPU_NONE)
    {
        emex64_cpu = emex64_arch;
    }

    /* ISA v17 has no FPU */
    if(!TARGET_FPU)
    {
        target_flags |= MASK_SOFT_FLOAT;
    }

    emex64_pac_ret = EMEX64_DEFAULT_PAC_RET;
    if(emex64_branch_protection_string)
    {
        const char *s = emex64_branch_protection_string;
        if(!strcmp (s, "none"))
        {
            emex64_pac_ret = 0;
        }
        else if(!strcmp (s, "standard") || !strcmp (s, "pac-ret"))
        {
            emex64_pac_ret = 1;
        }
        else if(!strcmp (s, "pac-ret+leaf"))
        {
            emex64_pac_ret = 2;
        }
        else
        {
            error("invalid argument %qs to %<-mbranch-protection=%>", s);
        }
    }
}

static void emex64_conditional_register_usage(void)
{
    if(!TARGET_HARD_FLOAT)
    {
        for(unsigned r = FR0_REGNUM; r <= FR15_REGNUM; r++)
        {
            fixed_regs[r] = call_used_regs[r] = 1;
        }
    }
}

/* -------------------------------------------------------------------------
   Registers
   ------------------------------------------------------------------------- */

bool emex64_regno_ok_for_base_p(unsigned int regno,
                                bool strict)
{
    if(regno >= FIRST_PSEUDO_REGISTER)
    {
        if(!strict)
        {
            return true;
        }
        if(!reg_renumber || reg_renumber[regno] < 0)
        {
	        return false;
        }
        regno = reg_renumber[regno];
    }
    return GP_REG_P(regno) || regno == AP_REGNUM || regno == SFP_REGNUM;
}

static bool emex64_hard_regno_mode_ok(unsigned int regno,
                                      machine_mode mode)
{
    if(regno == CF_REGNUM)
    {
        return GET_MODE_CLASS(mode) == MODE_CC;
    }
    if(GET_MODE_CLASS(mode) == MODE_CC)
    {
        return false;
    }
    if(FP_REG_P(regno))
    {
        return mode == SFmode || mode == DFmode;
    }
    if(GET_MODE_SIZE(mode) > UNITS_PER_WORD)
    {
        return GET_MODE_SIZE(mode) <= 2 * UNITS_PER_WORD && regno < TP_REGNUM;
    }
    return true;
}

static bool emex64_modes_tieable_p(machine_mode m1,
                                   machine_mode m2)
{
    return(m1 == m2 || (GET_MODE_CLASS(m1) == MODE_INT && GET_MODE_CLASS(m2) == MODE_INT && GET_MODE_SIZE(m1) <= UNITS_PER_WORD && GET_MODE_SIZE(m2) <= UNITS_PER_WORD));
}

static bool emex64_can_change_mode_class(machine_mode from,
                                         machine_mode to,
                                         reg_class_t rclass)
{
    if(reg_classes_intersect_p(FP_REGS, rclass))
    {
        return known_eq (GET_MODE_SIZE(from), GET_MODE_SIZE(to));
    }
    return true;
}

static int emex64_register_move_cost(machine_mode,
                                     reg_class_t from,
                                     reg_class_t to)
{
    bool ffp = reg_class_subset_p(from, FP_REGS);
    bool tfp = reg_class_subset_p(to, FP_REGS);
    return ffp != tfp ? 4 : 2;
}

static int emex64_memory_move_cost(machine_mode,
                                   reg_class_t,
                                   bool)
{
    return 4;
}

/* -------------------------------------------------------------------------
   Frames
   ------------------------------------------------------------------------- */

static bool emex64_save_reg_p(unsigned int regno)
{
    if(crtl->calls_eh_return && (regno == R0_REGNUM || regno == R1_REGNUM))
    {
        return true;
    }
    return df_regs_ever_live_p(regno) && !call_used_or_fixed_reg_p(regno);
}

static void emex64_compute_frame_layout(void)
{
    machine_function *m = cfun->machine;
    HOST_WIDE_INT saved = 0;

    for(unsigned int r = 0; r < FIRST_PSEUDO_REGISTER; r++)
    {
        if(emex64_save_reg_p(r))
        {
            saved += UNITS_PER_WORD;
        }
    }

    m->saved_size = saved;
    m->local_size = ROUND_UP(get_frame_size().to_constant(), UNITS_PER_WORD);
    m->outgoing_size = ROUND_UP(crtl->outgoing_args_size.to_constant(), UNITS_PER_WORD);
    m->args_copy_size = (crtl->args.pretend_args_size ? ROUND_UP(crtl->args.size.to_constant(), UNITS_PER_WORD) : 0);
    HOST_WIDE_INT body = (m->saved_size + m->args_copy_size + m->local_size + m->outgoing_size);
    m->total_size = body ? body + UNITS_PER_WORD : 0;
    m->sign_return = (emex64_pac_ret && TARGET_PAUTH && !crtl->calls_eh_return && (emex64_pac_ret == 2 || !crtl->is_leaf || body != 0));
}

bool emex64_sign_return_address_p(void)
{
    return cfun->machine->sign_return;
}

HOST_WIDE_INT emex64_initial_elimination_offset(int from,
                                                int to)
{
    emex64_compute_frame_layout();
    gcc_assert(to == HARD_FRAME_POINTER_REGNUM);
    machine_function *m = cfun->machine;
    if(from == ARG_POINTER_REGNUM)
    {
        return m->args_copy_size ? -(m->saved_size + m->args_copy_size) : 24;
    }
    if(from == FRAME_POINTER_REGNUM)
    {
        return -(m->saved_size + m->args_copy_size);
    }
    gcc_unreachable();
}

static bool emex64_frame_pointer_required(void)
{
    return true;
}

static void emex64_frame_related(rtx_insn *insn,
                                 bool *first)
{
    RTX_FRAME_RELATED_P(insn) = 1;
    if(*first)
    {
        add_reg_note(insn, REG_CFA_DEF_CFA, plus_constant(Pmode, hard_frame_pointer_rtx, 16));
        *first = false;
    }
}

static machine_mode emex64_save_mode(unsigned int regno)
{
    return FP_REG_P(regno) ? (machine_mode)DFmode : (machine_mode)DImode;
}

void emex64_expand_prologue(void)
{
    emex64_compute_frame_layout();
    machine_function *m = cfun->machine;
    bool first = true;
    rtx_insn *insn;

    if(m->sign_return)
    {
        insn = emit_insn(gen_emex64_pacret ());
        emex64_frame_related(insn, &first);
    }

    if(m->total_size == 0)
    {
        return;
    }

    insn = emit_insn(gen_adddi3(stack_pointer_rtx, stack_pointer_rtx, GEN_INT(-m->total_size)));
    emex64_frame_related(insn, &first);

    HOST_WIDE_INT off = 0;
    for(unsigned int r = 0; r < FIRST_PSEUDO_REGISTER; r++)
    {
        if(emex64_save_reg_p(r))
        {
            off += UNITS_PER_WORD;
            machine_mode mode = emex64_save_mode(r);
            rtx mem = gen_frame_mem(mode, plus_constant(Pmode, hard_frame_pointer_rtx, -off));
            rtx reg = gen_rtx_REG(mode, r);
            insn = emit_move_insn(mem, reg);
            emex64_frame_related(insn, &first);
            add_reg_note(insn, REG_CFA_OFFSET, gen_rtx_SET(mem, reg));
        }
    }

    if(m->args_copy_size)
    {
        HOST_WIDE_INT base = -(m->saved_size + m->args_copy_size);
        HOST_WIDE_INT pretend = crtl->args.pretend_args_size;
        rtx tmp = gen_rtx_REG(DImode, RR_REGNUM);
        for(HOST_WIDE_INT i = 0; i + pretend < m->args_copy_size; i += UNITS_PER_WORD)
        {
            emit_move_insn(tmp, gen_frame_mem(DImode, plus_constant(Pmode, hard_frame_pointer_rtx, 24 + i)));
            emit_move_insn(gen_frame_mem(DImode, plus_constant(Pmode, hard_frame_pointer_rtx, base + pretend + i)), tmp);
        }
    }

    emit_insn(gen_blockage());
}

static void emex64_va_start(tree valist,
                            rtx nextarg)
{
    if(crtl->args.pretend_args_size)
    {
        nextarg = plus_constant(Pmode, hard_frame_pointer_rtx, 24 + crtl->args.size.to_constant() - crtl->args.pretend_args_size);
    }
    std_expand_builtin_va_start(valist, nextarg);
}

void emex64_expand_epilogue (void)
{
    emex64_compute_frame_layout();
    
    HOST_WIDE_INT off = 0;
    for(unsigned int r = 0; r < FIRST_PSEUDO_REGISTER; r++)
    {
        if(emex64_save_reg_p(r))
        {
            off += UNITS_PER_WORD;
            machine_mode mode = emex64_save_mode(r);
            rtx mem = gen_frame_mem(mode, plus_constant(Pmode, hard_frame_pointer_rtx, -off));
            emit_move_insn(gen_rtx_REG(mode, r), mem);
        }
    }

    if(crtl->calls_eh_return)
    {
        emit_jump_insn(gen_emex64_eh_return_internal());
    }
    else
    {
        emit_jump_insn(gen_emex64_return());
    }
}

void emex64_expand_eh_return(rtx handler)
{
    rtx slot = gen_rtx_MEM(Pmode, plus_constant(Pmode, hard_frame_pointer_rtx, 16));
    MEM_VOLATILE_P(slot) = 1;
    emit_move_insn(slot, force_reg(Pmode, handler));
}

rtx emex64_return_addr(int count,
                       rtx frame)
{
    rtx base = count == 0 ? hard_frame_pointer_rtx : frame;
    rtx ra = copy_to_reg(gen_rtx_MEM(Pmode, plus_constant(Pmode, base, 16)));
    if(TARGET_PAUTH)
    {
        rtx t = gen_reg_rtx(Pmode);
        emit_insn(gen_emex64_xpac(t, ra));
        ra = t;
    }
    return ra;
}

/* -------------------------------------------------------------------------
   Arguments and return values
   ------------------------------------------------------------------------- */

#define EMEX64_ARG_GPRS 6
#define EMEX64_ARG_FPRS 8

static bool emex64_fp_value_p(machine_mode mode,
                              const_tree type)
{
    if(!TARGET_HARD_FLOAT || (mode != SFmode && mode != DFmode))
    {
        return false;
    }
    return type ? SCALAR_FLOAT_TYPE_P(type) : true;
}

static HOST_WIDE_INT emex64_value_size(machine_mode mode,
                                       const_tree type)
{
    if(type)
    {
        return int_size_in_bytes (type);
    }
    return GET_MODE_SIZE(mode);
}

static rtx emex64_gpr_value(machine_mode mode,
                            const_tree type,
                            unsigned int regno,
                            unsigned int regno2)
{
    HOST_WIDE_INT size = emex64_value_size(mode, type);
    bool composite = (type && (AGGREGATE_TYPE_P(type) || TREE_CODE(type) == COMPLEX_TYPE));

    if(mode != BLKmode && size <= UNITS_PER_WORD && !COMPLEX_MODE_P(mode))
    {
        return gen_rtx_REG(mode, regno);
    }

    if(!composite && mode != BLKmode && size <= UNITS_PER_WORD)
    {
        return gen_rtx_REG(mode, regno);
    }

    int n = size > UNITS_PER_WORD ? 2 : 1;
    rtvec v = rtvec_alloc(n);
    RTVEC_ELT(v, 0) = gen_rtx_EXPR_LIST(VOIDmode, gen_rtx_REG(DImode, regno), const0_rtx);
    if(n == 2)
    {
        RTVEC_ELT(v, 1) = gen_rtx_EXPR_LIST(VOIDmode, gen_rtx_REG(DImode, regno2), GEN_INT(UNITS_PER_WORD));
    }
    return gen_rtx_PARALLEL(mode, v);
}

static int emex64_arg_words(const function_arg_info &arg)
{
    HOST_WIDE_INT size = emex64_value_size(arg.mode, arg.type);
    return (size + UNITS_PER_WORD - 1) / UNITS_PER_WORD;
}

static rtx emex64_function_arg(cumulative_args_t cum_v,
                               const function_arg_info &arg)
{
    CUMULATIVE_ARGS *cum = get_cumulative_args(cum_v);
    
    if(arg.end_marker_p () || !arg.named)
    {
        return NULL_RTX;
    }

    if(emex64_fp_value_p(arg.mode, arg.type))
    {
        if(cum->fprs < EMEX64_ARG_FPRS)
        {
            return gen_rtx_REG(arg.mode, FR0_REGNUM + cum->fprs);
        }
        return NULL_RTX;
    }

    int words = emex64_arg_words(arg);
    if(words == 0 || cum->gprs >= EMEX64_ARG_GPRS)
    {
        return NULL_RTX;
    }
    if(cum->gprs + words > EMEX64_ARG_GPRS)
    {
        return gen_rtx_REG (DImode, R0_REGNUM + cum->gprs);
    }

    return emex64_gpr_value (arg.mode, arg.type, R0_REGNUM + cum->gprs, R0_REGNUM + cum->gprs + 1);
}

static void emex64_function_arg_advance(cumulative_args_t cum_v,
                                        const function_arg_info &arg)
{
    CUMULATIVE_ARGS *cum = get_cumulative_args(cum_v);
    if(!arg.named)
    {
        return;
    }

    if(emex64_fp_value_p (arg.mode, arg.type))
    {
        if(cum->fprs < EMEX64_ARG_FPRS)
        {
            cum->fprs++;
        }
        return;
    }

    int words = emex64_arg_words (arg);
    if(cum->gprs + words > EMEX64_ARG_GPRS)
    {
        cum->gprs = EMEX64_ARG_GPRS;
    }
    else
    {
        cum->gprs += words;
    }
}

static int emex64_arg_partial_bytes(cumulative_args_t cum_v,
                                    const function_arg_info &arg)
{
    CUMULATIVE_ARGS *cum = get_cumulative_args(cum_v);
    if(!arg.named || emex64_fp_value_p(arg.mode, arg.type))
    {
        return 0;
    }
    int words = emex64_arg_words(arg);
    if(cum->gprs < EMEX64_ARG_GPRS && cum->gprs + words > EMEX64_ARG_GPRS)
    {
        return(EMEX64_ARG_GPRS - cum->gprs) * UNITS_PER_WORD;
    }
    return 0;
}

static unsigned int emex64_function_arg_boundary(machine_mode,
                                                 const_tree)
{
    return PARM_BOUNDARY;
}

static bool emex64_pass_by_reference(cumulative_args_t,
                                     const function_arg_info &arg)
{
    HOST_WIDE_INT size = emex64_value_size(arg.mode, arg.type);
    return size < 0 || size > 2 * UNITS_PER_WORD;
}

static bool emex64_return_in_memory(const_tree type,
                                    const_tree)
{
    HOST_WIDE_INT size = int_size_in_bytes(type);
    return size < 0 || size > 2 * UNITS_PER_WORD;
}

static machine_mode emex64_promote_function_mode(const_tree type,
                                                 machine_mode mode,
                                                 int *,
                                                 const_tree funtype,
                                                 int for_return)
{
    if(GET_MODE_CLASS(mode) == MODE_INT && GET_MODE_SIZE(mode) < 4)
    {
        return DImode;
    }
    if(!for_return && GET_MODE_CLASS(mode) == MODE_INT && GET_MODE_SIZE(mode) < UNITS_PER_WORD && type && (INTEGRAL_TYPE_P(type) || POINTER_TYPE_P(type)) && funtype && FUNC_OR_METHOD_TYPE_P(funtype) && stdarg_p(funtype))
    {
        return DImode;
    }
    return mode;
}

static rtx emex64_function_value(const_tree type,
                                 const_tree fn,
                                 bool)
{
    machine_mode mode = TYPE_MODE(type);
    int unsignedp = TYPE_UNSIGNED(type);
    
    if(INTEGRAL_TYPE_P(type) || POINTER_TYPE_P(type))
    {
        mode = promote_function_mode(type, mode, &unsignedp, fn, 1);
    }
    if(emex64_fp_value_p(mode, type))
    {
        return gen_rtx_REG(mode, FR0_REGNUM);
    }
    return emex64_gpr_value(mode, type, RR_REGNUM, R0_REGNUM);
}

static rtx emex64_libcall_value(machine_mode mode,
                                const_rtx)
{
    if(emex64_fp_value_p(mode, NULL_TREE))
    {
        return gen_rtx_REG(mode, FR0_REGNUM);
    }
    return emex64_gpr_value(mode, NULL_TREE, RR_REGNUM, R0_REGNUM);
}

static bool emex64_function_value_regno_p(const unsigned int regno)
{
    return (regno == RR_REGNUM || regno == R0_REGNUM || (TARGET_HARD_FLOAT && regno == FR0_REGNUM));
}

static bool emex64_strict_argument_naming(cumulative_args_t)
{
    return true;
}

static bool emex64_function_ok_for_sibcall (tree, tree)
{
    return false;
}

/* -------------------------------------------------------------------------
   Addresses and constants
   ------------------------------------------------------------------------- */

static rtx emex64_split_symbolic(rtx x,
                                 HOST_WIDE_INT *off)
{
    *off = 0;
    if(GET_CODE(x) == CONST)
    {
        x = XEXP(x, 0);
        if(GET_CODE(x) == PLUS && CONST_INT_P(XEXP(x, 1)))
        {
            *off = INTVAL(XEXP(x, 1));
            x = XEXP (x, 0);
        }
    }
    if(SYMBOL_REF_P (x) || LABEL_REF_P (x))
    {
        return x;
    }
    return NULL_RTX;
}

bool emex64_tls_symbol_p(rtx x)
{
    HOST_WIDE_INT off;
    rtx base = emex64_split_symbolic(x, &off);
    return base && SYMBOL_REF_P(base) && SYMBOL_REF_TLS_MODEL(base) != 0;
}

bool emex64_symbolic_operand_p(rtx x)
{
    HOST_WIDE_INT off;
    rtx base = emex64_split_symbolic (x, &off);
    if(!base)
    {
        return false;
    }
    return !(SYMBOL_REF_P(base) && SYMBOL_REF_TLS_MODEL(base) != 0);
}

bool emex64_local_symbolic_p(rtx x)
{
    HOST_WIDE_INT off;
    rtx base = emex64_split_symbolic(x, &off);
    if(!base)
    {
        return false;
    }
    if(LABEL_REF_P(base))
    {
        return true;
    }
    return SYMBOL_REF_LOCAL_P(base);
}

static bool emex64_unspec_const_p(rtx x,
                                  int kind)
{
    if(GET_CODE(x) != CONST)
    {
        return false;
    }
    x = XEXP(x, 0);
    if(GET_CODE(x) == PLUS && CONST_INT_P(XEXP(x, 1)))
    {
        x = XEXP (x, 0);
    }
    return GET_CODE(x) == UNSPEC && XINT(x, 1) == kind;
}

bool emex64_tpoff_const_p(rtx x)
{
    return emex64_unspec_const_p(x, UNSPEC_TPOFF);
}

static bool emex64_base_reg_p(rtx x,
                              bool strict)
{
    if(SUBREG_P(x) && !strict)
    {
        x = SUBREG_REG(x);
    }
    return REG_P(x) && emex64_regno_ok_for_base_p(REGNO(x), strict);
}

static bool emex64_legitimate_address_p(machine_mode mode,
                                        rtx x,
                                        bool strict,
                                        code_helper = ERROR_MARK)
{
    if(emex64_base_reg_p(x, strict))
    {
        return true;
    }

    if(GET_CODE(x) == PLUS)
    {
        rtx a = XEXP(x, 0), b = XEXP(x, 1);
        if(!emex64_base_reg_p (a, strict))
        {
            return false;
        }
        if(CONST_INT_P(b))
        {
            return true;
        }
        if(emex64_base_reg_p(b, strict))
        {
            return known_le(GET_MODE_SIZE(mode), UNITS_PER_WORD);
        }
        return emex64_unspec_const_p(b, UNSPEC_TPOFF);
    }

    if(emex64_unspec_const_p(x, UNSPEC_GOTPCREL) || emex64_unspec_const_p(x, UNSPEC_GOTTPOFF))
    {
        return mode == DImode;
    }

    if(emex64_symbolic_operand_p(x))
    {
        return !flag_pic || emex64_local_symbolic_p(x);
    }

    return false;
}

static bool emex64_legitimate_constant_p(machine_mode,
                                         rtx x)
{
    switch(GET_CODE(x))
    {
        case CONST_INT:
        case CONST_DOUBLE:
        case CONST_WIDE_INT:
            return true;
        case SYMBOL_REF:
        case LABEL_REF:
        case CONST:
            if(!emex64_symbolic_operand_p (x))
            {
                return false;
            }
            return !flag_pic || emex64_local_symbolic_p (x);
        default:
            return false;
    }
}

static bool emex64_cannot_force_const_mem(machine_mode,
                                          rtx x)
{
    subrtx_iterator::array_type array;
    FOR_EACH_SUBRTX(iter, array, x, ALL)
    {
        const_rtx y = *iter;
        if(GET_CODE(y) == UNSPEC)
        {
            return true;
        }
        if(SYMBOL_REF_P(y) && (SYMBOL_REF_TLS_MODEL(y) != 0 || (flag_pic && !SYMBOL_REF_LOCAL_P(y))))
        {
            return true;
        }
    }
    return false;
}

static rtx emex64_unspec_const(rtx sym,
                               int kind)
{
    return gen_rtx_CONST(Pmode, gen_rtx_UNSPEC(Pmode, gen_rtvec(1, sym), kind));
}

static GTY(()) rtx emex64_tls_get_addr_libfunc;

static rtx emex64_legitimize_tls_address(rtx x)
{
    HOST_WIDE_INT off;
    rtx sym = emex64_split_symbolic(x, &off);
    rtx tp = gen_rtx_REG(Pmode, TP_REGNUM);
    rtx res;

    switch(SYMBOL_REF_TLS_MODEL(sym))
    {
        case TLS_MODEL_LOCAL_EXEC:
            res = gen_reg_rtx(Pmode);
            emit_insn(gen_emex64_add_tpoff(res, tp, emex64_unspec_const(sym, UNSPEC_TPOFF)));
            break;
        case TLS_MODEL_INITIAL_EXEC:
        {
            rtx t = gen_reg_rtx(Pmode);
            emit_move_insn(t, gen_const_mem(Pmode, emex64_unspec_const(sym, UNSPEC_GOTTPOFF)));
            res = gen_reg_rtx(Pmode);
            emit_insn(gen_adddi3(res, tp, t));
            break;
        }
        default:
        {
            if(!emex64_tls_get_addr_libfunc)
            {
                emex64_tls_get_addr_libfunc = init_one_libfunc("__tls_get_addr");
            }
            rtx a = gen_reg_rtx(Pmode);
            emit_insn(gen_emex64_load_tlsgd(a, sym));
            res = emit_library_call_value(emex64_tls_get_addr_libfunc, NULL_RTX, LCT_CONST, Pmode, a, Pmode);
            res = copy_to_mode_reg(Pmode, res);
            break;
        }
    }

    if(off)
    {
        res = force_reg(Pmode, plus_constant(Pmode, res, off));
    }

    return res;
}

static rtx emex64_legitimize_got(rtx x)
{
    HOST_WIDE_INT off;
    rtx sym = emex64_split_symbolic(x, &off);
    rtx res = gen_reg_rtx(Pmode);
    emit_move_insn(res, gen_const_mem(Pmode, emex64_unspec_const(sym, UNSPEC_GOTPCREL)));
    if(off)
    {
        res = force_reg(Pmode, plus_constant(Pmode, res, off));
    }
    return res;
}

static rtx emex64_legitimize_address(rtx x,
                                     rtx,
                                     machine_mode)
{
    if(!can_create_pseudo_p())
    {
        return x;
    }
    if(emex64_tls_symbol_p(x))
    {
        return emex64_legitimize_tls_address(x);
    }
    if(flag_pic && emex64_symbolic_operand_p(x) && !emex64_local_symbolic_p(x))
    {
        return emex64_legitimize_got(x);
    }
    return x;
}

bool emex64_expand_move(machine_mode mode,
                        rtx *operands)
{
    rtx dst = operands[0], src = operands[1];

    if(!can_create_pseudo_p())
    {
        return false;
    }

    if(mode == DImode && (SYMBOL_REF_P(src) || LABEL_REF_P(src) || GET_CODE(src) == CONST))
    {
        if(emex64_tls_symbol_p(src))
        {
            src = emex64_legitimize_tls_address(src);
        }
        else if(flag_pic && emex64_symbolic_operand_p(src) && !emex64_local_symbolic_p(src))
        {
            src = emex64_legitimize_got (src);
        }
    }

    if(MEM_P(dst) && !register_operand(src, mode) && !CONST_INT_P(src) && !(CONST_DOUBLE_P(src) && SCALAR_FLOAT_MODE_P(mode)))
    {
        src = force_reg(mode, src);
    }

    operands[1] = src;
    return false;
}

/* -------------------------------------------------------------------------
   Compares
   ------------------------------------------------------------------------- */

static rtx emex64_cf(machine_mode mode)
{
    return gen_rtx_REG(mode, CF_REGNUM);
}

static rtx_insn *emex64_emit_jump(rtx cond,
                                  rtx label,
                                  rtx clobber)
{
    rtx set = gen_rtx_SET(pc_rtx, gen_rtx_IF_THEN_ELSE(VOIDmode, cond, gen_rtx_LABEL_REF(Pmode, label), pc_rtx));
    rtx pat = clobber ? gen_rtx_PARALLEL(VOIDmode, gen_rtvec(2, set, clobber)) : set;
    rtx_insn *insn = emit_jump_insn(pat);
    JUMP_LABEL(insn) = label;
    LABEL_NUSES(label)++;
    return insn;
}

static rtx emex64_flip_sign(rtx x)
{
    if(CONST_INT_P(x))
    {
        return GEN_INT(INTVAL(x) ^ HOST_WIDE_INT_MIN);
    }
    return expand_simple_binop(DImode, XOR, x, GEN_INT(HOST_WIDE_INT_MIN), NULL_RTX, 1, OPTAB_DIRECT);
}

static void emex64_canonicalize_int_compare(rtx_code *code,
                                            rtx *a,
                                            rtx *b)
{
    if(CONST_INT_P(*a) && !CONST_INT_P(*b))
    {
        std::swap(*a, *b);
        *code = swap_condition(*code);
    }

    if(*b == const0_rtx)
    {
        if(*code == GTU)
        {
            *code = NE;
        }
        else if(*code == LEU)
        {
            *code = EQ;
        }
    }

    switch(*code)
    {
        case LTU:
        case GTU:
        case LEU:
        case GEU:
            *a = emex64_flip_sign(*a);
            *b = emex64_flip_sign(*b);
            *code = signed_condition(*code);
            break;
        default:
            break;
    }

    if(CONST_INT_P(*a) && CONST_INT_P(*b))
    {
        *a = force_reg (DImode, *a);
    }
}

void emex64_expand_cbranch(rtx *operands,
                           machine_mode mode)
{
    rtx_code code = GET_CODE(operands[0]);
    rtx a = operands[1], b = operands[2], label = operands[3];

    if(SCALAR_FLOAT_MODE_P(mode))
    {
        rtx cf = emex64_cf(CCFPmode);
        emit_insn(gen_rtx_SET (cf, gen_rtx_COMPARE(CCFPmode, a, b)));
        emex64_emit_jump(gen_rtx_fmt_ee(code, VOIDmode, cf, const0_rtx), label, NULL_RTX);
        return;
    }

    if(TARGET_ISA20)
    {
        if(CONST_INT_P(a) && !CONST_INT_P(b))
        {
            std::swap(a, b);
            code = swap_condition(code);
        }
        if(b == const0_rtx)
        {
            if(code == GTU)
            {
                code = NE;
            }
            else if(code == LEU)
            {
                code = EQ;
            }
        }
        if(CONST_INT_P(a))
        {
            a = force_reg(DImode, a);
        }
        if((code == EQ || code == NE) && b == const0_rtx)
        {
            emex64_emit_jump(gen_rtx_fmt_ee(code, VOIDmode, a, const0_rtx), label, NULL_RTX);
            return;
        }
        emex64_emit_jump(gen_rtx_fmt_ee(code, VOIDmode, a, b), label, NULL_RTX);
        return;
    }

    emex64_canonicalize_int_compare(&code, &a, &b);
    if((code == EQ || code == NE) && b == const0_rtx)
    {
        a = force_reg(DImode, a);
        emex64_emit_jump(gen_rtx_fmt_ee(code, VOIDmode, a, const0_rtx), label, NULL_RTX);
        return;
    }

    if(TARGET_ISA19)
    {
        if(CONST_INT_P(b))
        {
            HOST_WIDE_INT v = INTVAL(b);
            if(code == LE && v != HOST_WIDE_INT_MAX)
            {
                code = LT, b = GEN_INT(v + 1);
            }
            else if(code == GE && v != HOST_WIDE_INT_MIN)
            {
                code = GT, b = GEN_INT(v - 1);
            }
        }
        if(code == LT || code == GT)
        {
            emex64_emit_jump(gen_rtx_fmt_ee(code, VOIDmode, a, b), label, gen_rtx_CLOBBER(VOIDmode, emex64_cf(CCmode)));
            return;
        }
    }

    rtx cf = emex64_cf(CCmode);
    emit_insn(gen_rtx_SET(cf, gen_rtx_COMPARE(CCmode, a, b)));
    emex64_emit_jump(gen_rtx_fmt_ee(code, VOIDmode, cf, const0_rtx), label, NULL_RTX);
}

void emex64_expand_cstore(rtx *operands,
                          machine_mode mode)
{
    rtx dst = operands[0];
    rtx_code code = GET_CODE(operands[1]);
    rtx a = operands[2], b = operands[3];
    rtx cf;

    if(SCALAR_FLOAT_MODE_P(mode))
    {
        cf = emex64_cf(CCFPmode);
        emit_insn(gen_rtx_SET(cf, gen_rtx_COMPARE(CCFPmode, a, b)));
    }
    else
    {
        emex64_canonicalize_int_compare(&code, &a, &b);
        cf = emex64_cf(CCmode);
        emit_insn(gen_rtx_SET(cf, gen_rtx_COMPARE(CCmode, a, b)));
    }

    emit_insn(gen_rtx_SET(dst, gen_rtx_fmt_ee(code, DImode, cf, const0_rtx)));
}

static unsigned int emex64_cond_mask(rtx_code code)
{
    switch(code)
    {
        case EQ:
            return 1;
        case NE:
            return 2 | 4 | 8;
        case LT:
            return 2;
        case GT:
            return 4;
        case LE:
            return 2 | 1;
        case GE:
            return 4 | 1;
        case UNORDERED:
            return 8;
        case ORDERED:
            return 1 | 2 | 4;
        case UNEQ:
            return 8 | 1;
        case UNLT:
            return 8 | 2;
        case UNGT:
            return 8 | 4;
        case UNLE:
            return 8 | 2 | 1;
        case UNGE:
            return 8 | 4 | 1;
        case LTGT:
            return 2 | 4;
        default:
            gcc_unreachable();
    }
}

const char *emex64_output_cf_branch(rtx cond,
                                    rtx label,
                                    bool reversed)
{
    rtx_code code = GET_CODE(cond);
    bool fp = GET_MODE(XEXP(cond, 0)) == CCFPmode;
    rtx ops[2];
    const char *insn = NULL;

    if(reversed)
    {
        code = fp ? reverse_condition_maybe_unordered(code) : reverse_condition(code);
    }

    switch(code)
    {
        case EQ:
            insn = "be";
            break;
        case NE:
            insn = "bne";
            break;
        case LT:
            insn = "blt";
            break;
        case GT:
            insn = "bgt";
            break;
        case LE:
            insn = "ble";
            break;
        case GE:
            insn = "bge";
            break;
        default:
            break;
    }

    ops[0] = label;
    if(insn)
    {
        char buf[32];
        snprintf(buf, sizeof buf, "%s\t%%l0@pcrel", insn);
        output_asm_insn(buf, ops);
    }
    else
    {
        ops[1] = GEN_INT(emex64_cond_mask(code));
        output_asm_insn("bbnz\tcf, %1, %l0@pcrel", ops);
    }

    return "";
}

const char *emex64_output_scc(rtx *operands)
{
    rtx_code code = GET_CODE(operands[1]);
    bool fp = GET_MODE(operands[2]) == CCFPmode;
    unsigned int all = fp ? 0xf : 0x7;
    int top = fp ? 3 : 2;
    unsigned int m = emex64_cond_mask(code) & all;
    unsigned int inv = all & ~m;
    static char buf[96];

    if(pow2p_hwi(m) || pow2p_hwi(inv))
    {
        bool invert = !pow2p_hwi(m);
        int k = exact_log2(invert ? inv : m);
        if(k == top)
        {
            snprintf(buf, sizeof buf, "shr\t%%0, cf, %d", k);
        }
        else if(k == 0)
        {
            snprintf(buf, sizeof buf, "and\t%%0, cf, 1");
        }
        else
        {
            snprintf(buf, sizeof buf, "shr\t%%0, cf, %d\n\tand\t%%0, %%0, 1", k);
        }
        if(invert)
        {
            strcat(buf, "\n\txor\t%0, %0, 1");
        }
        return buf;
    }

    snprintf (buf, sizeof buf, "and\t%%0, cf, %u\n\tadd\t%%0, %%0, 15\n\tshr\t%%0, %%0, 4", m);
    return buf;
}

/* -------------------------------------------------------------------------
   Calls
   ------------------------------------------------------------------------- */

void emex64_expand_call(rtx retval,
                        rtx fnmem,
                        rtx callarg)
{
    rtx addr = XEXP(fnmem, 0);
    if(!emex64_call_operand(addr, Pmode))
    {
        addr = force_reg(Pmode, addr);
    }
    rtx call = gen_rtx_CALL(VOIDmode, gen_rtx_MEM(QImode, addr), callarg);
    if(retval)
    {
        call = gen_rtx_SET(retval, call);
    }
    emit_call_insn (call);
}

#define EMEX64_CALL_ARGS 6

static bool emex64_call_arg_value_p(rtx dest,
                                    rtx src)
{
    machine_mode mode = GET_MODE(dest);
    if(!known_le(GET_MODE_SIZE(mode), UNITS_PER_WORD))
    {
        return false;
    }
    if(REG_P(src))
    {
        return HARD_REGISTER_P(src) && REGNO(src) < ARG_POINTER_REGNUM;
    }
    if(GET_MODE_CLASS(mode) != MODE_INT)
    {
        return false;
    }
    if(CONST_INT_P(src))
    {
        return INTVAL(src) >= 0;
    }
    if(mode != DImode)
    {
        return false;
    }
    if(GET_CODE(src) == PLUS)
    {
        rtx a = XEXP(src, 0), b = XEXP(src, 1);
        return REG_P(a) && HARD_REGISTER_P(a) && ((REG_P(b) && HARD_REGISTER_P(b)) || (CONST_INT_P(b) && INTVAL(b) != HOST_WIDE_INT_MIN));
    }
    return emex64_symbolic_operand(src, DImode);
}

static bool emex64_fold_call_arguments(rtx_insn *call)
{
    rtx pat = PATTERN(call);
    rtx c = GET_CODE(pat) == SET ? SET_SRC(pat) : pat;
    if(GET_CODE(c) != CALL || !MEM_P(XEXP(c, 0)))
    {
        return false;
    }

    rtx target = XEXP(XEXP(c, 0), 0);
    unsigned int target_reg = REG_P(target) ? REGNO(target) : INVALID_REGNUM;

    rtx_insn *moves[EMEX64_CALL_ARGS] = {};
    rtx values[EMEX64_CALL_ARGS] = {};
    HARD_REG_SET read_behind;
    CLEAR_HARD_REG_SET(read_behind);
    unsigned int folded = 0;
    int last = -1;

    for(rtx_insn *p = PREV_INSN(call); p; p = PREV_INSN(p))
    {
        if(DEBUG_INSN_P(p))
        {
            continue;
        }
        if(NOTE_P(p))
        {
            int kind = NOTE_KIND(p);
            if(kind == NOTE_INSN_DELETED || kind == NOTE_INSN_VAR_LOCATION || kind == NOTE_INSN_BEGIN_STMT || kind == NOTE_INSN_INLINE_ENTRY)
            {
                continue;
            }
            break;
        }
        if(!NONJUMP_INSN_P(p) || RTX_FRAME_RELATED_P(p) || GET_CODE(PATTERN(p)) != SET)
        {
            break;
        }

        rtx dest = SET_DEST(PATTERN(p)), src = SET_SRC(PATTERN(p));
        if(!REG_P(dest) || REGNO(dest) >= EMEX64_CALL_ARGS || !emex64_call_arg_value_p(dest, src))
        {
            break;
        }
        unsigned int arg = REGNO(dest);
        if(moves[arg] || arg == target_reg || TEST_HARD_REG_BIT(read_behind, arg))
        {
            break;
        }

        moves[arg] = p;
        values[arg] = src;
        folded++;
        last = MAX(last, (int)arg);

        subrtx_iterator::array_type array;
        FOR_EACH_SUBRTX(iter, array, src, NONCONST)
        {
            const_rtx x = *iter;
            if(REG_P(x))
            {
                SET_HARD_REG_BIT(read_behind, REGNO(x));
            }
        }
    }

    if(folded == 0 || (unsigned int)(last + 1) - folded > folded)
    {
        return false;
    }

    rtvec v = rtvec_alloc(last + 1);
    for(int i = 0; i <= last; i++)
    {
        RTVEC_ELT(v, i) = moves[i] ? copy_rtx(values[i]) : gen_rtx_REG(DImode, i);
    }
    rtx args = gen_rtx_UNSPEC(DImode, v, UNSPEC_CALL_ARGS);
    PATTERN(call) = gen_rtx_PARALLEL(VOIDmode, gen_rtvec(2, pat, gen_rtx_USE(VOIDmode, args)));
    INSN_CODE(call) = -1;
    if(recog_memoized(call) < 0)
    {
        PATTERN(call) = pat;
        INSN_CODE(call) = -1;
        return false;
    }

    for(int i = 0; i <= last; i++)
    {
        if(moves[i])
        {
            delete_insn(moves[i]);
        }
    }
    return true;
}

static void emex64_reorg(void)
{
    if(!TARGET_ISA22)
    {
        return;
    }
    for(rtx_insn *insn = get_insns(); insn; insn = NEXT_INSN(insn))
    {
        if(CALL_P(insn) && !SIBLING_CALL_P(insn))
        {
            emex64_fold_call_arguments(insn);
        }
    }
}

/* -------------------------------------------------------------------------
   Atomics
   ------------------------------------------------------------------------- */

void emex64_expand_atomic_cas(rtx *operands)
{
    rtx bval = operands[0], oldval = operands[1], mem = operands[2];
    rtx expected = operands[3], desired = operands[4];
    machine_mode mode = GET_MODE(mem);
    rtx ok = gen_reg_rtx(DImode);
    rtx addr = force_reg(Pmode, XEXP(mem, 0));

    if(mode == DImode)
    {
        rtx word = change_address(mem, DImode, addr);
        rtx old = gen_reg_rtx(DImode);
        emit_move_insn(old, expected);
        rtx des = force_reg(DImode, desired);
        emit_insn(gen_emex64_cas(old, word, old, des));
        emit_insn(gen_rtx_SET(ok, gen_rtx_EQ(DImode, emex64_cf(CCmode), const0_rtx)));
        emit_move_insn(oldval, old);
        emit_move_insn(bval, gen_lowpart(SImode, ok));
        return;
    }

    unsigned HOST_WIDE_INT fmask = GET_MODE_MASK(mode);
    rtx aligned = expand_simple_binop(Pmode, AND, addr, GEN_INT(-8), NULL_RTX, 1, OPTAB_DIRECT);
    aligned = force_reg(Pmode, aligned);
    rtx shift = expand_simple_binop (Pmode, AND, addr, GEN_INT(7), NULL_RTX, 1, OPTAB_DIRECT);
    shift = expand_simple_binop (Pmode, ASHIFT, shift, GEN_INT(3), NULL_RTX, 1, OPTAB_DIRECT);
    rtx mask = expand_simple_binop(DImode, ASHIFT, force_reg(DImode, GEN_INT(fmask)), shift, NULL_RTX, 1, OPTAB_DIRECT);
    rtx notmask = expand_simple_unop(DImode, NOT, mask, NULL_RTX, 1);

    rtx e = CONST_INT_P(expected) ? force_reg(DImode, GEN_INT(INTVAL(expected) & fmask)) : convert_modes(DImode, mode, force_reg(mode, expected), 1);
    rtx d = CONST_INT_P (desired) ? force_reg(DImode, GEN_INT(INTVAL(desired) & fmask)) : convert_modes(DImode, mode, force_reg(mode, desired), 1);
    e = expand_simple_binop(DImode, ASHIFT, e, shift, NULL_RTX, 1, OPTAB_DIRECT);
    d = expand_simple_binop(DImode, ASHIFT, d, shift, NULL_RTX, 1, OPTAB_DIRECT);
    e = force_reg(DImode, e);
    d = force_reg(DImode, d);

    rtx word = gen_rtx_MEM(DImode, aligned);
    MEM_VOLATILE_P(word) = 1;
    set_mem_alias_set(word, 0);

    rtx cur = gen_reg_rtx(DImode);
    rtx old = gen_reg_rtx(DImode);
    rtx_code_label *loop = gen_label_rtx();
    rtx_code_label *done = gen_label_rtx();

    emit_move_insn(cur, word);
    emit_label(loop);
    rtx rest = expand_simple_binop(DImode, AND, cur, notmask, NULL_RTX, 1, OPTAB_DIRECT);
    rtx we = expand_simple_binop(DImode, IOR, rest, e, NULL_RTX, 1, OPTAB_DIRECT);
    rtx wd = force_reg(DImode, expand_simple_binop(DImode, IOR, rest, d, NULL_RTX, 1, OPTAB_DIRECT));
    emit_move_insn(old, we);
    emit_insn(gen_emex64_cas(old, word, old, wd));
    emit_move_insn(ok, const1_rtx);
    emex64_emit_jump(gen_rtx_EQ(VOIDmode, emex64_cf(CCmode), const0_rtx), done, NULL_RTX);
    emit_move_insn(ok, const0_rtx);
    rtx field = expand_simple_binop(DImode, AND, old, mask, NULL_RTX, 1, OPTAB_DIRECT);
    emit_cmp_and_jump_insns (field, e, NE, NULL_RTX, DImode, 1, done);
    emit_move_insn(cur, old);
    emit_jump(loop);
    emit_barrier();
    emit_label(done);

    rtx v = expand_simple_binop(DImode, AND, old, mask, NULL_RTX, 1, OPTAB_DIRECT);
    v = expand_simple_binop(DImode, LSHIFTRT, v, shift, NULL_RTX, 1, OPTAB_DIRECT);
    emit_move_insn(oldval, gen_lowpart(mode, force_reg(DImode, v)));
    emit_move_insn(bval, gen_lowpart(SImode, ok));
}

/* -------------------------------------------------------------------------
   Assembly output
   ------------------------------------------------------------------------- */

static void emex64_print_symbolic(FILE *f,
                                  rtx x,
                                  const char *suffix)
{
    HOST_WIDE_INT off;
    rtx base = emex64_split_symbolic(x, &off);
    if(!base)
    {
        output_addr_const(f, x);
        fputs(suffix, f);
        return;
    }
    output_addr_const(f, base);
    fputs(suffix, f);
    if(off > 0)
    {
        fprintf(f, "+" HOST_WIDE_INT_PRINT_DEC, off);
    }
    else if(off < 0)
    {
        fprintf(f, HOST_WIDE_INT_PRINT_DEC, off);
    }
}

static bool emex64_print_unspec(FILE *f,
                                rtx x)
{
    if(GET_CODE(x) != CONST)
    {
        return false;
    }

    rtx u = XEXP(x, 0);
    HOST_WIDE_INT off = 0;
    if(GET_CODE(u) == PLUS && CONST_INT_P(XEXP(u, 1)))
    {
        off = INTVAL(XEXP(u, 1));
        u = XEXP(u, 0);
    }

    if(GET_CODE(u) != UNSPEC)
    {
        return false;
    }

    const char *s;
    switch(XINT(u, 1))
    {
        case UNSPEC_TPOFF:
            s = "@tpoff";
            break;
        case UNSPEC_GOTPCREL:
            s = "@gotpcrel";
            break;
        case UNSPEC_GOTTPOFF:
            s = "@gottpoff";
            break;
        case UNSPEC_TLSGD:
            s = "@tlsgd";
            break;
        default:
            return false;
    }

    output_addr_const(f, XVECEXP(u, 0, 0));
    fputs(s, f);
    if(off > 0)
    {
        fprintf(f, "+" HOST_WIDE_INT_PRINT_DEC, off);
    }
    else if(off < 0)
    {
        fprintf(f, HOST_WIDE_INT_PRINT_DEC, off);
    }

    return true;
}

static void emex64_print_float_bits(FILE *f,
                                    rtx x)
{
    long l[2] = { 0, 0 };
    machine_mode mode = GET_MODE(x);
    real_to_target(l, CONST_DOUBLE_REAL_VALUE(x), mode);
    if(mode == SFmode)
    {
        fprintf (f, "0x%08lx", (unsigned long)(l[0] & 0xffffffff));
    }
    else
    {
        fprintf(f, "0x%08lx%08lx", (unsigned long)(l[1] & 0xffffffff), (unsigned long)(l[0] & 0xffffffff));
    }
}

static void emex64_print_operand_address(FILE *f,
                                         machine_mode,
                                         rtx addr)
{
    switch(GET_CODE(addr))
    {
        case REG:
            fprintf(f, "[%s]", reg_names[REGNO(addr)]);
            return;
        case PLUS:
        {
            rtx a = XEXP(addr, 0), b = XEXP(addr, 1);
            gcc_assert(REG_P(a));
            if(CONST_INT_P(b))
            {
                HOST_WIDE_INT v = INTVAL(b);
                if(v < 0 && v != HOST_WIDE_INT_MIN)
                {
                    fprintf(f, "[%s - " HOST_WIDE_INT_PRINT_DEC "]", reg_names[REGNO(a)], -v);
                }
                else
                {
                    fprintf(f, "[%s + " HOST_WIDE_INT_PRINT_DEC "]",
                    reg_names[REGNO (a)], v);
                }
            }
	        else if(REG_P(b))
            {
	            fprintf(f, "[%s + %s]", reg_names[REGNO(a)], reg_names[REGNO(b)]);
            }
            else
            {
                fprintf(f, "[%s + ", reg_names[REGNO(a)]);
                if(!emex64_print_unspec(f, b))
                {
                    output_addr_const(f, b);
                }
                fputc(']', f);
            }
            return;
        }
        case CONST:
            if(emex64_unspec_const_p(addr, UNSPEC_GOTPCREL) || emex64_unspec_const_p(addr, UNSPEC_GOTTPOFF))
            {
                fputs("[pc + ", f);
                emex64_print_unspec(f, addr);
                fputc(']', f);
                return;
            }
            break;
        default:
            break;
    }
    
    if(flag_pic)
    {
        fputs("[pc + ", f);
        emex64_print_symbolic(f, addr, "@pcrel");
        fputc(']', f);
    }
    else
    {
        fputc('[', f);
        output_addr_const(f, addr);
        fputc(']', f);
    }
}

const char *emex64_output_call_args(rtx target,
                                    rtx args)
{
    FILE *f = asm_out_file;
    fputs("\tbl\t", f);
    if(REG_P(target))
    {
        fputs(reg_names[REGNO(target)], f);
    }
    else
    {
        output_addr_const(f, target);
        fputs("@plt", f);
    }

    for(int i = 0; i < XVECLEN(args, 0); i++)
    {
        rtx x = XVECEXP(args, 0, i);
        fputs(", ", f);
        if(REG_P(x))
        {
            fputs(reg_names[REGNO(x)], f);
        }
        else if(CONST_INT_P(x))
        {
            fprintf(f, HOST_WIDE_INT_PRINT_DEC, INTVAL(x));
        }
        else if(GET_CODE(x) == PLUS)
        {
            emex64_print_operand_address(f, DImode, x);
        }
        else if(flag_pic)
        {
            fputs("[pc + ", f);
            emex64_print_symbolic(f, x, "@pcrel");
            fputc(']', f);
        }
        else
        {
            output_addr_const(f, x);
        }
    }

    fputc('\n', f);
    return "";
}

static void emex64_print_operand(FILE *f,
                                 rtx x,
                                 int code)
{
    switch(code)
    {
        case 'H':
            gcc_assert(REG_P (x));
            fputs(reg_names[REGNO (x) + 1], f);
            return;
        case 'F':
            if(CONST_DOUBLE_P (x))
            {
                emex64_print_float_bits (f, x);
            }
            else
            {
                output_operand_lossage ("invalid %%F operand");
            }
            return;
        case 'B':
        case 'W':
        case 'D':
            if(CONST_INT_P(x))
            {
                unsigned HOST_WIDE_INT m = code == 'B' ? 0xff : code == 'W' ? 0xffff : 0xffffffff;
                fprintf(f, HOST_WIDE_INT_PRINT_UNSIGNED, UINTVAL(x) & m);
            }
            else
            {
                output_operand_lossage("invalid %%%c operand", code);
            }
            return;
        case 'd':
            if(!CONST_INT_P(x))
            {
                break;
            }
            fprintf(f, HOST_WIDE_INT_PRINT_UNSIGNED, UINTVAL(x) & 0xffffffff);
            return;
        case 'n':
            if(CONST_INT_P(x))
            {
                fprintf(f, HOST_WIDE_INT_PRINT_UNSIGNED, (-UINTVAL (x)) & 0xffffffff);
            }
            else
            {
                output_operand_lossage ("invalid %%n operand");
            }
            return;
        case 'N':
            if(CONST_INT_P(x))
            {
                fprintf(f, HOST_WIDE_INT_PRINT_UNSIGNED, -(unsigned HOST_WIDE_INT)INTVAL(x));
            }
            else
            {
                output_operand_lossage ("invalid %%N operand");
            }
            return;
        case 'P':
            emex64_print_symbolic(f, x, "@pcrel");
            return;
        case 0:
            break;
        default:
            output_operand_lossage("invalid operand code '%c'", code);
            return;
    }

    switch(GET_CODE(x))
    {
        case REG:
            fputs(reg_names[REGNO(x)], f);
            break;
        case MEM:
            output_address(GET_MODE(x), XEXP (x, 0));
            break;
        case CONST_INT:
            fprintf (f, HOST_WIDE_INT_PRINT_DEC, INTVAL(x));
            break;
        case CONST_DOUBLE:
            emex64_print_float_bits(f, x);
            break;
        case CONST:
            if(emex64_print_unspec(f, x))
            {
                break;
            }
            output_addr_const(f, x);
            break;
        default:
            output_addr_const(f, x);
            break;
    }
}

bool emex64_neg_imm_p(rtx x)
{
    return(CONST_INT_P(x) && INTVAL(x) < 0 && INTVAL(x) >= -(HOST_WIDE_INT) 0xffffffff);
}

bool emex64_dw_neg_imm_p(rtx x)
{
    return CONST_INT_P(x) && (INTVAL(x) & 0x80000000) != 0;
}

const char *emex64_output_const_move(rtx *operands)
{
    return emex64_neg_imm_p(operands[1]) ? "sub\t%0, 0, %N1" : "mov\t%0, %1";
}

const char *emex64_output_symbolic_move (rtx *)
{
    return flag_pic ? "add\t%0, pc, %P1" : "mov\t%0, %1";
}

const char *emex64_output_sign_extend(rtx *operands,
                                      int bits)
{
    static char buf[96];
    const char *ld = bits == 8 ? "ldb" : bits == 16 ? "ldw" : "ldd";
    const char *src = "%1";

    if(MEM_P(operands[1]) && TARGET_ISA20)
    {
        return bits == 8 ? "ldsb\t%0, %1" : bits == 16 ? "ldsw\t%0, %1" : "ldsd\t%0, %1";
    }

    buf[0] = 0;
    if(MEM_P(operands[1]))
    {
        snprintf(buf, sizeof buf, "%s\t%%0, %%1\n\t", ld);
        src = "%0";
    }
    size_t n = strlen(buf);
    if(TARGET_ISA19)
    {
        snprintf(buf + n, sizeof buf - n, "sext\t%%0, %s, %d", src, bits);
    }
    else
    {
        snprintf(buf + n, sizeof buf - n, "shl\t%%0, %s, %d\n\tsar\t%%0, %%0, %d", src, 64 - bits, 64 - bits);
    }
    return buf;
}

const char *emex64_output_cbranch(rtx *operands,
                                  bool reversed)
{
    static char buf[64];
    rtx_code code = GET_CODE(operands[0]);
    if(reversed)
    {
        code = reverse_condition(code);
    }

    const char *op;
    bool swap = false;
    switch(code)
    {
        case EQ:
            op = "cbeq";
            break;
        case NE:
            op = "cbne";
            break;
        case LT:
            op = "cblt";
            break;
        case GE:
            op = "cbge";
            break;
        case GT:
            op = "cblt";
            swap = true;
            break;
        case LE:
            op = "cbge";
            swap = true;
            break;
        case LTU:
            op = "cbltu";
            break;
        case GEU:
            op = "cbgeu";
            break;
        case GTU:
            op = "cbltu";
            swap = true;
            break;
        case LEU:
            op = "cbgeu";
            swap = true;
            break;
        default:
            gcc_unreachable();
    }

    snprintf(buf, sizeof buf, swap ? "%s\t%%2, %%1, %%l3@pcrel" : "%s\t%%1, %%2, %%l3@pcrel", op);
    return buf;
}

static void emex64_output_dwarf_dtprel(FILE *f,
                                       int size,
                                       rtx x)
{
    fputs(size == 4 ? "\t.long\t" : "\t.quad\t", f);
    output_addr_const (f, x);
    fputs("@dtpoff", f);
}

/* -------------------------------------------------------------------------
   Costs
   ------------------------------------------------------------------------- */

static bool emex64_rtx_costs(rtx x,
                             machine_mode,
                             int,
                             int,
                             int *total,
                             bool)
{
    switch(GET_CODE(x))
    {
        case CONST_INT:
        case CONST_DOUBLE:
        case CONST_WIDE_INT:
            /* every operand can be an immediate */
            *total = 0;
            return true;
        case SYMBOL_REF:
        case LABEL_REF:
        case CONST:
            *total = flag_pic ? COSTS_N_INSNS(1) : 0;
            return true;
        case MULT:
            *total = COSTS_N_INSNS(2);
            return false;
        case DIV:
        case UDIV:
        case MOD:
        case UMOD:
            *total = COSTS_N_INSNS(8);
            return false;
        default:
        return false;
    }
}

static int emex64_address_cost(rtx,
                               machine_mode,
                               addr_space_t,
                               bool)
{
    return 1;
}

/* -------------------------------------------------------------------------
   Target hooks
   ------------------------------------------------------------------------- */

#undef TARGET_OPTION_OVERRIDE
#define TARGET_OPTION_OVERRIDE emex64_option_override
#undef TARGET_CONDITIONAL_REGISTER_USAGE
#define TARGET_CONDITIONAL_REGISTER_USAGE emex64_conditional_register_usage

#undef TARGET_HARD_REGNO_MODE_OK
#define TARGET_HARD_REGNO_MODE_OK emex64_hard_regno_mode_ok
#undef TARGET_MODES_TIEABLE_P
#define TARGET_MODES_TIEABLE_P emex64_modes_tieable_p
#undef TARGET_CAN_CHANGE_MODE_CLASS
#define TARGET_CAN_CHANGE_MODE_CLASS emex64_can_change_mode_class
#undef TARGET_REGISTER_MOVE_COST
#define TARGET_REGISTER_MOVE_COST emex64_register_move_cost
#undef TARGET_MEMORY_MOVE_COST
#define TARGET_MEMORY_MOVE_COST emex64_memory_move_cost

#undef TARGET_COMPUTE_FRAME_LAYOUT
#define TARGET_COMPUTE_FRAME_LAYOUT emex64_compute_frame_layout
#undef TARGET_FRAME_POINTER_REQUIRED
#define TARGET_FRAME_POINTER_REQUIRED emex64_frame_pointer_required
#undef TARGET_CAN_ELIMINATE
#define TARGET_CAN_ELIMINATE hook_bool_const_int_const_int_true

#undef TARGET_FUNCTION_ARG
#define TARGET_FUNCTION_ARG emex64_function_arg
#undef TARGET_FUNCTION_ARG_ADVANCE
#define TARGET_FUNCTION_ARG_ADVANCE emex64_function_arg_advance
#undef TARGET_FUNCTION_ARG_BOUNDARY
#define TARGET_FUNCTION_ARG_BOUNDARY emex64_function_arg_boundary
#undef TARGET_EXPAND_BUILTIN_VA_START
#define TARGET_EXPAND_BUILTIN_VA_START emex64_va_start
#undef TARGET_ARG_PARTIAL_BYTES
#define TARGET_ARG_PARTIAL_BYTES emex64_arg_partial_bytes
#undef TARGET_PASS_BY_REFERENCE
#define TARGET_PASS_BY_REFERENCE emex64_pass_by_reference
#undef TARGET_RETURN_IN_MEMORY
#define TARGET_RETURN_IN_MEMORY emex64_return_in_memory
#undef TARGET_PROMOTE_FUNCTION_MODE
#define TARGET_PROMOTE_FUNCTION_MODE emex64_promote_function_mode
#undef TARGET_FUNCTION_VALUE
#define TARGET_FUNCTION_VALUE emex64_function_value
#undef TARGET_LIBCALL_VALUE
#define TARGET_LIBCALL_VALUE emex64_libcall_value
#undef TARGET_FUNCTION_VALUE_REGNO_P
#define TARGET_FUNCTION_VALUE_REGNO_P emex64_function_value_regno_p
#undef TARGET_STRICT_ARGUMENT_NAMING
#define TARGET_STRICT_ARGUMENT_NAMING emex64_strict_argument_naming
#undef TARGET_FUNCTION_OK_FOR_SIBCALL
#define TARGET_FUNCTION_OK_FOR_SIBCALL emex64_function_ok_for_sibcall
#undef TARGET_EMPTY_RECORD_P
#define TARGET_EMPTY_RECORD_P default_is_empty_record

#undef TARGET_LEGITIMATE_ADDRESS_P
#define TARGET_LEGITIMATE_ADDRESS_P emex64_legitimate_address_p
#undef TARGET_LEGITIMIZE_ADDRESS
#define TARGET_LEGITIMIZE_ADDRESS emex64_legitimize_address
#undef TARGET_LEGITIMATE_CONSTANT_P
#define TARGET_LEGITIMATE_CONSTANT_P emex64_legitimate_constant_p
#undef TARGET_CANNOT_FORCE_CONST_MEM
#define TARGET_CANNOT_FORCE_CONST_MEM emex64_cannot_force_const_mem
#undef TARGET_HAVE_TLS
#define TARGET_HAVE_TLS true

#undef TARGET_PRINT_OPERAND
#define TARGET_PRINT_OPERAND emex64_print_operand
#undef TARGET_PRINT_OPERAND_ADDRESS
#define TARGET_PRINT_OPERAND_ADDRESS emex64_print_operand_address
#undef TARGET_ASM_OUTPUT_DWARF_DTPREL
#define TARGET_ASM_OUTPUT_DWARF_DTPREL emex64_output_dwarf_dtprel
#undef TARGET_ASM_ALIGNED_HI_OP
#define TARGET_ASM_ALIGNED_HI_OP "\t.short\t"
#undef TARGET_ASM_ALIGNED_SI_OP
#define TARGET_ASM_ALIGNED_SI_OP "\t.long\t"
#undef TARGET_ASM_ALIGNED_DI_OP
#define TARGET_ASM_ALIGNED_DI_OP "\t.quad\t"
#undef TARGET_ASM_UNALIGNED_HI_OP
#define TARGET_ASM_UNALIGNED_HI_OP "\t.short\t"
#undef TARGET_ASM_UNALIGNED_SI_OP
#define TARGET_ASM_UNALIGNED_SI_OP "\t.long\t"
#undef TARGET_ASM_UNALIGNED_DI_OP
#define TARGET_ASM_UNALIGNED_DI_OP "\t.quad\t"

#undef TARGET_RTX_COSTS
#define TARGET_RTX_COSTS emex64_rtx_costs
#undef TARGET_ADDRESS_COST
#define TARGET_ADDRESS_COST emex64_address_cost

#undef TARGET_MACHINE_DEPENDENT_REORG
#define TARGET_MACHINE_DEPENDENT_REORG emex64_reorg

struct gcc_target targetm = TARGET_INITIALIZER;

static bool emex64_register_name_p(const char *name)
{
    static const char *const fixed[] = { "pc", "sp", "fp", "cf", "fpc", "rr", "tp" };
    char lower[32];
    size_t len = strlen(name);
    if(len == 0 || len >= sizeof lower)
    {
        return false;
    }
    for(size_t i = 0; i <= len; i++)
    {
        lower[i] = TOLOWER(name[i]);
    }
    for(const char *f : fixed)
    {
        if(strcmp (lower, f) == 0)
        {
            return true;
        }
    }

    const char *digits;
    unsigned limit;
    if(lower[0] == 'e' && lower[1] == 'r')
    {
        digits = lower + 2, limit = 16;
    }
    else if(lower[0] == 'f' && lower[1] == 'r')
    {
        digits = lower + 2, limit = 16;
    }
    else if(lower[0] == 'r')
    {
        digits = lower + 1, limit = 10;
    }
    else
    {
        return false;
    }
    if(*digits == '\0')
    {
        return false;
    }
    unsigned value = 0;
    for(const char *p = digits; *p; p++)
    {

        if(!ISDIGIT(*p))
        {
            return false;
        }
        value = value * 10 + (*p - '0');
        if(value >= limit)
        {
            return false;
        }
    }
    return true;
}

void emex64_output_labelref(FILE *file,
                            const char *name)
{
    if(emex64_register_name_p(name))
    {
        fprintf(file, "\"%s%s\"", user_label_prefix, name);
    }
    else
    {
        fprintf(file, "%s%s", user_label_prefix, name);
    }
}

#include "gt-emex64.h"
