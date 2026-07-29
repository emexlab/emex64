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
#include <EmexToolchain/ETAssembler/ETAssemblerDiagnosticConsumer.h>
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
    /* virtual ELF object file */
    EFAUTOREL EFFileRef firmwareFile = EFFileCreateWithStringAndPath(kEFAllocatorDefault, EFFilePolicyOutData, EFSTR("test.img"), EFSTR(""));

    /* building pipeline */
    {
        /* ELF object file shall be available during assembling and linking */
        EFAUTOREL EFFileRef objectFile = EFFileCreateWithStringAndPath(kEFAllocatorDefault, EFFilePolicyOutData, EFSTR("test.o"), EFSTR(""));

        /* assembling */
        {
            /*
             * allocating all necessary virtual files to
             * assemble to a virtual object file we can
             * then link.
             */
            EFAUTOREL EFFileRef unsavedFile = EFFileCreateWithStringAndPath(kEFAllocatorDefault, EFFilePolicyInData, EFSTR("test.e64"), EFSTR(
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
            if(objectFile == NULL || firmwareFile == NULL || unsavedFile == NULL)
            {
                diagnostic_report(NULL, kDiagnosticSeverityFatal, NULL, "failed to allocate virtual files");
                return 1;
            }

            /* we need the invocation to assemble */
            EFAUTOREL ETAssemblerDiagnosticConsumerRef diagnosticConsumer = ETAssemblerDiagnosticConsumerCreate(kEFAllocatorDefault, ETAssemblerDiagnosticOptionsDefault);
            EFAUTOREL ETAssemblerInvocationRef inv = ETAssemblerInvocationCreate(kEFAllocatorDefault, diagnosticConsumer);

            Boolean success = ETAssemblerInvocationSetInputFile(inv, unsavedFile) && ETAssemblerInvocationSetOutputFile(inv, objectFile) && ETAssemblerInvocationEmit(inv);
            ETAssemblerDiagnosticConsumerEmit(diagnosticConsumer);
            if(!success)
            {
                diagnostic_report(NULL, kDiagnosticSeverityFatal, NULL, "ouweee =<");
                return 1;
            }
        }

        /* linking */
        {
            EFAUTOREL EFMutableArrayRef inputFiles = EFArrayCreateMutable(kEFAllocatorDefault, kEFArrayCallbacksObjectCallbacks, 1);
            if(!EFArrayAppendValue(inputFiles, objectFile))
            {
                diagnostic_report(NULL, kDiagnosticSeverityFatal, NULL, "couldn't allocate input files array for the linker");
                return 1;
            }

            linker_diagnostic_consumer_t *diagnosticConsumer = linker_diagnostic_consumer_alloc();
            if(diagnosticConsumer == NULL)
            {
                diagnostic_report(NULL, kDiagnosticSeverityFatal, NULL, "failed to allocate linker's diagnostic consumer");
                return 1;
            }

            linker_options_t linkerOptions = linker_options_default;
            linkerOptions.verbose = true;
            linkerOptions.use_old_magic = true;
            linker_invocation_t *inv = linker_invocation_alloc(linkerOptions, diagnosticConsumer);
            if(inv == NULL)
            {
                diagnostic_report(NULL, kDiagnosticSeverityFatal, NULL, "failed to allocate linker's invocation");
                return 1;
            }

            Boolean success = linker_link(inv, inputFiles, NULL, firmwareFile);
            linker_invocation_dealloc(inv);
            linker_diagnostic_consumer_emit(diagnosticConsumer);
            linker_diagnostic_consumer_dealloc(diagnosticConsumer);
            if(!success)
            {
                diagnostic_report(NULL, kDiagnosticSeverityFatal, NULL, "failed to link virtual object file into virtual firmware file");
                return 1;
            }
        }
    }

    /* let the core spin >:3 */
    EFAUTOREL E64MachineRef machine = E64MachineCreateWithOptions(kEFAllocatorDefault, E64MachineOptionsMinimal);
    if(machine == NULL)
    {
        diagnostic_report(NULL, kDiagnosticSeverityFatal, NULL, "failed to allocate virtual machine");
        return 1;
    }

    if(!E64MemoryLoadImage(E64MachineGetMemory(machine), firmwareFile))
    {
        diagnostic_report(NULL, kDiagnosticSeverityFatal, NULL, "failed to load virtual firmware file into virtual machine");
        return 1;
    }

    return E64CoreExecute(E64MachineGetCore(machine)) == kE64ExceptionNone ? 0 : 1;
}
