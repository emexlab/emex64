/*
 * SPDX-License-Identifier: AGPL-3.0-or-later
 *
 * Copyright (C) 2026 emexlab
 *
 * This file is part of emex64.
 *
 * emex64 is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * emex64 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with emex64. If not, see <https://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <fcntl.h>
#include <EmexFoundation/EmexFoundation.h>
#include <EmexToolchain/Support/diagnostic/log.h>
#include <EmexToolchain/ETAssembler/diagnostic/ETAssemblerDiagnosticConsumer.h>
#include <EmexToolchain/ETAssembler/ETAssemblerInvocation.h>
#include <EmexToolchain/ETLinker/linker.h>
#include <EmexToolchain/VM/E64Machine.h>

EFFileRef EFFileCreateWithStringAndPath(EFAllocatorRef allocatorRef,
                                        EFFilePolicy policy,
                                        EFStringRef path,
                                        EFStringRef content)
{
    EFAUTOREL EFURLRef fileURL = EFURLCreateWithString(allocatorRef, path);
    return EFFileCreateWithString(allocatorRef, policy, fileURL, content);
}

SInt32 main(void)
{
    /*
     * allocating all necessary virtual files to
     * assemble to a virtual object file we can
     * then link.
     */
    EFAUTOREL EFFileRef unsaved_file = EFFileCreateWithStringAndPath(kEFAllocatorDefault, EFFilePolicyInData, EFSTR("test.e64"), EFSTR(
        "section .data\n"
        "    msg db \"hello, world!\\n\\0\"\n"
        "\n"
        "_start:\n"
        "    clar\n"
        "    mov r3, 0x0020000000008000\n"
        ".loop:\n"
        "    ldb r1, [msg + r0++]\n"
        ".retry:\n"
        "    ldq r2, [r3 + 8]\n"
        "    bbz r2, 0x02, .retry\n"
        "    stq r3, r1\n"
        "    bnz r1, .loop\n"
        ".end:\n"
        "    stq [r3 + 0x4000], 0\n"
    ));
    if(unsaved_file == NULL)
    {
        diagnostic_report(NULL, kDiagnosticSeverityFatal, NULL, "failed to allocate virtual assembly file");
        return 1;
    }

    EFAUTOREL EFFileRef object_file = EFFileCreateWithStringAndPath(kEFAllocatorDefault, EFFilePolicyOutData, EFSTR("test.o"), EFSTR(""));
    if(object_file == NULL)
    {
        diagnostic_report(NULL, kDiagnosticSeverityFatal, NULL, "failed to allocate virtual object file");
        return 1;
    }

    /* we need the invocation to assemble */
    EFAUTOREL ETAssemblerDiagnosticConsumerRef assemblerDiagnosticConsumer = ETAssemblerDiagnosticConsumerCreate(kEFAllocatorDefault, ETAssemblerDiagnosticOptionsDefault);
    EFAUTOREL ETAssemblerInvocationRef inv = ETAssemblerInvocationCreate(kEFAllocatorDefault, assemblerDiagnosticConsumer);

    Boolean success = ETAssemblerInvocationSetInputFile(inv, unsaved_file) && ETAssemblerInvocationSetOutputFile(inv, object_file) && ETAssemblerInvocationEmit(inv);
    ETAssemblerDiagnosticConsumerEmit(assemblerDiagnosticConsumer);
    if(!success)
    {
        diagnostic_report(NULL, kDiagnosticSeverityFatal, NULL, "ouweee =<");
        return 1;
    }

    /* now we come to linkage >:3 */
    EFFileRef *input_file = calloc(1, sizeof(EFFileRef));
    if(input_file == NULL)
    {
        diagnostic_report(NULL, kDiagnosticSeverityFatal, NULL, "couldn't allocate input files array for the linker");
        return 1;
    }
    input_file[0] = object_file;

    EFAUTOREL EFFileRef firmware_file = EFFileCreateWithStringAndPath(kEFAllocatorDefault, EFFilePolicyOutData, EFSTR("test.img"), EFSTR(""));
    if(firmware_file == NULL)
    {
        diagnostic_report(NULL, kDiagnosticSeverityFatal, NULL, "failed to allocate virtual firmware file");
        free(input_file);
        return 1;
    }

    linker_diagnostic_consumer_t *lnkconsumer = linker_diagnostic_consumer_alloc();
    if(lnkconsumer == NULL)
    {
        diagnostic_report(NULL, kDiagnosticSeverityFatal, NULL, "failed to allocate linkers consumer");
        free(input_file);
        return 1;
    }

    linker_options_t linkerOptions = linker_options_default;
    linkerOptions.verbose = true;
    linkerOptions.use_old_magic = true;
    success = linker_link(linkerOptions, lnkconsumer, input_file, 1, NULL, 0, firmware_file);
    linker_diagnostic_consumer_emit(lnkconsumer);
    linker_diagnostic_consumer_dealloc(lnkconsumer);
    free(input_file);
    if(!success)
    {
        diagnostic_report(NULL, kDiagnosticSeverityFatal, NULL, "failed to link virtual object file into virtual firmware file");
        return 1;
    }

    /* let the core spin >:3 */
    EFAUTOREL E64MachineRef machine = E64MachineCreateWithOptions(kEFAllocatorDefault, E64MachineOptionsMinimal);
    if(machine == NULL)
    {
        diagnostic_report(NULL, kDiagnosticSeverityFatal, NULL, "failed to allocate virtual machine");
        return 1;
    }

    if(!E64MemoryLoadImage(E64MachineGetMemory(machine), firmware_file))
    {
        diagnostic_report(NULL, kDiagnosticSeverityFatal, NULL, "failed to load virtual firmware file into virtual machine");
        return 1;
    }

    return E64CoreExecute(E64MachineGetCore(machine)) == kE64ExceptionNone ? 0 : 1;
}
