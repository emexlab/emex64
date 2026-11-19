/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2026 emexlab
 */

#ifndef GCC_EMEX64_PROTOS_H
#define GCC_EMEX64_PROTOS_H

extern HOST_WIDE_INT emex64_initial_elimination_offset(int, int);
extern bool emex64_regno_ok_for_base_p(unsigned int, bool);
extern void emex64_expand_prologue(void);
extern void emex64_expand_epilogue(void);
extern bool emex64_sign_return_address_p(void);

extern void emex64_output_labelref(FILE *, const char *);

#ifdef RTX_CODE
extern bool emex64_expand_move(machine_mode, rtx *);
extern void emex64_expand_cbranch(rtx *, machine_mode);
extern void emex64_expand_cstore(rtx *, machine_mode);
extern void emex64_expand_call(rtx, rtx, rtx);
extern const char *emex64_output_call_args(rtx, rtx);
extern void emex64_expand_eh_return(rtx);
extern void emex64_expand_atomic_cas(rtx *);
extern rtx emex64_return_addr(int, rtx);
extern const char *emex64_output_cf_branch(rtx, rtx, bool);
extern const char *emex64_output_scc(rtx *);
extern const char *emex64_output_symbolic_move(rtx *);
extern const char *emex64_output_const_move(rtx *);
extern bool emex64_neg_imm_p(rtx);
extern bool emex64_dw_neg_imm_p(rtx);
extern const char *emex64_output_sign_extend(rtx *, int);
extern const char *emex64_output_cbranch(rtx *, bool);
extern bool emex64_symbolic_operand_p(rtx);
extern bool emex64_tls_symbol_p(rtx);
extern bool emex64_local_symbolic_p(rtx);
extern bool emex64_tpoff_const_p(rtx);
#endif /* RTX_CODE */

#endif /* GCC_EMEX64_PROTOS_H */
