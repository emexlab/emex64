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
#include <EmexFoundation/EmexFoundation.h>
#include <EmexToolchain/Support/version.h>
#include <EmexToolchain/Support/diagnostic/log.h>
#include <EmexToolchain/Support/ratchet/args.h>
#include <EmexToolchain/ETAssembler/ETAssemblerDriver.h>
#include <EmexToolchain/ETAssembler/ETAssemblerInvocation.h>
#include <EmexToolchain/ETLinker/driver.h>

typedef struct __ETAssemblerDriver {
    EFObject header;

    EFArrayRef arguments;

    ETAssemblerDriverOptions driverOptions;
    ETAssemblerDiagnosticOptions diagnosticOptions;

    ETAssemblerDiagnosticConsumerRef diagnosticConsumer;

    EFMutableArrayRef inputFiles;
    EFStringRef outputPath;

    EFMutableArrayRef includeSearchPaths;
    EFMutableArrayRef temporaryOutputPaths;

    EFMutableArrayRef linkerFlags;

    EFMutableArrayRef jobs;

    EFIndex macroCount;
    assembler_macro_definition_t *macros;
} *__ETAssemblerDriver;

static void __ETAssemblerDriverDeinit(EFObjectRef driverRef)
{
    __ETAssemblerDriver driver = (__ETAssemblerDriver)driverRef;

    for(EFIndex i = 0; i < driver->macroCount; i++)
    {
        free(driver->macros[i].match);
        free(driver->macros[i].value);
    }
    free(driver->macros);

    ETAssemblerDiagnosticConsumerEmit(driver->diagnosticConsumer);
    EFReleaseTry(driver->diagnosticConsumer);
    EFReleaseTry(driver->jobs);
    EFReleaseTry(driver->arguments);
    EFReleaseTry(driver->inputFiles);
    EFReleaseTry(driver->includeSearchPaths);
    EFReleaseTry(driver->linkerFlags);

    /* temporaryOutputPaths have to be unlinked */
    EFIndex temporaryObjectPathCount = EFArrayGetCount(driver->temporaryOutputPaths);
    for(EFIndex index = 0; index < temporaryObjectPathCount; index++)
    {
        unlink(EFStringGetCStringPtr(EFArrayGetValueAtIndex(driver->temporaryOutputPaths, index), kEFStringEncodingUTF8));
    }
    EFReleaseTry(driver->temporaryOutputPaths);
}

static EFStringRef __ETAssemblerDriverCopyDescription(EFObjectRef driverRef)
{
    __ETAssemblerDriver driver = (__ETAssemblerDriver)driverRef;
    return EFStringCreateWithFormat(EFGetAllocator(driverRef), EFSTR("<ETAssemblerDriver %p>{arguments = %@, diagnosticConsumer = %@, inputFiles = %@, outputPath = %@, includeSearchPaths = %@, temporaryOutputPaths = %@, linkerFlags = %@, jobs = %@}"), driverRef, driver->arguments, driver->diagnosticConsumer, driver->inputFiles, driver->outputPath, driver->includeSearchPaths, driver->temporaryOutputPaths, driver->linkerFlags, driver->jobs);
}

static EFClassDefinitionV2 ETAssemblerDriverClass = {
    .header = {
        .version = 2,
        .typeID = kEFTypeIDNone,
        .name = NULL,
    },
    .name = "ETAssemblerDriver",
    .init = NULL,
    .deinit = __ETAssemblerDriverDeinit,
    .equal = NULL,
    .copyDescription = __ETAssemblerDriverCopyDescription,
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
    return ETAssemblerDriverClass.header.typeID;
}

Boolean __ETAssemblerDriverPredrive(__ETAssemblerDriver driver)
{
    /* better starting with the default assembler options ^^ */
    driver->diagnosticOptions = ETAssemblerDiagnosticOptionsDefault;

    driver->outputPath = NULL;

    driver->macroCount = 0;
    driver->macros = NULL;

    EFIndex argumentsCount = EFArrayGetCount(driver->arguments);

    driver->inputFiles = EFArrayCreateMutable(EFGetAllocator(driver), kEFArrayCallbacksObjectCallbacks, argumentsCount);
    if(driver->inputFiles == NULL)
    {
        return false;
    }

    driver->includeSearchPaths = EFArrayCreateMutable(EFGetAllocator(driver), kEFArrayCallbacksObjectCallbacks, argumentsCount);
    if(driver->includeSearchPaths == NULL)
    {
        return false;
    }

    driver->temporaryOutputPaths = EFArrayCreateMutable(EFGetAllocator(driver), kEFArrayCallbacksObjectCallbacks, argumentsCount);
    if(driver->temporaryOutputPaths == NULL)
    {
        return false;
    }

    driver->linkerFlags = EFArrayCreateMutable(EFGetAllocator(driver), kEFArrayCallbacksObjectCallbacks, argumentsCount);
    if(driver->linkerFlags == NULL)
    {
        return false;
    }

    EFStringRef processCommand = EFProcessGetCommand(EFProcessGetCurrentProcess());

    for(EFIndex index = 0; index < argumentsCount; index++)
    {
        EFStringRef argument = EFArrayGetValueAtIndex(driver->arguments, index);
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
            ), processCommand ? processCommand : EFSTR("emex64asm"));
            return false;
        }
        else if(EFEqual(argument, EFSTR("--version")))
        {
            EFLog(EFSTR("%@ version %d.%d.%d (%s)\n"), processCommand ? processCommand : EFSTR("emex64asm"), EMEX64_VERSION_MAJOR, EMEX64_VERSION_MINOR, EMEX64_VERSION_PATCH, EMEX64_VERSION_STRING);
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
                if(!EFArrayAppendValue(driver->linkerFlags, EFArrayGetValueAtIndex(components, index)))
                {
                    ETAssemblerDiagnosticConsumerReport(driver->diagnosticConsumer, kDiagnosticSeverityFatal, NULL, EFSTR("out of memory, can't extract arguments from '-Wl,' argument"));
                    return false;
                }
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
                ETAssemblerDiagnosticConsumerReport(driver->diagnosticConsumer, kDiagnosticSeverityFatal, NULL, EFSTR("out of memory, can't extract arguments from '-D'"));
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
            EFAUTOREL EFStringRef flagArgument = NULL;
            EFIndex length = EFStringGetLength(argument);
            if(length > 2)
            {
                flagArgument = EFStringCreateCopyWithRange(kEFAllocatorDefault, argument, EFRangeMake(2, length - 2));
            }
            else if(index <= argumentsCount)
            {
                flagArgument = EFRetainTry(EFArrayGetValueAtIndex(driver->arguments, ++index));
            }
            else
            {
                ETAssemblerDiagnosticConsumerReport(driver->diagnosticConsumer, kDiagnosticSeverityError, NULL, EFSTR("missing argument to '-I'"));
                return false;
            }

            if(!EFArrayAppendValue(driver->includeSearchPaths, flagArgument))
            {
                ETAssemblerDiagnosticConsumerReport(driver->diagnosticConsumer, kDiagnosticSeverityFatal, NULL, EFSTR("out of memory, can't extract arguments from '-I'"));
                return false;
            }
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
        else if(argument != NULL && !EFStringEqualRange(argument, EFSTR("-"), EFRangeMake(0, 1)))
        {
            EFAUTOREL EFFileRef file = EFFileCreateWithPath(EFGetAllocator(driver), EFFilePolicyInData, argument);
            EFFileType fileType = EFFileGetType(file);
            if(file == NULL || !(fileType == kEFFileTypeAssembly || fileType == kEFFileTypeAssemblyIncludations || fileType == kEFFileTypeObject))
            {
                ETAssemblerDiagnosticConsumerReport(driver->diagnosticConsumer, kDiagnosticSeverityError, NULL, EFSTR("unknown or non existing input file '%@'"), argument);
                return false;
            }

            if(!EFArrayAppendValue(driver->inputFiles, file))
            {
                ETAssemblerDiagnosticConsumerReport(driver->diagnosticConsumer, kDiagnosticSeverityFatal, NULL, EFSTR("out of memory, couldn't append file to input files"));
                return false;
            }
        }
        else if(EFEqual(argument, EFSTR("--target")))
        {
            if(index >= (argumentsCount - 1))
            {
                ETAssemblerDiagnosticConsumerReport(driver->diagnosticConsumer, kDiagnosticSeverityError, NULL, EFSTR("missing argument to '--target'"));
                return false;
            }

            EFStringRef targetStr = EFRetainTry(EFArrayGetValueAtIndex(driver->arguments, ++index));

            if(EFEqual(targetStr, EFSTR("la64-generic")))
            {
                driver->driverOptions.isa = 15;
                goto valid_target;
            }

            if(EFStringHasPrefix(targetStr, EFSTR("la64-generic-v")))
            {
                EFIndex suffixLength = EFStringGetLength(targetStr) - 14;
                EFRange suffixRange = EFRangeMake(14, suffixLength);
                EFAUTOREL EFStringRef suffix = EFStringCreateCopyWithRange(EFGetAllocator(driver), targetStr, suffixRange);
                EFAUTOREL EFNumberRef versionNumber = EFStringCopyNumber(EFGetAllocator(driver), suffix);

                UInt16 isa;
                if(!EFNumberGetValue(versionNumber, kEFNumberTypeUInt16, &isa))
                {
                    ETAssemblerDiagnosticConsumerReport(driver->diagnosticConsumer, kDiagnosticSeverityError, NULL, EFSTR("target '%@' is not supported by this version of EmexToolchain"), targetStr);
                    return false;
                }

                switch(isa)
                {
                    case 0:
                    case 1:
                    case 2:
                    case 3:
                    case 4:
                    case 5:
                    case 6:
                    case 7:
                    case 8:
                    case 9:
                    case 10:
                    case 11:
                    case 12:
                    case 13:
                    case 14:
                    case 15:
                        driver->driverOptions.isa = 15;
                        goto valid_target;
                    default:
                        break;
                }
            }

            ETAssemblerDiagnosticConsumerReport(driver->diagnosticConsumer, kDiagnosticSeverityError, NULL, EFSTR("target '%@' is not supported by this version of EmexToolchain"), targetStr);
            return false;

        valid_target:
            continue;
        }
        else
        {
            ETAssemblerDiagnosticConsumerReport(driver->diagnosticConsumer, kDiagnosticSeverityError, NULL, EFSTR("unknown option '%@'"), argument);
            return false;
        }
    }

    ETAssemblerDiagnosticConsumerSetDiagnosticOptions(driver->diagnosticConsumer, driver->diagnosticOptions);

    if(EFArrayGetCount(driver->inputFiles) <= 0)
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

static EFStringRef __ETAssemblerDriverTemporaryObjectPathForInputPath(__ETAssemblerDriver driver,
                                                                      const char *input_path)
{
    const char *base = strrchr(input_path, '/');
    base = base ? base + 1 : input_path;
    const char *dot = strrchr(base, '.');
    EFSize stem_len = dot ? (EFSize)(dot - base) : strlen(base);

    const char *tmpdir = getenv("TMPDIR");
    if(tmpdir == NULL || tmpdir[0] == '\0')
    {
        tmpdir = "/tmp";
    }

    EFSize len = strlen(tmpdir) + 1 + 7 + stem_len + 1 + 6 + 2 + 1;
    char *path = malloc(len);
    if(path == NULL)
    {
        return NULL;
    }

    snprintf(path, len, "%s/emex64-%.*s-XXXXXX.o", tmpdir, (SInt32)stem_len, base);

    SInt32 fd = mkstemps(path, 2);
    if(fd < 0)
    {
        free(path);
        return NULL;
    }
    close(fd);

    EFAUTOREL EFStringRef temporaryOutputPath = EFStringCreateWithCString(EFGetAllocator(driver), path, kEFStringEncodingUTF8);
    free(path);
    if(!EFArrayAppendValue(driver->temporaryOutputPaths, temporaryOutputPath))
    {
        return NULL;
    }
    return EFAUTOTRANSFER(temporaryOutputPath);
}

Boolean __ETAssemblerDriverJobgen(__ETAssemblerDriver driver)
{
    /* -c is only meant to assemble one assembly file to a object file */
    EFIndex inputFileCount = EFArrayGetCount(driver->inputFiles);
    if(driver->driverOptions.assembleOnly && inputFileCount > 1)
    {
        ETAssemblerDiagnosticConsumerReport(driver->diagnosticConsumer, kDiagnosticSeverityFatal, NULL, EFSTR("multiple input files were passed in object emit mode"));
        return false;
    }

    /* creating assembler jobs */
    for(EFIndex index = 0; index < inputFileCount; index++)
    {
        EFFileRef inputFile = EFArrayGetValueAtIndex(driver->inputFiles, index);
        EFURLRef url = EFFileGetURL(inputFile);
        const char *input_path = EFStringGetCStringPtr(EFURLGetPath(url), kEFStringEncodingUTF8);
        EFFileType input_type = EFFileGetType(inputFile);

        switch(input_type)
        {
            case kEFFileTypeAssembly:
            case kEFFileTypeAssemblyIncludations:
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
                ratchet_args_efappend(&ra, __ETAssemblerDriverTemporaryObjectPathForInputPath(driver, input_path));
                ratchet_args_append(&ra, input_path);

                /* feature flags */
                ratchet_args_append(&ra, driver->diagnosticOptions.caret_diagnostics ? "-fcaret-diagnostics" : "-fno-caret-diagnostics");
                ratchet_args_append(&ra, driver->diagnosticOptions.color_diagnostics ? "-fcolor-diagnostics" : "-fno-color-diagnostics");

                /* warning flags */
                ratchet_args_append(&ra, driver->diagnosticOptions.warning_error ? "-Werror" : "-Wno-error");
                ratchet_args_append(&ra, driver->diagnosticOptions.warning_deprecated ? "-Wdeprecated" : "-Wno-deprecated");

                EFIndex includeSearchCount = EFArrayGetCount(driver->includeSearchPaths);
                for(EFIndex j = 0; j < includeSearchCount; j++)
                {
                    EFAUTOREL EFStringRef includeSearchArgument = EFStringCreateWithFormat(kEFAllocatorDefault, EFSTR("-I%@"), EFArrayGetValueAtIndex(driver->includeSearchPaths, j));
                    ratchet_args_efappend(&ra, includeSearchArgument);
                }
                for(EFIndex j = 0; j < driver->macroCount; j++)
                {
                    const char *m = driver->macros[j].match;
                    const char *v = driver->macros[j].value;

                    EFSize blen = 2 + strlen(m) + 1 + strlen(v) + 1;
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
            case kEFFileTypeObject:
                if(!EFArrayAppendValue(driver->linkerFlags, EFStringCreateWithCString(EFGetAllocator(driver), input_path, kEFStringEncodingUTF8)))
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
    EFIndex temporaryOutputPathCount = EFArrayGetCount(driver->temporaryOutputPaths);
    if(!driver->driverOptions.assembleOnly && temporaryOutputPathCount > 0)
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
        for(EFIndex index = 0; index < temporaryOutputPathCount; index++)
        {
            ratchet_args_efappend(&ra, EFArrayGetValueAtIndex(driver->temporaryOutputPaths, index));
        }
        EFIndex linkerFlagCount = EFArrayGetCount(driver->linkerFlags);
        for(EFIndex index = 0; index < linkerFlagCount; index++)
        {
            ratchet_args_efappend(&ra, EFArrayGetValueAtIndex(driver->linkerFlags, index));
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
    EFAUTOREL __ETAssemblerDriver driver = (__ETAssemblerDriver)EFObjectCreate(allocatorRef, ETAssemblerDriverGetTypeID(), (EFIndex)sizeof(struct __ETAssemblerDriver));
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

    EFStringRef command = EFProcessGetCommand(EFProcessGetCurrentProcess());
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

        EFIndex inputFileCount = EFArrayGetCount(driver->inputFiles);
        fprintf(stderr, "inputFiles[%ld]: { ", inputFileCount);
        for(EFIndex index = 0; index < inputFileCount; index++)
        {
            if(index != 0)
            {
                fprintf(stderr, ", ");
            }
            EFFileRef file = EFArrayGetValueAtIndex(driver->inputFiles, index);
            EFURLRef fileURL = EFFileGetURL(file);
            fprintf(stderr, "%s", EFStringGetCStringPtr(EFURLGetPath(fileURL), kEFStringEncodingUTF8));
        }
        fprintf(stderr, " }\n");

        EFIndex includeSearchPathCount = EFArrayGetCount(driver->includeSearchPaths);
        fprintf(stderr, "includeSearchPaths[%ld]: { ", includeSearchPathCount);
        for(EFIndex index = 0; index < includeSearchPathCount; index++)
        {
            if(index != 0)
            {
                fprintf(stderr, ", ");
            }
            const char *includeSearchPathC = EFStringGetCStringPtr(EFArrayGetValueAtIndex(driver->includeSearchPaths, index), kEFStringEncodingUTF8);
            fprintf(stderr, "%s", includeSearchPathC ? includeSearchPathC : "<nil>");
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
            EFIndex linkerFlagCount = EFArrayGetCount(driver->linkerFlags);
            fprintf(stderr, "linkerFlags[%ld]: { ", linkerFlagCount);
            for(EFIndex index = 0; index < linkerFlagCount; index++)
            {
                if(index != 0)
                {
                    fprintf(stderr, ", ");
                }
                const char *linkerFlagC = EFStringGetCStringPtr(EFArrayGetValueAtIndex(driver->linkerFlags, index), kEFStringEncodingUTF8);
                fprintf(stderr, "%s", linkerFlagC ? linkerFlagC : "<nil>");
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
        EFAUTOREL ETAssemblerInvocationRef invocation = ETAssemblerInvocationCreate(kEFAllocatorDefault, driver->diagnosticConsumer);
        if(invocation == NULL)
        {
            return false;
        }

        for(EFIndex index = 0; index < driver->macroCount; index++)
        {
            if(!ETAssemblerInvocationAddMacroDefinition(invocation, &driver->macros[index]))
            {
                return false;
            }
        }

        EFIndex includeSearchPathCount = EFArrayGetCount(driver->includeSearchPaths);
        for(EFIndex index = 0; index < includeSearchPathCount; index++)
        {
            if(!ETAssemblerInvocationAddIncludeSearchPath(invocation, EFArrayGetValueAtIndex(driver->includeSearchPaths, index)))
            {
                return false;
            }
        }

        {
            EFAUTOREL EFFileRef outputFile = EFFileCreateWithPath(EFGetAllocator(driver), EFFilePolicyOutData, driver->outputPath);
            if(!ETAssemblerInvocationSetInputFile(invocation, EFArrayGetValueAtIndex(driver->inputFiles, 0)) ||
               !ETAssemblerInvocationSetOutputFile(invocation, outputFile))
            {
                return false;
            }
        }

        return ETAssemblerInvocationEmit(invocation);
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
                EFAUTOREL EFProcessRef process = EFProcessCreateWithCommand(EFGetAllocator(driver), jobCommand, jobArguments);
                if(process == NULL)
                {
                    ETAssemblerDiagnosticConsumerReport(driver->diagnosticConsumer, kDiagnosticSeverityFatal, NULL, EFSTR("couldn't spawn job: %s"), strerror(errno));
                    return false;
                }

                SInt32 processIdentifier = EFProcessGetProcessIdentifier(process);
                SInt32 rstatus = 0;
                if(EFProcessWaitPID(process, &rstatus, 0) != processIdentifier)
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
                    ETAssemblerDiagnosticConsumerReport(driver->diagnosticConsumer, kDiagnosticSeverityFatal, NULL, EFSTR("job (command='%@' | pid=%d) terminated by signal %d"), jobCommand, processIdentifier, WTERMSIG(rstatus));
                    return false;
                }
            }
        }

        return true;
    }

    return false;
}

EFArrayRef ETAssemblerDriverGetJobs(ETAssemblerDriverRef driverRef)
{
    __ETAssemblerDriver driver = (__ETAssemblerDriver)driverRef;
    return driver != NULL ? driver->jobs : NULL;
}

EFStringRef ETAssemblerDriverGetOutputPath(ETAssemblerDriverRef driverRef)
{
    __ETAssemblerDriver driver = (__ETAssemblerDriver)driverRef;
    return driver != NULL ? driver->outputPath : NULL;
}

ETAssemblerDiagnosticConsumerRef ETAssemblerDriverGetDiagnosticConsumer(ETAssemblerDriverRef driverRef)
{
    __ETAssemblerDriver driver = (__ETAssemblerDriver)driverRef;
    return driver != NULL ? driver->diagnosticConsumer : NULL;
}

ETAssemblerDriverOptions ETAssemblerDriverGetDriverOptions(ETAssemblerDriverRef driverRef)
{
    __ETAssemblerDriver driver = (__ETAssemblerDriver)driverRef;
    return driver != NULL ? driver->driverOptions : ETAssemblerDriverOptionsDefault;
}

ETAssemblerDiagnosticOptions ETAssemblerDriverGetDiagnosticOptions(ETAssemblerDriverRef driverRef)
{
    __ETAssemblerDriver driver = (__ETAssemblerDriver)driverRef;
    return driver != NULL ? driver->diagnosticOptions : ETAssemblerDiagnosticOptionsDefault;
}

void ETAssemblerDriverSetDriverOptions(ETAssemblerDriverRef driverRef,
                                       ETAssemblerDriverOptions driverOptions)
{
    __ETAssemblerDriver driver = (__ETAssemblerDriver)driverRef;
    if(driver == NULL)
    {
        return;
    }

    driver->driverOptions = driverOptions;
}

void ETAssemblerDriverSetDiagnosticOptions(ETAssemblerDriverRef driverRef,
                                           ETAssemblerDiagnosticOptions diagnosticOptions)
{
    __ETAssemblerDriver driver = (__ETAssemblerDriver)driverRef;
    if(driver == NULL)
    {
        return;
    }

    driver->diagnosticOptions = diagnosticOptions;
    ETAssemblerDiagnosticConsumerSetDiagnosticOptions(driver->diagnosticConsumer, diagnosticOptions);
}
