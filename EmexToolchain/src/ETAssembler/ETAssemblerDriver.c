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

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <spawn.h>
#include <sys/wait.h>
#include <EmexToolchain/Support/version.h>
#include <EmexToolchain/Support/diagnostic/log.h>
#include <EmexToolchain/Support/ratchet/args.h>
#include <EmexToolchain/ETAssembler/ETAssemblerDriver.h>
#include <EmexToolchain/ETAssembler/invocation.h>
#include <EmexToolchain/ETLinker/driver.h>

typedef struct __ETAssemblerDriver {
    EFObject header;

    EFArrayRef arguments;

    ETAssemblerDriverOptions driverOptions;
    ETAssemblerDiagnosticOptions diagnosticOptions;
    ETAssemblerDiagnosticConsumerRef diagnosticConsumer;
    EFStringRef outputPath;
    EFMutableArrayRef jobs;

    EFIndex inputFileCount;
    emex_file_t **inputFiles;

    EFIndex incDirCount;
    char **incDirs;

    EFIndex tmpPathCount;
    char **tmpPaths;

    EFIndex macroCount;
    assembler_macro_definition_t *macros;

    EFIndex linkerFlagCount;
    char **linkerFlags;
} *__ETAssemblerDriver;

static void __ETAssemblerDriverDeinit(EFObjectRef driverRef)
{
    __ETAssemblerDriver driver = (__ETAssemblerDriver)driverRef;

    for(int i = 0; i < driver->inputFileCount; i++)
    {
        emex_file_dealloc(driver->inputFiles[i]);
    }
    free(driver->inputFiles);

    for(EFIndex i = 0; i < driver->incDirCount; i++)
    {
        free(driver->incDirs[i]);
    }
    free(driver->incDirs);

    for(EFIndex i = 0; i < driver->tmpPathCount; i++)
    {
        unlink(driver->tmpPaths[i]);
        free(driver->tmpPaths[i]);
    }
    free(driver->tmpPaths);

    for(EFIndex i = 0; i < driver->macroCount; i++)
    {
        free(driver->macros[i].match);
        free(driver->macros[i].value);
    }
    free(driver->macros);

    for(int i = 0; i < driver->linkerFlagCount; i++)
    {
        free(driver->linkerFlags[i]);
    }
    free(driver->linkerFlags);

    ETAssemblerDiagnosticConsumerEmit(driver->diagnosticConsumer);
    EFReleaseTry(driver->diagnosticConsumer);
    EFReleaseTry(driver->jobs);
    EFReleaseTry(driver->arguments);
}

EFClass ETAssemblerDriverClass = {
    .name = "ETAssemblerDriver",
    .typeID = kEFNotATypeID,
    .init = NULL,
    .deinit = __ETAssemblerDriverDeinit,
    .equal = NULL,
    .copyDescription = NULL,
    .hash = NULL,
};

static void ETAssemblerDriverRefisterClass(void)
{
    EFClassRegister(&ETAssemblerDriverClass);
}

EFTypeID ETAssemblerDriverGetTypeID(void)
{
    pthread_once_t once = PTHREAD_ONCE_INIT;
    pthread_once(&once, ETAssemblerDriverRefisterClass);
    return ETAssemblerDriverClass.typeID;
}

Boolean __ETAssemblerDriverPredrive(__ETAssemblerDriver driver)
{
    /* better starting with the default assembler options ^^ */
    driver->diagnosticOptions = ETAssemblerDiagnosticOptionsDefault;

    driver->outputPath = NULL;
    driver->inputFileCount = 0;

    driver->incDirCount = 0;
    driver->incDirs = NULL;

    driver->macroCount = 0;
    driver->macros = NULL;

    EFIndex argumentsCount = EFArrayGetCount(driver->arguments);

    driver->inputFiles = calloc(argumentsCount, sizeof(emex_file_t*));
    if(driver->inputFiles == NULL)
    {
        return false;
    }

    for(EFIndex index = 0; index < argumentsCount; index++)
    {
        EFStringRef argument = EFArrayGetValueAtIndex(driver->arguments, index);
        if(argument == NULL)
        {
            return false;
        }

        const char *cArgument = EFStringGetCStringPtr(argument, kEFStringEncodingUTF8);
        if(cArgument == NULL)
        {
            return false;
        }

        if(EFEqual(argument, EFSTR("--help")))
        {
            EFLog(EFSTR(
                "Usage: %@ [options] file...\n"
                "\n"
                "Options:\n"
                "  --help                 Shows this help menu.\n"
                "  --version              Prints version.\n"
                "  --in-process           All jobs are executed within the same process.\n"
                "\n"
                "  -o <output path>       Sets the output file path, is set to \"a.out\" when not passed.\n"
                "  -c                     Assemble the source file, but do not link.\n"
                "  -r                     Assemble all source files to one ELF object.\n"
                "  -v                     Prints verbose assembler log.\n"
                "  -D macro[=<value>]     Defines an assembler macro, set to 1 when no value is given.\n"
                "  -I <dir>               Adds a directory to the include search paths.\n"
                "  -Wl,<arg>,...          Passes the comma separated arguments to the linker.\n"
                "\n"
                "  -fcaret-diagnostics    The assembler will print diagnostics showing their caret positions.\n"
                "  -fcolor-diagnostics    The assembler will print diagnostics with color.\n"
                "                         Each feature flag can be reversed by prefixing it with a \"no\" (i.e -fno-caret-diagnostics).\n"
                "\n"
                "  -Werror                The assembler will treat every warning as a error.\n"
                "  -Wdeprecated           The assembler will print a warning on every as deprecated marked symbol or internal features.\n"
                "                         Each warning flag can be reversed by prefixing it with a \"no\" (i.e -Wno-error).\n"
            ), EFProcessGetCommand(EFProcessCurrent)?: EFSTR("emex64asm"));
            return false;
        }
        else if(EFEqual(argument, EFSTR("--version")))
        {
            EFLog(EFSTR("%@ version %d.%d.%d (%s)\n"), EFProcessGetCommand(EFProcessCurrent)?: EFSTR("emex64asm"), EMEX64_VERSION_MAJOR, EMEX64_VERSION_MINOR, EMEX64_VERSION_PATCH, EMEX64_VERSION_STRING);
            return false;
        }
        else if(EFEqual(argument, EFSTR("-o")) && index + 1 < argumentsCount)
        {
            driver->outputPath = EFArrayGetValueAtIndex(driver->arguments, ++index);
        }
        else if(EFStringEqualRange(argument, EFSTR("-f"), EFRangeMake(0, 2)))
        {
            EFIndex length = EFStringGetLength(argument);
            if(length <= 2)
            {
                ETAssemblerDiagnosticConsumerReport(driver->diagnosticConsumer, kDiagnosticSeverityError, NULL, EFSTR("missing argument to '-f'"));
                return false;
            }
            EFRange flagArgumentRange = EFRangeMake(2, length - 2);

            if(EFStringEqualRange(argument, EFSTR("page-align"), flagArgumentRange) || EFStringEqualRange(argument, EFSTR("no-page-align"), flagArgumentRange))
            {
                EFAUTOREL EFStringRef flagArgument = EFStringCreateCopyWithRange(kEFAllocatorDefault, argument, flagArgumentRange);
                ETAssemblerDiagnosticConsumerReport(driver->diagnosticConsumer, kDiagnosticSeverityWarning, NULL, EFSTR("feature flag '%@' is deprecated, please you equivalents if available"), flagArgument);
            }
            else if(EFStringEqualRange(argument, EFSTR("caret-diagnostics"), flagArgumentRange))
            {
                driver->diagnosticOptions.caret_diagnostics = true;
            }
            else if(EFStringEqualRange(argument, EFSTR("no-caret-diagnostics"), flagArgumentRange))
            {
                driver->diagnosticOptions.caret_diagnostics = false;
            }
            else if(EFStringEqualRange(argument, EFSTR("color-diagnostics"), flagArgumentRange))
            {
                driver->diagnosticOptions.color_diagnostics = true;
            }
            else if(EFStringEqualRange(argument, EFSTR("no-color-diagnostics"), flagArgumentRange))
            {
                driver->diagnosticOptions.color_diagnostics = false;
            }
            else
            {
                EFAUTOREL EFStringRef flagArgument = EFStringCreateCopyWithRange(kEFAllocatorDefault, argument, flagArgumentRange);
                ETAssemblerDiagnosticConsumerReport(driver->diagnosticConsumer, kDiagnosticSeverityError, NULL, EFSTR("unknown feature flag '%@'"), flagArgument);
                return false;
            }
        }
        else if(EFStringEqualRange(argument, EFSTR("-Wl,"), EFRangeMake(0, 4)))
        {
            EFIndex length = EFStringGetLength(argument);
            if(length <= 4)
            {
                ETAssemblerDiagnosticConsumerReport(driver->diagnosticConsumer, kDiagnosticSeverityError, NULL, EFSTR("missing argument to '-Wl,'"));
                return false;
            }
            EFRange flagArgumentRange = EFRangeMake(4, length - 4);

            EFAUTOREL EFStringRef flagArgument = EFStringCreateCopyWithRange(kEFAllocatorDefault, argument, flagArgumentRange);
            EFAUTOREL EFArrayRef components = EFStringComponentsSplitBySeparator(flagArgument, EFSTR(","));
            if(components == NULL)
            {
                ETAssemblerDiagnosticConsumerReport(driver->diagnosticConsumer, kDiagnosticSeverityFatal, NULL, EFSTR("out of memory, can't extract arguments from '-Wl,' argument"));
                return false;
            }

            EFIndex flagCount = EFArrayGetCount(components);
            for(EFIndex index = 0; index < flagCount; index++)
            {
                const char *cptr = EFStringGetCStringPtr(EFArrayGetValueAtIndex(components, index), kEFStringEncodingUTF8);
                if(cptr == NULL)
                {
                    ETAssemblerDiagnosticConsumerReport(driver->diagnosticConsumer, kDiagnosticSeverityFatal, NULL, EFSTR("out of memory, can't extract arguments from '-Wl,' argument"));
                    return false;
                }

                driver->linkerFlags = realloc(driver->linkerFlags, (driver->linkerFlagCount + 1) * sizeof(char *));
                driver->linkerFlags[driver->linkerFlagCount++] = strdup(cptr);
            }
        }
        else if(EFStringEqualRange(argument, EFSTR("-W"), EFRangeMake(0, 2)))
        {
            EFIndex length = EFStringGetLength(argument);
            if(length <= 2)
            {
                ETAssemblerDiagnosticConsumerReport(driver->diagnosticConsumer, kDiagnosticSeverityError, NULL, EFSTR("missing argument to '-W'"));
                return false;
            }
            EFRange flagArgumentRange = EFRangeMake(2, length - 2);

            if(EFStringEqualRange(argument, EFSTR("error"), flagArgumentRange))
            {
                driver->diagnosticOptions.warning_error = true;
            }
            else if(EFStringEqualRange(argument, EFSTR("no-error"), flagArgumentRange))
            {
                driver->diagnosticOptions.warning_error = false;
            }
            else if(EFStringEqualRange(argument, EFSTR("deprecated"), flagArgumentRange))
            {
                driver->diagnosticOptions.warning_deprecated = true;
            }
            else if(EFStringEqualRange(argument, EFSTR("no-deprecated"), flagArgumentRange))
            {
                driver->diagnosticOptions.warning_deprecated = false;
            }
            else
            {
                EFAUTOREL EFStringRef flagArgument = EFStringCreateCopyWithRange(kEFAllocatorDefault, argument, flagArgumentRange);
                ETAssemblerDiagnosticConsumerReport(driver->diagnosticConsumer, kDiagnosticSeverityError, NULL, EFSTR("unknown warning flag '%@'"), flagArgument);
                return false;
            }
        }
        else if(EFStringEqualRange(argument, EFSTR("-D"), EFRangeMake(0, 2)))
        {
            EFIndex length = EFStringGetLength(argument);
            if(length <= 2)
            {
                ETAssemblerDiagnosticConsumerReport(driver->diagnosticConsumer, kDiagnosticSeverityError, NULL, EFSTR("missing argument to '-D'"));
                return false;
            }
            EFRange flagArgumentRange = EFRangeMake(2, length - 2);

            EFAUTOREL EFStringRef flagArgument = EFStringCreateCopyWithRange(kEFAllocatorDefault, argument, flagArgumentRange);
            EFAUTOREL EFArrayRef components = EFStringComponentsSplitBySeparator(flagArgument, EFSTR("="));
            if(components == NULL || EFArrayGetCount(components) < 1)
            {
                ETAssemblerDiagnosticConsumerReport(driver->diagnosticConsumer, kDiagnosticSeverityFatal, NULL, EFSTR("out of memory, can't extract arguments from '-D' argument"));
                return false;
            }

            EFStringRef macro = EFArrayGetValueAtIndex(components, 0);
            EFRange macroRange = EFRangeMake(0, EFStringGetLength(macro));
            EFRange valueRange = (EFArrayGetCount(components) > 1) ? EFRangeMake(macroRange.length + 1, EFStringGetLength(flagArgument) - (macroRange.length + 1)) : EFRangeZero;
            EFAUTOREL EFStringRef value = EFRangeIsEqual(valueRange, EFRangeZero) ? EFSTR("1") : EFStringCreateCopyWithRange(kEFAllocatorDefault, flagArgument, valueRange);

            EFIndex macroSlot = driver->macroCount++;
            if(driver->macros == NULL)
            {
                driver->macros = calloc(driver->macroCount, sizeof(assembler_macro_definition_t));
            }
            else
            {
                driver->macros = realloc(driver->macros, driver->macroCount * sizeof(assembler_macro_definition_t));
            }

            driver->macros[macroSlot].match = strdup(EFStringGetCStringPtr(macro, kEFStringEncodingUTF8));
            driver->macros[macroSlot].value = strdup(EFStringGetCStringPtr(value, kEFStringEncodingUTF8));
        }
        else if(EFStringEqualRange(argument, EFSTR("-I"), EFRangeMake(0, 2)))
        {
            const char *dir;
            if(cArgument[2] != '\0')
            {
                dir = cArgument + 2;
            }
            else if(index + 1 < argumentsCount)
            {
                EFStringRef argument = EFArrayGetValueAtIndex(driver->arguments, ++index);
                dir = EFStringGetCStringPtr(argument, kEFStringEncodingUTF8);
            }
            else
            {
                ETAssemblerDiagnosticConsumerReport(driver->diagnosticConsumer, kDiagnosticSeverityError, NULL, EFSTR("missing argument to '-I'"));
                return false;
            }
            driver->incDirs = realloc(driver->incDirs, (driver->incDirCount + 1) * sizeof(char*));
            driver->incDirs[driver->incDirCount++] = strdup(dir);
        }
        else if(EFEqual(argument, EFSTR("-c")))
        {
            driver->driverOptions.assembleOnly = true;
        }
        else if(EFEqual(argument, EFSTR("-v")))
        {
            driver->driverOptions.verbose = true;
        }
        else if(EFEqual(argument, EFSTR("--in-process")))
        {
            driver->driverOptions.inProcess = true;
        }
        else if(EFEqual(argument, EFSTR("-r")))
        {
            driver->driverOptions.emitMode = kEmitModeRelocatableObject;
        }
        else if(!EFStringEqualRange(argument, EFSTR("-"), EFRangeMake(0, 1)))
        {
            emex_file_t *file = emex_file_alloc(cArgument, in_data_file_policy);
            if(file == NULL || !(file->type == kEmexFileTypeAssembly || file->type == kEmexFileTypeAssemblyIncludation || file->type == kEmexFileTypeObject))
            {
                ETAssemblerDiagnosticConsumerReport(driver->diagnosticConsumer, kDiagnosticSeverityError, NULL, EFSTR("unknown or non existing input file '%s'"), cArgument);
                return false;
            }

            driver->inputFiles[driver->inputFileCount++] = file;
        }
        else
        {
            ETAssemblerDiagnosticConsumerReport(driver->diagnosticConsumer, kDiagnosticSeverityError, NULL, EFSTR("unknown option '%@'"), argument);
            return false;
        }
    }

    ETAssemblerDiagnosticConsumerSetDiagnosticOptions(driver->diagnosticConsumer, driver->diagnosticOptions);

    if(driver->inputFileCount <= 0)
    {
        ETAssemblerDiagnosticConsumerReport(driver->diagnosticConsumer, kDiagnosticSeverityError, NULL, EFSTR("no input files"));
        return false;
    }

    if(driver->outputPath == NULL)
    {
        ETAssemblerDiagnosticConsumerReport(driver->diagnosticConsumer, kDiagnosticSeverityWarning, NULL, EFSTR("no output path provided, falling back to 'a.out'"));
        driver->outputPath = EFSTR("a.out");
    }

    return true;
}

static char *__ETAssemblerDriverTemporaryObjectPathForInputPath(__ETAssemblerDriver driver,
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

    driver->tmpPaths = realloc(driver->tmpPaths, (driver->tmpPathCount + 1) * sizeof(char *));
    driver->tmpPaths[driver->tmpPathCount++] = path;

    return path;
}

static Boolean __ETAssemblerDriverAppendLinkerFlag(__ETAssemblerDriver driver,
                                                   const char *flag)
{
    if(flag == NULL)
    {
        return false;
    }

    char *copiedFlag = strdup(flag);
    if(copiedFlag == NULL)
    {
        return false;
    }

    void *nptr = realloc(driver->linkerFlags, (driver->linkerFlagCount + 1) * sizeof(char *));
    if(nptr == NULL)
    {
        free(copiedFlag);
        return false;
    }

    driver->linkerFlags[driver->linkerFlagCount++] = copiedFlag;
    return true;
}

Boolean __ETAssemblerDriverJobgen(__ETAssemblerDriver driver)
{
    /* -c is only meant to assemble one assembly file to a object file */
    if(driver->driverOptions.assembleOnly && driver->inputFileCount > 1)
    {
        ETAssemblerDiagnosticConsumerReport(driver->diagnosticConsumer, kDiagnosticSeverityFatal, NULL, EFSTR("multiple input files were passed in object emit mode"));
        return false;
    }

    /* creating assembler jobs */
    for(int i = 0; i < driver->inputFileCount; i++)
    {
        const char *input_path = driver->inputFiles[i]->path;
        kEmexFileType input_type = driver->inputFiles[i]->type;

        switch(input_type)
        {
            case kEmexFileTypeAssembly:
            case kEmexFileTypeAssemblyIncludation:
            {
                ratchet_args_t ra;
                if(!ratchet_args_init(&ra))
                {
                    ETAssemblerDiagnosticConsumerReport(driver->diagnosticConsumer, kDiagnosticSeverityFatal, NULL, EFSTR("out of memory, can't allocate arguments array for assembler job"));
                    ratchet_args_deinit(&ra);
                    return false;
                }

                if(driver->driverOptions.verbose)
                {
                    ratchet_args_append(&ra, "-v");
                }
                ratchet_args_append(&ra, "-c");
                ratchet_args_append(&ra, "-o");
                ratchet_args_append(&ra, __ETAssemblerDriverTemporaryObjectPathForInputPath(driver, input_path));
                ratchet_args_append(&ra, input_path);

                /* feature flags */
                ratchet_args_append(&ra, driver->diagnosticOptions.caret_diagnostics ? "-fcaret-diagnostics" : "-fno-caret-diagnostics");
                ratchet_args_append(&ra, driver->diagnosticOptions.color_diagnostics ? "-fcolor-diagnostics" : "-fno-color-diagnostics");

                /* warning flags */
                ratchet_args_append(&ra, driver->diagnosticOptions.warning_error ? "-Werror" : "-Wno-error");
                ratchet_args_append(&ra, driver->diagnosticOptions.warning_deprecated ? "-Wdeprecated" : "-Wno-deprecated");

                for(EFIndex j = 0; j < driver->incDirCount; j++)
                {
                    size_t ilen = strlen(driver->incDirs[j]);
                    char *new_buf = malloc(ilen + 3);
                    if(new_buf == NULL)
                    {
                        ratchet_args_deinit(&ra);
                        return false;
                    }
                    snprintf(new_buf, ilen + 3, "-I%s", driver->incDirs[j]);
                    ratchet_args_append(&ra, new_buf);
                    free(new_buf);
                }
                for(EFIndex j = 0; j < driver->macroCount; j++)
                {
                    const char *m = driver->macros[j].match;
                    const char *v = driver->macros[j].value;

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
                    ETAssemblerDiagnosticConsumerReport(driver->diagnosticConsumer, kDiagnosticSeverityFatal, NULL, EFSTR("out of memory, can't allocate arguments array for assembler job"));
                    ratchet_args_deinit(&ra);
                    return false;
                }

                EFAUTOREL ETAssemblerJobRef job = ETAssemblerJobCreate(kEFAllocatorDefault, (driver->driverOptions.assembleOnly) ? kETAssemblerJobTypeAssembler : kETAssemblerJobTypeDriver, EFSTR("emex64asm"), ra.array);
                ratchet_args_deinit(&ra);
                if(job == NULL)
                {
                    ETAssemblerDiagnosticConsumerReport(driver->diagnosticConsumer, kDiagnosticSeverityFatal, NULL, EFSTR("out of memory, can't allocate assembler job"));
                    return false;
                }

                if(!EFArrayAppendValue(driver->jobs, job))
                {
                    ETAssemblerDiagnosticConsumerReport(driver->diagnosticConsumer, kDiagnosticSeverityFatal, NULL, EFSTR("out of memory, can't allocate assembler job"));
                    return false;
                }
                break;
            }
            case kEmexFileTypeObject:
                if(!__ETAssemblerDriverAppendLinkerFlag(driver, input_path))
                {
                    ETAssemblerDiagnosticConsumerReport(driver->diagnosticConsumer, kDiagnosticSeverityFatal, NULL, EFSTR("out of memory, can't append object input path to linker flags"));
                    return false;
                }
                break;
            default:
                return false;
        }
    }

    /* we only need a linker job when we got objects to link */
    if(!driver->driverOptions.assembleOnly && driver->tmpPathCount > 0)
    {
        ratchet_args_t ra;
        if(!ratchet_args_init(&ra))
        {
            ETAssemblerDiagnosticConsumerReport(driver->diagnosticConsumer, kDiagnosticSeverityFatal, NULL, EFSTR("out of memory, can't allocate arguments array for linker job"));
            ratchet_args_deinit(&ra);
            return false;
        }

        if(driver->driverOptions.verbose)
        {
            ratchet_args_append(&ra, "-v");
        }
        if(driver->driverOptions.emitMode == kEmitModeRelocatableObject)
        {
            ratchet_args_append(&ra, "-r");
        }
        ratchet_args_append(&ra, "-o");
        ratchet_args_efappend(&ra, driver->outputPath);
        for(EFIndex i = 0; i < driver->tmpPathCount; i++)
        {
            ratchet_args_append(&ra, driver->tmpPaths[i]);
        }
        for(EFIndex i = 0; i < driver->linkerFlagCount; i++)
        {
            ratchet_args_append(&ra, driver->linkerFlags[i]);
        }

        if(ra.failed)
        {
            ETAssemblerDiagnosticConsumerReport(driver->diagnosticConsumer, kDiagnosticSeverityFatal, NULL, EFSTR("out of memory, can't allocate arguments array for linker job"));
            ratchet_args_deinit(&ra);
            return false;
        }

        EFAUTOREL ETAssemblerJobRef job = ETAssemblerJobCreate(kEFAllocatorDefault, kETAssemblerJobTypeLinker, EFSTR("emex64ld"), ra.array);
        ratchet_args_deinit(&ra);
        if(job == NULL)
        {
            ETAssemblerDiagnosticConsumerReport(driver->diagnosticConsumer, kDiagnosticSeverityFatal, NULL, EFSTR("out of memory, can't allocate assembler job"));
            return false;
        }

        if(!EFArrayAppendValue(driver->jobs, job))
        {
            ETAssemblerDiagnosticConsumerReport(driver->diagnosticConsumer, kDiagnosticSeverityFatal, NULL, EFSTR("out of memory, can't allocate assembler job"));
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

const char *assembler_emit_mode_string_for_mode(kEmitMode mode)
{
    switch(mode)
    {
        case kEmitModeFirmware:
            return "firmware image";
        case kEmitModeRelocatableObject:
            return "ELF";
        default:
            return "unknown";
    }
}

ETAssemblerDriverRef ETAssemblerDriverCreate(EFAllocatorRef allocatorRef,
                                             EFArrayRef arguments)
{
    return ETAssemblerDriverCreateWithOptions(allocatorRef, arguments, ETAssemblerDriverOptionsDefault, ETAssemblerDiagnosticOptionsDefault);
}

ETAssemblerDriverRef ETAssemblerDriverCreateWithOptions(EFAllocatorRef allocatorRef,
                                                        EFArrayRef arguments,
                                                        ETAssemblerDriverOptions driverOptions,
                                                        ETAssemblerDiagnosticOptions diagnosticOptions)
{
    EFAUTOREL __ETAssemblerDriver driver = (__ETAssemblerDriver)EFObjectAlloc(allocatorRef, ETAssemblerDriverGetTypeID(), sizeof(struct __ETAssemblerDriver));
    if(driver == NULL)
    {
        return NULL;
    }

    driver->jobs = EFArrayCreateMutable(allocatorRef, kEFArrayCallbacksObjectCallbacks, 0);
    if(driver->jobs == NULL)
    {
        return NULL;
    }

    driver->arguments = EFArrayCreateCopy(allocatorRef, arguments);
    if(driver->arguments == NULL)
    {
        return NULL;
    }

    driver->driverOptions = driverOptions;
    driver->diagnosticOptions = diagnosticOptions;
    driver->diagnosticConsumer = ETAssemblerDiagnosticConsumerCreate(kEFAllocatorDefault, driver->diagnosticOptions);
    if(driver->diagnosticConsumer == NULL)
    {
        return NULL;
    }

    if(!__ETAssemblerDriverPredrive(driver) ||
       !__ETAssemblerDriverJobgen(driver))
    {
        return NULL;
    }

    EFStringRef command = EFProcessGetCommand(EFProcessCurrent);
    if(command == NULL)
    {
        command = EFSTR("emex64asm");
    }

    if(driver->driverOptions.verbose)
    {
        fprintf(stderr, "%s driver version %d.%d.%d (%s)\n", EFStringGetCStringPtr(command, kEFStringEncodingUTF8), EMEX64_VERSION_MAJOR, EMEX64_VERSION_MINOR, EMEX64_VERSION_PATCH, EMEX64_VERSION_STRING);
        fprintf(stderr, "pid: %d\n", getpid());
        fprintf(stderr, "ppid: %d\n", getppid());
        fprintf(stderr, "uid: %d\n", getuid());
        fprintf(stderr, "gid: %d\n", getgid());
        fprintf(stderr, "driverOptions: {\n");
        fprintf(stderr, "    assembleOnly: %d,\n", driver->driverOptions.assembleOnly);
        fprintf(stderr, "    verbose: %d,\n", driver->driverOptions.verbose);
        fprintf(stderr, "    inProcess: %d,\n", driver->driverOptions.inProcess || driver->driverOptions.assembleOnly);
        fprintf(stderr, "    emitMode: %s,\n", assembler_emit_mode_string_for_mode(driver->driverOptions.emitMode));
        fprintf(stderr, "}\n");
        fprintf(stderr, "diagnosticOptions: {\n");
        fprintf(stderr, "    caret_diagnostics: %d,\n", driver->diagnosticOptions.caret_diagnostics);
        fprintf(stderr, "    color_diagnostics: %d,\n", driver->diagnosticOptions.color_diagnostics);
        fprintf(stderr, "    warning_error: %d,\n", driver->diagnosticOptions.warning_error);
        fprintf(stderr, "    warning_deprecated: %d,\n", driver->diagnosticOptions.warning_deprecated);
        fprintf(stderr, "}\n");
        fprintf(stderr, "output_path: %s\n", EFStringGetCStringPtr(driver->outputPath, kEFStringEncodingUTF8));

        fprintf(stderr, "inputFile[%ld]: { ", driver->inputFileCount);
        for(EFIndex i = 0; i < driver->inputFileCount; i++)
        {
            if(i != 0)
            {
                fprintf(stderr, ", ");
            }
            fprintf(stderr, "%s", driver->inputFiles[i]->path);
        }
        fprintf(stderr, " }\n");

        fprintf(stderr, "incDirs[%ld]: { ", driver->incDirCount);
        for(EFIndex i = 0; i < driver->incDirCount; i++)
        {
            if(i != 0)
            {
                fprintf(stderr, ", ");
            }
            fprintf(stderr, "%s", driver->incDirs[i]);
        }
        fprintf(stderr, " }\n");

        fprintf(stderr, "macros[%ld]: { ", driver->macroCount);
        for(EFIndex i = 0; i < driver->macroCount; i++)
        {
            if(i != 0)
            {
                fprintf(stderr, ", ");
            }
            fprintf(stderr, "(match='%s' | replacement='%s')", driver->macros[i].match, driver->macros[i].value);
        }
        fprintf(stderr, " }\n");

        if(!driver->driverOptions.assembleOnly)
        {
            fprintf(stderr, "linkerFlags[%ld]: { ", driver->linkerFlagCount);
            for(EFIndex i = 0; i < driver->linkerFlagCount; i++)
            {
                if(i != 0)
                {
                    fprintf(stderr, ", ");
                }
                fprintf(stderr, "%s", driver->linkerFlags[i]);
            }
            fprintf(stderr, " }\n");
        }
        fprintf(stderr, "\n");
    }

    return (ETAssemblerDriverRef)EFAUTOTRANSFER(driver);
}

extern char **environ;

Boolean ETAssemblerDriverRun(ETAssemblerDriverRef driverRef)
{
    __ETAssemblerDriver driver = (__ETAssemblerDriver)driverRef;
    if(driver == NULL)
    {
        return false;
    }

    ETAssemblerDiagnosticConsumerEmit(driver->diagnosticConsumer);

    if(driver->driverOptions.assembleOnly)
    {
        assembler_diagnostic_consumer_t *consumer = ETAssemblerDiagnosticConsumerGetPtr(driver->diagnosticConsumer);
        if(consumer == NULL)
        {
            return false;
        }

        assembler_invocation_t *inv = assembler_invocation_alloc(consumer);
        if(inv == NULL)
        {
            return false;
        }

        inv->definition_cnt = driver->macroCount;
        inv->definition = driver->macros;
        inv->include_dir_cnt = driver->incDirCount;
        inv->include_dirs = driver->incDirs;

        emex_file_t *output = emex_file_alloc(EFStringGetCStringPtr(driver->outputPath, kEFStringEncodingUTF8), out_data_file_policy);
        if(output == NULL)
        {
            emex_file_dealloc(output);
            assembler_invocation_dealloc(inv);
            return false;
        }

        Boolean success = assembler_invocation_emit(inv, driver->inputFiles[0], output);

        emex_file_dealloc(output);
        assembler_invocation_dealloc(inv);

        return success;
    }
    else
    {
        EFIndex count = EFArrayGetCount(driver->jobs);
        for(EFIndex index = 0; index < count; index++)
        {
            ETAssemblerJobRef job = EFArrayGetValueAtIndex(driver->jobs, index);
            ETAssemblerJobType jobType = ETAssemblerJobGetType(job);
            EFArrayRef jobArguments = ETAssemblerJobGetArguments(job);
            EFStringRef jobCommand = ETAssemblerJobGetCommand(job);

            const char *commandPtr = EFStringGetCStringPtr(jobCommand, kEFStringEncodingASCII);
            if(commandPtr == NULL)
            {
                return false;
            }

            EFIndex argumentsCount = EFArrayGetCount(jobArguments) + 1;
            const char *argv[argumentsCount + 1];
            argv[0] = commandPtr;
            for(EFIndex argumentsIndex = 0; argumentsIndex < (argumentsCount - 1); argumentsIndex++)
            {
                const char *cptr = EFStringGetCStringPtr(EFArrayGetValueAtIndex(jobArguments, argumentsIndex), kEFStringEncodingASCII);
                if(cptr == NULL)
                {
                    return false;
                }
                argv[argumentsIndex + 1] = cptr;
            }
            argv[argumentsCount] = NULL;

            if(jobType == kETAssemblerJobTypeDriver && driver->driverOptions.inProcess)
            {
                EFAUTOREL ETAssemblerDriverRef subDriver = ETAssemblerDriverCreate(EFGetAllocator(driverRef), jobArguments);
                if(subDriver == NULL || !ETAssemblerDriverRun(subDriver))
                {
                    return false;
                }
            }
            else if(jobType == kETAssemblerJobTypeLinker && driver->driverOptions.inProcess)
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
                    ETAssemblerDiagnosticConsumerReport(driver->diagnosticConsumer, kDiagnosticSeverityFatal, NULL, EFSTR("couldn't spawn job: %s"), strerror(errno));
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
                    ETAssemblerDiagnosticConsumerReport(driver->diagnosticConsumer, kDiagnosticSeverityFatal, NULL, EFSTR("job (command='%s' | pid=%d) terminated by signal %d"), commandPtr, pid, WTERMSIG(rstatus));
                    return false;
                }
            }
        }

        return true;
    }

    return false;
}
