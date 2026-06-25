/*
 * MIT License
 *
 * Copyright (c) 2026 emexlab
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <stdio.h>
#include <fcntl.h>
#include <emex64lib/support/diagnostic/log.h>
#include <emex64lib/asm/invocation.h>
#include <emex64lib/linker/linker.h>
#include <emex64lib/vm/machine.h>

static inline emex_file_t *emex_file_alloc_vopen(const char *path,
                                                 emex_file_policy_t policy)
{
    /* opening a virtual file descriptor */
    vfd_t *d = vfd_vopen();
    if(d == NULL)
    {
        return NULL;
    }

    /*
     * creating a file backed by the virtual file descriptor
     * which is backed by a vpageobj_t.
     */
    emex_file_t *file = emex_file_alloc_vfd(path, policy, d);
    vfd_close(d);
    if(file == NULL)
    {
        return NULL;
    }

    /* file made a duplication of it */
    return file;
}

int main(void)
{
    /*
     * allocating all necessary virtual files to
     * assemble to a virtual object file we can
     * then link.
     */
    emex_file_t *unsaved_file = emex_file_alloc_unsaved("test.e64", in_data_file_policy,
        "section .data\n"
        "    msg db \"hello, world!\\r\\n\\0\"\n"
        "\n"
        "_start:\n"
        "    mov r0, msg\n"
        ".loop:\n"
        "    ldb r1, r0\n"
        ".retry:\n"
        "    ldd r2, 0x0020000000008008\n"
        "    and r2, 0x02\n"
        "    bz r2, .retry\n"
        "    std 0x0020000000008000, r1\n"
        "    inc r0\n"
        "    bnz r1, .loop\n"
        ".end:\n"
        "    stb 0x002000000000C000, 0\n"
    );
    if(unsaved_file == NULL)
    {
        diagnostic_report(NULL, kDiagnosticSeverityFatal, NULL, "failed to allocate virtual assembly file");
        return 1;
    }

    emex_file_t *object_file = emex_file_alloc_vopen("test.o", out_data_file_policy);
    if(object_file == NULL)
    {
        diagnostic_report(NULL, kDiagnosticSeverityFatal, NULL, "failed to allocate virtual object file");
        emex_file_dealloc(unsaved_file);
        return 1;
    }

    /* we need the invocation to assemble */
    assembler_diagnostic_consumer_t *consumer = assembler_diagnostic_consumer_alloc();
    if(consumer == NULL)
    {
        diagnostic_report(NULL, kDiagnosticSeverityFatal, NULL, "failed to allocate consumer for assembler invocation");
        emex_file_dealloc(object_file);
        emex_file_dealloc(unsaved_file);
        return 1;
    }

    assembler_invocation_t *inv = assembler_invocation_alloc(assembler_invocation_options_default, consumer);
    if(inv == NULL)
    {
        diagnostic_report(NULL, kDiagnosticSeverityFatal, NULL, "failed to allocate assembler invocation");
        assembler_diagnostic_consumer_dealloc(consumer);
        emex_file_dealloc(object_file);
        emex_file_dealloc(unsaved_file);
        return 1;
    }

    bool success = assembler_invocation_emit(inv, unsaved_file, object_file);
    assembler_invocation_dealloc(inv);
    assembler_diagnostic_consumer_dealloc(consumer);
    emex_file_dealloc(unsaved_file);
    if(!success)
    {
        diagnostic_report(NULL, kDiagnosticSeverityFatal, NULL, "ouweee =<");
        emex_file_dealloc(object_file);
        return 1;
    }
    else
    {
        vfd_t *d = emex_file_dup_vfd(object_file);
        if(d != NULL)
        {
            struct stat fdstat;
            if(vfd_stat(d, &fdstat) == 0)
            {
                diagnostic_report(NULL, kDiagnosticSeverityNote, NULL, "compiled virtual assembly file into virtual object file");
                fprintf(stderr, "\tvirtual_object_file_size: %llu bytes\n", (uint64_t)fdstat.st_size);
            }
            vfd_close(d);
        }
    }

    /* now we come to linkage >:3 */
    emex_file_t **input_file = calloc(1, sizeof(emex_file_t*));
    if(input_file == NULL)
    {
        diagnostic_report(NULL, kDiagnosticSeverityFatal, NULL, "couldn't allocate input files array for the linker");
        emex_file_dealloc(object_file);
        return 1;
    }
    input_file[0] = object_file;

    emex_file_t *firmware_file = emex_file_alloc_vopen("test.img", out_data_file_policy);
    if(firmware_file == NULL)
    {
        diagnostic_report(NULL, kDiagnosticSeverityFatal, NULL, "failed to allocate virtual firmware file");
        free(input_file);
        emex_file_dealloc(object_file);
        return 1;
    }

    success = linker_link(linker_options_default, input_file, 1, NULL, 0, firmware_file);
    free(input_file);
    emex_file_dealloc(object_file);
    if(!success)
    {
        diagnostic_report(NULL, kDiagnosticSeverityFatal, NULL, "failed to link virtual object file into virtual firmware file");
        emex_file_dealloc(firmware_file);
        return 1;
    }
    else
    {
        diagnostic_report(NULL, kDiagnosticSeverityNote, NULL, "linked virtual object file into virtual firmware file");
    }

    /* let the core spin >:3 */
    emex64_machine_options_t machine_options = emex64_machine_options_default();
    machine_options.display.enabled = false;

    emex64_machine_t *machine = emex64_machine_alloc(machine_options);
    if(machine == NULL)
    {
        diagnostic_report(NULL, kDiagnosticSeverityFatal, NULL, "failed to allocate virtual machine");
        emex_file_dealloc(firmware_file);
        return 1;
    }

    success = emex64_memory_load_image(machine->memory, firmware_file);
    emex_file_dealloc(firmware_file);
    if(!success)
    {
        diagnostic_report(NULL, kDiagnosticSeverityFatal, NULL, "failed to load virtual firmware file");
        return 1;
    }

    emex64_core_execute(machine->core);
    emex64_machine_dealloc(machine);

    return 0;
}
