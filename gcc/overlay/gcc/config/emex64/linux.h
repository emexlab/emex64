/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2026 emexlab
 */

#ifndef GCC_EMEX64_LINUX_H
#define GCC_EMEX64_LINUX_H

#undef TARGET_OS_CPP_BUILTINS
#define TARGET_OS_CPP_BUILTINS()            \
    do {                                    \
        GNU_USER_TARGET_OS_CPP_BUILTINS (); \
    } while (0)

#define GLIBC_DYNAMIC_LINKER "/lib/ld-linux-emex64.so.1"
#undef MUSL_DYNAMIC_LINKER
#define MUSL_DYNAMIC_LINKER "/lib/ld-musl-emex64.so.1"

#undef CPP_SPEC
#define CPP_SPEC "%{pthread:-D_REENTRANT}"

#undef LINK_SPEC
#define LINK_SPEC "\
%{shared} \
  %{!shared: \
    %{!static: \
      %{!static-pie: \
    %{rdynamic:-export-dynamic} \
    -dynamic-linker " GNU_USER_DYNAMIC_LINKER "}} \
    %{static:-static} %{static-pie:-static -pie --no-dynamic-linker -z text}}"

#undef TARGET_ASM_FILE_END
#define TARGET_ASM_FILE_END file_end_indicate_exec_stack

#endif /* GCC_EMEX64_LINUX_H */
