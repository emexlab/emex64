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
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <spawn.h>
#include <sys/wait.h>
#include <EmexToolchain/support/version.h>
#include <EmexToolchain/support/diagnostic/log.h>
#include <EmexToolchain/support/ratchet/args.h>
#include <EmexToolchain/asm/driver.h>
#include <EmexToolchain/asm/invocation.h>
#include <EmexToolchain/linker/driver.h>

extern char **environ;

typedef struct __ETAssemblerJob {
    EFObject header;
    ETAssemblerJobType type;
    EFStringRef command;
    EFArrayRef arguments;
} *__ETAssemblerJob;

static void __ETAssemblerJobDeinit(EFObjectRef jobRef)
{
    __ETAssemblerJob job = (__ETAssemblerJob)jobRef;
    EFRelease(job->command);
    EFRelease(job->arguments);
}

static EFClass ETAssemblerJobClass = {
    .name = "ETAssemblerJob",
    .typeID = kEFNotATypeID,
    .init = NULL,
    .deinit = __ETAssemblerJobDeinit,
    .equal = NULL,
    .copyDescription = NULL,
};

static void ETAssemblerJobRegisterClass(void)
{
    EFClassRegister(&ETAssemblerJobClass);
}

EFTypeID ETAssemblerJobGetTypeID(void)
{
    static pthread_once_t once = PTHREAD_ONCE_INIT;
    pthread_once(&once, ETAssemblerJobRegisterClass);
    return ETAssemblerJobClass.typeID;
}

ETAssemblerJobRef ETAssemblerJobCreate(EFAllocatorRef allocatorRef,
                                       ETAssemblerJobType type,
                                       EFStringRef command,
                                       EFArrayRef arguments)
{
    if(command == NULL || arguments == NULL)
    {
        return NULL;
    }

    EFStringRef ownedCommand = EFRetain(command);
    if(ownedCommand == NULL)
    {
        return NULL;
    }

    EFArrayRef ownedArguments = EFArrayCreateCopy(allocatorRef, arguments);
    if(ownedArguments == NULL)
    {
        EFRelease(ownedCommand);
        return NULL;
    }

    __ETAssemblerJob job = (__ETAssemblerJob)EFObjectAlloc(allocatorRef, ETAssemblerJobGetTypeID(), sizeof(struct __ETAssemblerJob));
    if(job == NULL)
    {
        EFRelease(ownedArguments);
        EFRelease(ownedCommand);
        return NULL;
    }

    job->command = ownedCommand;
    job->arguments = ownedArguments;

    return (ETAssemblerJobRef)job;
}

ETAssemblerJobType ETAssemblerJobGetType(ETAssemblerJobRef jobRef)
{
    __ETAssemblerJob job = (__ETAssemblerJob)jobRef;
    if(job == NULL)
    {
        return kETAssemblerJobTypeUnknown;
    }

    return job->type;
}

EFStringRef ETAssemblerJobGetCommand(ETAssemblerJobRef jobRef)
{
    __ETAssemblerJob job = (__ETAssemblerJob)jobRef;
    if(job == NULL)
    {
        return NULL;
    }

    return job->command;
}
EFArrayRef ETAssemblerJobGetArguments(ETAssemblerJobRef jobRef)
{
    __ETAssemblerJob job = (__ETAssemblerJob)jobRef;
    if(job == NULL)
    {
        return NULL;
    }

    return job->arguments;
}

Boolean assembler_driver_predrive(assembler_driver_t *driver,
                               int argc,
                               const char **argv)
{
    /* better starting with the default assembler options ^^ */
    driver->diagnosticOptions = ETAssemblerDiagnosticOptionsDefault;

    driver->output_path = NULL;
    driver->input_file_count = 0;
    driver->input_file = calloc(argc, sizeof(emex_file_t*));
    if(driver->input_file == NULL)
    {
        return false;
    }

    driver->inc_dir_cnt = 0;
    driver->inc_dirs = NULL;

    driver->macro_cnt = 0;
    driver->macro = NULL;

    for(int i = 1; i < argc; i++)
    {
        if(strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0)
        {
            fprintf(stderr, "Usage: %s [options] file...\n", argv[0]);
            fprintf(stderr, "\n");
            fprintf(stderr, "Options:\n");
            fprintf(stderr, "  --help                 Shows this help menu.\n");
            fprintf(stderr, "  --version              Prints version.\n");
            fprintf(stderr, "  --in-process           All jobs are handled within the same process instead of executing subprocesses.\n");
            fprintf(stderr, "\n");
            fprintf(stderr, "  -o <output path>       Sets the output file path, is set to \"a.out\" when not passed.\n");
            fprintf(stderr, "  -c                     Assemble the source files, but do not link.\n");
            fprintf(stderr, "  -r                     Relocatable object mode, meaning a ELF object will be emitted out of all assembly files.\n");
            fprintf(stderr, "  -v                     Prints verbose driver log.\n");
            fprintf(stderr, "  -D macro[=<value>]     Defines an assembler macro, set to 1 when no value is given.\n");
            fprintf(stderr, "  -I <dir>               Adds a directory to the include search path.\n");
            fprintf(stderr, "  -Wl,<arg>,...          Pass the comma separated arguments to the linker.\n");
            fprintf(stderr, "\n");
            fprintf(stderr, "  -fcaret-diagnostics    The assembler will print diagnostics showing their caret positions.\n");
            fprintf(stderr, "  -fcolor-diagnostics    The assembler will print diagnostics with color.\n");
            fprintf(stderr, "                         Each feature flag can be reversed by prefixing it with a \"no\" (i.e -fno-page-align).\n");
            fprintf(stderr, "\n");
            fprintf(stderr, "  -Werror                The assembler will treat every warning as a error.\n");
            fprintf(stderr, "  -Wdeprecated           The assembler will print a warning on every as deprecated marked symbol or internal features.\n");
            fprintf(stderr, "                         Each warning flag can be reversed by prefixing it with a \"no\" (i.e -Wno-error).\n");
            return false;
        }
        else if(strcmp(argv[i], "--version") == 0)
        {
            fprintf(stderr, "%s version %d.%d.%d (%s)\n", argv[0], EMEX64_VERSION_MAJOR, EMEX64_VERSION_MINOR, EMEX64_VERSION_PATCH, EMEX64_VERSION_STRING);
            return false;
        }
        else if(strcmp(argv[i], "-o") == 0 && i + 1 < argc)
        {
            driver->output_path = argv[++i];
        }
        else if(strncmp(argv[i], "-f", 2) == 0)
        {
            const char *flag;
            if(argv[i][2] != '\0')
            {
                flag = argv[i] + 2;
            }
            else if(i + 1 < argc)
            {
                flag = argv[++i];
            }
            else
            {
                diagnostic_report(driver->consumer, kDiagnosticSeverityError, NULL, "missing argument to '-f'");
                return false;
            }

            if(strcmp(flag, "page-align") == 0 ||
               strcmp(flag, "no-page-align") == 0)
            {
                diagnostic_report(driver->consumer, kDiagnosticSeverityWarning, NULL, "feature flag '%s' is deprecated, please you equivalents if available", flag);
            }
            else if(strcmp(flag, "caret-diagnostics") == 0)
            {
                driver->diagnosticOptions.caret_diagnostics = true;
            }
            else if(strcmp(flag, "no-caret-diagnostics") == 0)
            {
                driver->diagnosticOptions.caret_diagnostics = false;
            }
            else if(strcmp(flag, "color-diagnostics") == 0)
            {
                driver->diagnosticOptions.color_diagnostics = true;
            }
            else if(strcmp(flag, "no-color-diagnostics") == 0)
            {
                driver->diagnosticOptions.color_diagnostics = false;
            }
            else
            {
                diagnostic_report(driver->consumer, kDiagnosticSeverityError, NULL, "unknown feature flag '%s'", flag);
                return false;
            }
        }
        else if(strncmp(argv[i], "-Wl,", 4) == 0)
        {
            const char *p = argv[i] + 4;
            if(*p == '\0')
            {
                diagnostic_report(driver->consumer, kDiagnosticSeverityError, NULL, "missing argument to '-Wl,'");
                return false;
            }

            while(*p != '\0')
            {
                const char *comma = strchr(p, ',');
                size_t len = comma ? (size_t)(comma - p) : strlen(p);

                char *arg = malloc(len + 1);
                memcpy(arg, p, len);
                arg[len] = '\0';

                driver->linker_flags = realloc(driver->linker_flags, (driver->linker_flags_cnt + 1) * sizeof(char *));
                driver->linker_flags[driver->linker_flags_cnt++] = arg;

                p = comma ? comma + 1 : p + len;
            }
        }
        else if(strncmp(argv[i], "-W", 2) == 0)
        {
            const char *flag;
            if(argv[i][2] != '\0')
            {
                flag = argv[i] + 2;
            }
            else if(i + 1 < argc)
            {
                flag = argv[++i];
            }
            else
            {
                diagnostic_report(driver->consumer, kDiagnosticSeverityError, NULL, "missing argument to '-W'");
                return false;
            }

            if(strcmp(flag, "error") == 0)
            {
                driver->diagnosticOptions.warning_error = true;
            }
            else if(strcmp(flag, "no-error") == 0)
            {
                driver->diagnosticOptions.warning_error = false;
            }
            else if(strcmp(flag, "deprecated") == 0)
            {
                driver->diagnosticOptions.warning_deprecated = true;
            }
            else if(strcmp(flag, "no-deprecated") == 0)
            {
                driver->diagnosticOptions.warning_deprecated = false;
            }
            else
            {
                diagnostic_report(driver->consumer, kDiagnosticSeverityError, NULL, "unknown warning flag '%s'", flag);
                return false;
            }
        }
        else if(strncmp(argv[i], "-D", 2) == 0)
        {
            const char *flag;
            if(argv[i][2] != '\0')
            {
                flag = argv[i] + 2;
            }
            else if(i + 1 < argc)
            {
                flag = argv[++i];
            }
            else
            {
                diagnostic_report(driver->consumer, kDiagnosticSeverityError, NULL, "missing argument to '-D'");
                return false;
            }

            const char *eq = strchr(flag, '=');
            char *match = NULL;
            char *value = NULL;
            if(!eq)
            {
                match = strdup(flag);
                size_t len = strlen(match);
                if(len > 0 && (match[len-1] == '"' || match[len-1] == '\''))
                {
                    match[len-1] = '\0';
                }
                value = strdup("1");
                goto early_macro_append;
            }

            size_t name_len = eq - flag;
            match = malloc(name_len + 1);
            memcpy(match, flag, name_len);
            match[name_len] = '\0';

            const char *v = eq + 1;
            char quote = 0;

            if(*v == '"' || *v == '\'')
            {
                quote = *v;
                v++;
            }

            const char *end = v;
            while (*end && *end != quote && *end != '\0') end++;

            size_t val_len = end - v;
            value = malloc(val_len + 1);
            memcpy(value, v, val_len);
            value[val_len] = '\0';

        early_macro_append:
            {
                UInt64 macro_slot = driver->macro_cnt++;
                if(driver->macro == NULL)
                {
                    driver->macro = calloc(driver->macro_cnt, sizeof(assembler_macro_definition_t));
                }
                else
                {
                    driver->macro = realloc(driver->macro, driver->macro_cnt * sizeof(assembler_macro_definition_t));
                }

                driver->macro[macro_slot].match = match;
                driver->macro[macro_slot].value = value;
            }
        }
        else if(strncmp(argv[i], "-I", 2) == 0)
        {
            const char *dir;
            if(argv[i][2] != '\0')
            {
                dir = argv[i] + 2;
            }
            else if(i + 1 < argc)
            {
                dir = argv[++i];
            }
            else
            {
                diagnostic_report(driver->consumer, kDiagnosticSeverityError, NULL, "missing argument to '-I'");
                return false;
            }
            driver->inc_dirs = realloc(driver->inc_dirs, (driver->inc_dir_cnt + 1) * sizeof(char*));
            driver->inc_dirs[driver->inc_dir_cnt++] = strdup(dir);
        }
        else if(strncmp(argv[i], "-c", 2) == 0)
        {
            driver->options.assemble_only = true;
        }
        else if(strncmp(argv[i], "-v", 2) == 0)
        {
            driver->options.verbose = true;
        }
        else if(strncmp(argv[i], "--in-process", 12) == 0)
        {
            driver->options.in_process = true;
        }
        else if(strncmp(argv[i], "-r", 2) == 0)
        {
            driver->emit_mode = kEmitModeRelocatableObject;
        }
        else if(argv[i][0] != '-')
        {
            emex_file_t *file = emex_file_alloc(argv[i], in_data_file_policy);
            if(file == NULL || !(file->type == kEmexFileTypeAssembly || file->type == kEmexFileTypeAssemblyIncludation || file->type == kEmexFileTypeObject))
            {
                diagnostic_report(driver->consumer, kDiagnosticSeverityError, NULL, "unknown or non existing input file '%s'", argv[i]);
                return false;
            }

            driver->input_file[driver->input_file_count++] = file;
        }
        else
        {
            diagnostic_report(driver->consumer, kDiagnosticSeverityError, NULL, "unknown option '%s'", argv[i]);
            return false;
        }
    }

    ((assembler_diagnostic_consumer_context_t*)driver->consumer->ctx)->options = driver->diagnosticOptions;

    if(driver->input_file_count <= 0)
    {
        diagnostic_report(driver->consumer, kDiagnosticSeverityError, NULL, "no input files");
        return false;
    }

    if(driver->output_path == NULL)
    {
        diagnostic_report(driver->consumer, kDiagnosticSeverityWarning, NULL,  "no output path provided, falling back to 'a.out'");
        driver->output_path = "a.out";
    }

    return true;
}

static char *assembler_driver_tmppath(assembler_driver_t *driver,
                                      const char *input_path)
{
    const char *base = strrchr(input_path, '/');
    base = base ? base + 1 : input_path;
    const char *dot = strrchr(base, '.');
    size_t stem_len = dot ? (size_t)(dot - base) : strlen(base);

    const char *tmpdir = getenv("TMPDIR");
    if(tmpdir == NULL || tmpdir[0] == '\0')
    {
        tmpdir = "/tmp";
    }

    size_t len = strlen(tmpdir) + 1 + 7 + stem_len + 1 + 6 + 2 + 1;
    char *path = malloc(len);
    if(path == NULL)
    {
        return NULL;
    }

    snprintf(path, len, "%s/emex64-%.*s-XXXXXX.o", tmpdir, (int)stem_len, base);

    int fd = mkstemps(path, 2);
    if(fd < 0)
    {
        free(path);
        return NULL;
    }
    close(fd);

    driver->tmp_paths = realloc(driver->tmp_paths, (driver->tmp_path_cnt + 1) * sizeof(char *));
    driver->tmp_paths[driver->tmp_path_cnt++] = path;

    return path;
}

static void assembler_driver_append_additional_linker_flag(assembler_driver_t *driver,
                                                           const char *flag)
{
    driver->linker_flags = realloc(driver->linker_flags, (driver->linker_flags_cnt + 1) * sizeof(char *));
    driver->linker_flags[driver->linker_flags_cnt++] = strdup(flag);
}

Boolean assembler_driver_jobgen(assembler_driver_t *driver)
{
    /* -c is only meant to assemble one assembly file to a object file */
    if(driver->options.assemble_only && driver->input_file_count > 1)
    {
        diagnostic_report(driver->consumer, kDiagnosticSeverityError, NULL,  "multiple input files were passed in object emit mode");
        return false;
    }

    /* creating assembler jobs */
    for(int i = 0; i < driver->input_file_count; i++)
    {
        const char *input_path = driver->input_file[i]->path;
        kEmexFileType input_type = driver->input_file[i]->type;

        switch(input_type)
        {
            case kEmexFileTypeAssembly:
            case kEmexFileTypeAssemblyIncludation:
            {
                ratchet_args_t ra;
                ratchet_args_init(&ra);

                ratchet_args_append(&ra, "emex64asm");
                if(driver->options.verbose)
                {
                    ratchet_args_append(&ra, "-v");
                }
                ratchet_args_append(&ra, "-c");
                ratchet_args_append(&ra, "-o");
                ratchet_args_append(&ra, assembler_driver_tmppath(driver, input_path));
                ratchet_args_append(&ra, input_path);

                /* feature flags */
                ratchet_args_append(&ra, driver->diagnosticOptions.caret_diagnostics ? "-fcaret-diagnostics" : "-fno-caret-diagnostics");
                ratchet_args_append(&ra, driver->diagnosticOptions.color_diagnostics ? "-fcolor-diagnostics" : "-fno-color-diagnostics");

                /* warning flags */
                ratchet_args_append(&ra, driver->diagnosticOptions.warning_error ? "-Werror" : "-Wno-error");
                ratchet_args_append(&ra, driver->diagnosticOptions.warning_deprecated ? "-Wdeprecated" : "-Wno-deprecated");


                for(size_t j = 0; j < driver->inc_dir_cnt; j++)
                {
                    size_t ilen = strlen(driver->inc_dirs[j]);
                    char *new_buf = malloc(ilen + 3);
                    if(new_buf == NULL)
                    {
                        ratchet_args_deinit(&ra);
                        return false;
                    }
                    snprintf(new_buf, ilen + 3, "-I%s", driver->inc_dirs[j]);
                    ratchet_args_append(&ra, new_buf);
                    free(new_buf);
                }
                for(UInt64 j = 0; j < driver->macro_cnt; j++)
                {
                    const char *m = driver->macro[j].match;
                    const char *v = driver->macro[j].value;

                    size_t blen = 2 + strlen(m) + 1 + strlen(v) + 1;
                    char *buf = malloc(blen);
                    if(buf == NULL)
                    {
                        ratchet_args_deinit(&ra);
                        return false;
                    }
                    snprintf(buf, blen, "-D%s=%s", m, v);
                    ratchet_args_append(&ra, buf);
                    free(buf);
                }

                if(ra.failed)
                {
                    diagnostic_report(driver->consumer, kDiagnosticSeverityFatal, NULL,  "out of memory, can't allocate arguments array for assembler job");
                    ratchet_args_deinit(&ra);
                    return false;
                }

                EFMutableArrayRef array = EFArrayCreateMutable(kEFAllocatorDefault, kEFArrayCallbacksObjectCallbacks, ra.argc);
                if(array == NULL)
                {
                    diagnostic_report(driver->consumer, kDiagnosticSeverityFatal, NULL,  "out of memory, can't allocate assembler job");
                    ratchet_args_deinit(&ra);
                    return false;
                }

                for(EFIndex index = 0; index < (EFIndex)ra.argc; index++)
                {
                    EFStringRef argument = EFStringCreateWithCString(kEFAllocatorDefault, ra.args[index], kEFStringEncodingASCII);
                    if(argument == NULL)
                    {
                        diagnostic_report(driver->consumer, kDiagnosticSeverityFatal, NULL,  "out of memory, can't allocate assembler job");
                        EFRelease(array);
                        ratchet_args_deinit(&ra);
                        return false;
                    }

                    Boolean success = EFArrayAppendValue(array, argument);
                    EFRelease(argument);
                    if(!success)
                    {
                        diagnostic_report(driver->consumer, kDiagnosticSeverityFatal, NULL,  "out of memory, can't allocate assembler job");
                        EFRelease(array);
                        ratchet_args_deinit(&ra);
                        return false;
                    }
                }

                ETAssemblerJobRef job = ETAssemblerJobCreate(kEFAllocatorDefault, (driver->options.assemble_only) ? kETAssemblerJobTypeAssembler : kETAssemblerJobTypeDriver, EF_STR("emex64asm"), array);
                EFRelease(array);
                ratchet_args_deinit(&ra);
                if(job == NULL)
                {
                    diagnostic_report(driver->consumer, kDiagnosticSeverityFatal, NULL,  "out of memory, can't allocate assembler job");
                    return false;
                }

                Boolean success = EFArrayAppendValue(driver->jobArrayRef, job);
                EFRelease(job);
                if(!success)
                {
                    diagnostic_report(driver->consumer, kDiagnosticSeverityFatal, NULL,  "out of memory, can't allocate assembler job");
                    return false;
                }
                break;
            }
            case kEmexFileTypeObject:
                assembler_driver_append_additional_linker_flag(driver, input_path);
                break;
            default:
                return false;
        }
    }

    /* we only need a linker job when we got objects to link */
    if(!driver->options.assemble_only && driver->tmp_path_cnt > 0)
    {
        ratchet_args_t ra;
        ratchet_args_init(&ra);

        ratchet_args_append(&ra, "emex64ld");
        if(driver->options.verbose)
        {
            ratchet_args_append(&ra, "-v");
        }
        if(driver->emit_mode == kEmitModeRelocatableObject)
        {
            ratchet_args_append(&ra, "-r");
        }
        ratchet_args_append(&ra, "-o");
        ratchet_args_append(&ra, driver->output_path);
        for(size_t i = 0; i < driver->tmp_path_cnt; i++)
        {
            ratchet_args_append(&ra, driver->tmp_paths[i]);
        }
        for(int i = 0; i < driver->linker_flags_cnt; i++)
        {
            ratchet_args_append(&ra, driver->linker_flags[i]);
        }

        if(ra.failed)
        {
            diagnostic_report(driver->consumer, kDiagnosticSeverityFatal, NULL,  "out of memory, can't allocate arguments array for linker job");
            ratchet_args_deinit(&ra);
            return false;
        }

        EFMutableArrayRef array = EFArrayCreateMutable(kEFAllocatorDefault, kEFArrayCallbacksObjectCallbacks, ra.argc);
        if(array == NULL)
        {
            diagnostic_report(driver->consumer, kDiagnosticSeverityFatal, NULL,  "out of memory, can't allocate assembler job");
            ratchet_args_deinit(&ra);
            return false;
        }

        for(EFIndex index = 0; index < (EFIndex)ra.argc; index++)
        {
            EFStringRef argument = EFStringCreateWithCString(kEFAllocatorDefault, ra.args[index], kEFStringEncodingASCII);
            if(argument == NULL)
            {
                diagnostic_report(driver->consumer, kDiagnosticSeverityFatal, NULL,  "out of memory, can't allocate assembler job");
                EFRelease(array);
                ratchet_args_deinit(&ra);
                return false;
            }

            Boolean success = EFArrayAppendValue(array, argument);
            EFRelease(argument);
            if(!success)
            {
                diagnostic_report(driver->consumer, kDiagnosticSeverityFatal, NULL,  "out of memory, can't allocate assembler job");
                EFRelease(array);
                ratchet_args_deinit(&ra);
                return false;
            }
        }

        ETAssemblerJobRef job = ETAssemblerJobCreate(kEFAllocatorDefault, kETAssemblerJobTypeLinker, EF_STR("emex64ld"), array);
        EFRelease(array);
        ratchet_args_deinit(&ra);
        if(job == NULL)
        {
            diagnostic_report(driver->consumer, kDiagnosticSeverityFatal, NULL,  "out of memory, can't allocate assembler job");
            return false;
        }
        
        ratchet_args_deinit(&ra);

        Boolean success = EFArrayAppendValue(driver->jobArrayRef, job);
        EFRelease(job);
        if(!success)
        {
            diagnostic_report(driver->consumer, kDiagnosticSeverityFatal, NULL,  "out of memory, can't allocate assembler job");
            return false;
        }
    }

    return true;
}

const char *assembler_job_string_for_type(ETAssemblerJobType type)
{
    switch(type)
    {
        case kETAssemblerJobTypeAssembler:
            return "assembler";
        case kETAssemblerJobTypeLinker:
            return "linker";
        case kETAssemblerJobTypeDriver:
            return "driver";
        default:
            return "unknown";
    }
}

assembler_driver_t *assembler_driver_alloc(int argc,
                                           const char **argv)
{
    assembler_driver_t *driver = calloc(1, sizeof(assembler_driver_t));
    if(driver == NULL)
    {
        return NULL;
    }

    driver->jobArrayRef = EFArrayCreateMutable(kEFAllocatorDefault, kEFArrayCallbacksObjectCallbacks, 0);
    if(driver->jobArrayRef == NULL)
    {
        free(driver);
        return NULL;
    }

    /* need default settings */
    driver->diagnosticOptions = ETAssemblerDiagnosticOptionsDefault;

    driver->consumer = assembler_diagnostic_consumer_alloc(driver->diagnosticOptions);
    if(driver->consumer == NULL)
    {
        assembler_driver_dealloc(driver);
        return NULL;
    }

    if(!assembler_driver_predrive(driver, argc, argv) ||
       !assembler_driver_jobgen(driver))
    {
        assembler_driver_dealloc(driver);
        return NULL;
    }

    if(driver->options.verbose)
    {
        fprintf(stderr, "%s version %d.%d.%d (%s)\n", argv[0], EMEX64_VERSION_MAJOR, EMEX64_VERSION_MINOR, EMEX64_VERSION_PATCH, EMEX64_VERSION_STRING);
        fprintf(stderr, "pid: %d\n", getpid());
        fprintf(stderr, "uid: %d\n", getuid());
        fprintf(stderr, "options: {\n");
        fprintf(stderr, "    assemble_only: %d,\n", driver->options.assemble_only);
        fprintf(stderr, "    verbose: %d,\n", driver->options.verbose);
        fprintf(stderr, "    in_process: %d,\n", driver->options.in_process);
        fprintf(stderr, "}\n");;
        fprintf(stderr, "diagnosticOptions: {\n");
        fprintf(stderr, "    caret_diagnostics: %d,\n", driver->diagnosticOptions.caret_diagnostics);
        fprintf(stderr, "    color_diagnostics: %d,\n", driver->diagnosticOptions.color_diagnostics);
        fprintf(stderr, "    warning_error: %d,\n", driver->diagnosticOptions.warning_error);
        fprintf(stderr, "    warning_deprecated: %d,\n", driver->diagnosticOptions.warning_deprecated);
        fprintf(stderr, "}\n");;
        fprintf(stderr, "output_path: %s\n", driver->output_path);

        fprintf(stderr, "input_file[%d]: { ", driver->input_file_count);
        for(int i = 0; i < driver->input_file_count; i++)
        {
            if(i != 0)
            {
                fprintf(stderr, ", ");
            }
            fprintf(stderr, "%s", driver->input_file[i]->path);
        }
        fprintf(stderr, " }\n");

        fprintf(stderr, "inc_dirs[%zu]: { ", driver->inc_dir_cnt);
        for(size_t i = 0; i < driver->inc_dir_cnt; i++)
        {
            if(i != 0)
            {
                fprintf(stderr, ", ");
            }
            fprintf(stderr, "%s", driver->inc_dirs[i]);
        }
        fprintf(stderr, " }\n");

        fprintf(stderr, "macro[%llu]: { ", (unsigned long long)driver->macro_cnt);
        for(UInt64 i = 0; i < driver->macro_cnt; i++)
        {
            if(i != 0)
            {
                fprintf(stderr, ", ");
            }
            fprintf(stderr, "(match='%s' | replacement='%s')", driver->macro[i].match, driver->macro[i].value);
        }
        fprintf(stderr, " }\n");

        if(!driver->options.assemble_only)
        {
            fprintf(stderr, "linker_flags[%d]: { ", driver->linker_flags_cnt);
            for(int i = 0; i < driver->linker_flags_cnt; i++)
            {
                if(i != 0)
                {
                    fprintf(stderr, ", ");
                }
                fprintf(stderr, "%s", driver->linker_flags[i]);
            }
            fprintf(stderr, " }\n");
        }

        /*fprintf(stderr, "jobs: {");
        assembler_job_t *job = driver->job;
        if(job != NULL)
        {
            fprintf(stderr, "\n");
        }
        while(job != NULL)
        {
            fprintf(stderr, "\t{\n");
            fprintf(stderr, "\t\ttype: %s\n", assembler_job_string_for_type(job->type));
            fprintf(stderr, "\t\tcommand: %s\n", job->command);
            fprintf(stderr, "\t\targv[%d]: {", job->argc);
            for(int i = 0; i < job->argc; i++)
            {
                if(i != 0)
                {
                    fprintf(stderr, ", ");
                }
                fprintf(stderr, "%s", job->argv[i]);
            }
            fprintf(stderr, " }\n");
            fprintf(stderr, "\t}");
            fprintf(stderr, "\n");

            job = job->next;
        }
        fprintf(stderr, "}\n");*/
    }

    return driver;
}

void assembler_driver_dealloc(assembler_driver_t *driver)
{
    for(int i = 0; i < driver->input_file_count; i++)
    {
        emex_file_dealloc(driver->input_file[i]);
    }
    free(driver->input_file);

    for(size_t i = 0; i < driver->inc_dir_cnt; i++)
    {
        free(driver->inc_dirs[i]);
    }
    free(driver->inc_dirs);

    for(UInt64 i = 0; i < driver->tmp_path_cnt; i++)
    {
        unlink(driver->tmp_paths[i]);
        free(driver->tmp_paths[i]);
    }
    free(driver->tmp_paths);

    for(UInt64 i = 0; i < driver->macro_cnt; i++)
    {
        free(driver->macro[i].match);
        free(driver->macro[i].value);
    }
    free(driver->macro);

    for(int i = 0; i < driver->linker_flags_cnt; i++)
    {
        free(driver->linker_flags[i]);
    }
    free(driver->linker_flags);

    EFRelease(driver->jobArrayRef);

    assembler_diagnostic_consumer_emit(driver->consumer);
    assembler_diagnostic_consumer_dealloc(driver->consumer);
    free(driver);
}

Boolean assembler_driver_drive_the_fucking_car(assembler_driver_t *driver)
{
    if(driver->options.assemble_only)
    {
        assembler_invocation_t *inv = assembler_invocation_alloc(driver->consumer);
        if(inv == NULL)
        {
            return false;
        }

        inv->definition_cnt = driver->macro_cnt;
        inv->definition = driver->macro;
        inv->include_dir_cnt = driver->inc_dir_cnt;
        inv->include_dirs = driver->inc_dirs;

        emex_file_t *output = emex_file_alloc(driver->output_path, out_data_file_policy);
        if(output == NULL)
        {
            emex_file_dealloc(output);
            assembler_invocation_dealloc(inv);
            return false;
        }

        Boolean success = assembler_invocation_emit(inv, driver->input_file[0], output);

        emex_file_dealloc(output);
        assembler_invocation_dealloc(inv);

        return success;
    }
    else
    {
        EFIndex count = EFArrayGetCount(driver->jobArrayRef);
        for(EFIndex index = 0; index < count; index++)
        {
            ETAssemblerJobRef job = EFArrayGetValueAtIndex(driver->jobArrayRef, index);
            ETAssemblerJobType jobType = ETAssemblerJobGetType(job);
            EFArrayRef jobArguments = ETAssemblerJobGetArguments(job);
            EFStringRef jobCommand = ETAssemblerJobGetCommand(job);

            const char *commandPtr = EFStringGetCStringPtr(jobCommand, kEFStringEncodingASCII);
            if(commandPtr == NULL)
            {
                return false;
            }

            EFIndex argumentsCount = EFArrayGetCount(jobArguments);
            const char *argv[argumentsCount + 1];
            for(EFIndex argumentsIndex = 0; argumentsIndex < argumentsCount; argumentsIndex++)
            {
                EFStringRef argument = EFArrayGetValueAtIndex(jobArguments, argumentsIndex);
                const char *cptr = EFStringGetCStringPtr(argument, kEFStringEncodingASCII);
                if(cptr == NULL)
                {
                    return false;
                }

                argv[argumentsIndex] = cptr;
            }
            argv[argumentsCount] = NULL;

            if(jobType == kETAssemblerJobTypeDriver && driver->options.in_process)
            {
                assembler_driver_t *subdriver = assembler_driver_alloc(argumentsCount, (const char**)argv);
                if(subdriver == NULL)
                {
                    return false;
                }

                Boolean success = assembler_driver_drive_the_fucking_car(subdriver);
                assembler_driver_dealloc(subdriver);
                if(!success)
                {
                    return false;
                }
            }
            else if(jobType == kETAssemblerJobTypeLinker && driver->options.in_process)
            {
                linker_driver_t *subdriver = linker_driver_alloc(argumentsCount, (const char**)argv);
                if(subdriver == NULL)
                {
                    return false;
                }

                Boolean success = linker_driver_drive_the_fucking_car(subdriver);
                linker_driver_dealloc(subdriver);
                if(!success)
                {
                    return false;
                }
            }
            else
            {
                pid_t pid = 0;
                if(posix_spawnp(&pid, commandPtr, NULL, NULL, (char *const *)argv, environ) != 0)
                {
                    diagnostic_report(driver->consumer, kDiagnosticSeverityError, NULL,  "failed to spawn it!");
                    return false;
                }

                int rstatus = 0;
                if(waitpid(pid, &rstatus, 0) != pid)
                {
                    return false;
                }

                if(WIFEXITED(rstatus))
                {
                    if(WEXITSTATUS(rstatus) != 0)
                    {
                        return false;
                    }
                }
                else if(WIFSIGNALED(rstatus))
                {
                    diagnostic_report(driver->consumer, kDiagnosticSeverityFatal, NULL,  "job (command='%s' | pid=%d) terminated by signal %d", commandPtr, pid, WTERMSIG(rstatus));
                    return false;
                }
            }
        }

        return true;
    }

    return false;
}
