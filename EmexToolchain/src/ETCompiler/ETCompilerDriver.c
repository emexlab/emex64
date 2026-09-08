
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

#include <pthread.h>
#include <EmexToolchain/Support/version.h>
#include <EmexToolchain/ETCompiler/ETCompilerDriver.h>

typedef struct __ETCompilerDriver {
    EFObject header;

    EFArrayRef arguments;

    EFMutableArrayRef inputFiles;
    EFStringRef outputPath;

    EFMutableArrayRef includeSearchPaths;
    EFMutableArrayRef linkerFlags;
} *__ETCompilerDriver;

static void __ETCompilerDriverDeinit(EFObjectRef driverRef)
{
    __ETCompilerDriver driver = (__ETCompilerDriver)driverRef;
    EFReleaseTry(driver->arguments);
}

static EFClassDefinitionV2 ETCompilerDriverClass = {
    .header = {
        .version = 2,
        .typeID = kEFTypeIDNone,
        .name = NULL,
    },
    .name = "ETCompilerDriver",
    .init = NULL,
    .deinit = __ETCompilerDriverDeinit,
    .equal = NULL,
    .copyDescription = NULL,
    .hash = NULL,
};

static Boolean __ETCompilerDriverPredrive(__ETCompilerDriver driver)
{
    /* better starting with the default assembler options ^^ */
    driver->outputPath = NULL;

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
                "  --target               Sets the target the assembly files will be assembled for (i.e la64-generic or la64-generic-v15).\n"
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
        else if(EFStringHasPrefix(argument, EFSTR("-f")))
        {
            EFIndex length = EFStringGetLength(argument);
            if(length <= 2)
            {
                //ETAssemblerDiagnosticConsumerReport(driver->diagnosticConsumer, kDiagnosticSeverityError, NULL, EFSTR("missing argument to '-f'"));
                return false;
            }
            EFRange flagArgumentRange = EFRangeMake(2, length - 2);

            if(EFStringEqualRange(argument, EFSTR("page-align"), flagArgumentRange) || EFStringEqualRange(argument, EFSTR("no-page-align"), flagArgumentRange))
            {
                EFAUTOREL EFStringRef flagArgument = EFStringCreateCopyWithRange(kEFAllocatorDefault, argument, flagArgumentRange);
                //ETAssemblerDiagnosticConsumerReport(driver->diagnosticConsumer, kDiagnosticSeverityWarning, NULL, EFSTR("feature flag '%@' is deprecated, please you equivalents if available"), flagArgument);
            }
            else if(EFStringEqualRange(argument, EFSTR("caret-diagnostics"), flagArgumentRange))
            {
                //driver->diagnosticOptions.caret_diagnostics = true;
            }
            else if(EFStringEqualRange(argument, EFSTR("no-caret-diagnostics"), flagArgumentRange))
            {
                //driver->diagnosticOptions.caret_diagnostics = false;
            }
            else if(EFStringEqualRange(argument, EFSTR("color-diagnostics"), flagArgumentRange))
            {
                //driver->diagnosticOptions.color_diagnostics = true;
            }
            else if(EFStringEqualRange(argument, EFSTR("no-color-diagnostics"), flagArgumentRange))
            {
                //driver->diagnosticOptions.color_diagnostics = false;
            }
            else
            {
                EFAUTOREL EFStringRef flagArgument = EFStringCreateCopyWithRange(kEFAllocatorDefault, argument, flagArgumentRange);
                //ETAssemblerDiagnosticConsumerReport(driver->diagnosticConsumer, kDiagnosticSeverityError, NULL, EFSTR("unknown feature flag '%@'"), flagArgument);
                return false;
            }
        }
        else if(EFStringHasPrefix(argument, EFSTR("-Wl,")))
        {
            EFIndex length = EFStringGetLength(argument);
            if(length <= 4)
            {
                //ETAssemblerDiagnosticConsumerReport(driver->diagnosticConsumer, kDiagnosticSeverityError, NULL, EFSTR("missing argument to '-Wl,'"));
                return false;
            }
            EFRange flagArgumentRange = EFRangeMake(4, length - 4);

            EFAUTOREL EFStringRef flagArgument = EFStringCreateCopyWithRange(kEFAllocatorDefault, argument, flagArgumentRange);
            EFAUTOREL EFArrayRef components = EFStringComponentsSplitBySeparator(flagArgument, EFSTR(","));
            if(components == NULL)
            {
                //ETAssemblerDiagnosticConsumerReport(driver->diagnosticConsumer, kDiagnosticSeverityFatal, NULL, EFSTR("out of memory, can't extract arguments from '-Wl,' argument"));
                return false;
            }

            EFIndex flagCount = EFArrayGetCount(components);
            for(EFIndex index = 0; index < flagCount; index++)
            {
                if(!EFArrayAppendValue(driver->linkerFlags, EFArrayGetValueAtIndex(components, index)))
                {
                    //ETAssemblerDiagnosticConsumerReport(driver->diagnosticConsumer, kDiagnosticSeverityFatal, NULL, EFSTR("out of memory, can't extract arguments from '-Wl,' argument"));
                    return false;
                }
            }
        }
        else if(EFStringHasPrefix(argument, EFSTR("-W")))
        {
            EFIndex length = EFStringGetLength(argument);
            if(length <= 2)
            {
                //ETAssemblerDiagnosticConsumerReport(driver->diagnosticConsumer, kDiagnosticSeverityError, NULL, EFSTR("missing argument to '-W'"));
                return false;
            }
            EFRange flagArgumentRange = EFRangeMake(2, length - 2);

            if(EFStringEqualRange(argument, EFSTR("error"), flagArgumentRange))
            {
                //driver->diagnosticOptions.warning_error = true;
            }
            else if(EFStringEqualRange(argument, EFSTR("no-error"), flagArgumentRange))
            {
                //driver->diagnosticOptions.warning_error = false;
            }
            else if(EFStringEqualRange(argument, EFSTR("deprecated"), flagArgumentRange))
            {
                //driver->diagnosticOptions.warning_deprecated = true;
            }
            else if(EFStringEqualRange(argument, EFSTR("no-deprecated"), flagArgumentRange))
            {
                //driver->diagnosticOptions.warning_deprecated = false;
            }
            else
            {
                EFAUTOREL EFStringRef flagArgument = EFStringCreateCopyWithRange(kEFAllocatorDefault, argument, flagArgumentRange);
                //ETAssemblerDiagnosticConsumerReport(driver->diagnosticConsumer, kDiagnosticSeverityError, NULL, EFSTR("unknown warning flag '%@'"), flagArgument);
                return false;
            }
        }
        else if(EFStringHasPrefix(argument, EFSTR("-D")))
        {
            EFIndex length = EFStringGetLength(argument);
            if(length <= 2)
            {
                //ETAssemblerDiagnosticConsumerReport(driver->diagnosticConsumer, kDiagnosticSeverityError, NULL, EFSTR("missing argument to '-D'"));
                return false;
            }
            EFRange flagArgumentRange = EFRangeMake(2, length - 2);

            EFAUTOREL EFStringRef flagArgument = EFStringCreateCopyWithRange(kEFAllocatorDefault, argument, flagArgumentRange);
            EFAUTOREL EFArrayRef components = EFStringComponentsSplitBySeparator(flagArgument, EFSTR("="));
            if(components == NULL || EFArrayGetCount(components) < 1)
            {
                //ETAssemblerDiagnosticConsumerReport(driver->diagnosticConsumer, kDiagnosticSeverityFatal, NULL, EFSTR("out of memory, can't extract arguments from '-D'"));
                return false;
            }

            EFStringRef macro = EFArrayGetValueAtIndex(components, 0);
            EFRange macroRange = EFRangeMake(0, EFStringGetLength(macro));
            EFRange valueRange = (EFArrayGetCount(components) > 1) ? EFRangeMake(macroRange.length + 1, EFStringGetLength(flagArgument) - (macroRange.length + 1)) : EFRangeZero;
            EFAUTOREL EFStringRef value = EFRangeIsEqual(valueRange, EFRangeZero) ? EFSTR("1") : EFStringCreateCopyWithRange(kEFAllocatorDefault, flagArgument, valueRange);

            /*EFIndex macroSlot = driver->macroCount++;
            if(driver->macros == NULL)
            {
                driver->macros = calloc(driver->macroCount, sizeof(assembler_macro_definition_t));
            }
            else
            {
                driver->macros = realloc(driver->macros, driver->macroCount * sizeof(assembler_macro_definition_t));
            }

            driver->macros[macroSlot].match = strdup(EFStringGetCStringPtr(macro, kEFStringEncodingUTF8));
            driver->macros[macroSlot].value = strdup(EFStringGetCStringPtr(value, kEFStringEncodingUTF8));*/
        }
        else if(EFStringHasPrefix(argument, EFSTR("-I")))
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
                //ETAssemblerDiagnosticConsumerReport(driver->diagnosticConsumer, kDiagnosticSeverityError, NULL, EFSTR("missing argument to '-I'"));
                return false;
            }

            if(!EFArrayAppendValue(driver->includeSearchPaths, flagArgument))
            {
                //ETAssemblerDiagnosticConsumerReport(driver->diagnosticConsumer, kDiagnosticSeverityFatal, NULL, EFSTR("out of memory, can't extract arguments from '-I'"));
                return false;
            }
        }
        else if(EFEqual(argument, EFSTR("-c")))
        {
            //driver->driverOptions.assembleOnly = true;
        }
        else if(EFEqual(argument, EFSTR("-v")))
        {
            //driver->driverOptions.verbose = true;
        }
        else if(EFEqual(argument, EFSTR("--in-process")))
        {
            //driver->driverOptions.inProcess = true;
        }
        else if(EFEqual(argument, EFSTR("-r")))
        {
            //driver->driverOptions.emitMode = kEmitModeRelocatableObject;
        }
        else if(argument != NULL && !EFStringEqualRange(argument, EFSTR("-"), EFRangeMake(0, 1)))
        {
            EFAUTOREL EFFileRef file = EFFileCreateWithPath(EFGetAllocator(driver), EFFilePolicyInData, argument);
            EFFileType fileType = EFFileGetType(file);
            if(file == NULL || !(fileType == kEFFileTypeAssembly || fileType == kEFFileTypeAssemblyIncludations || fileType == kEFFileTypeObject))
            {
                //ETAssemblerDiagnosticConsumerReport(driver->diagnosticConsumer, kDiagnosticSeverityError, NULL, EFSTR("unknown or non existing input file '%@'"), argument);
                return false;
            }

            if(!EFArrayAppendValue(driver->inputFiles, file))
            {
                //ETAssemblerDiagnosticConsumerReport(driver->diagnosticConsumer, kDiagnosticSeverityFatal, NULL, EFSTR("out of memory, couldn't append file to input files"));
                return false;
            }
        }
        else if(EFEqual(argument, EFSTR("--target")))
        {
            if(index >= (argumentsCount - 1))
            {
                //ETAssemblerDiagnosticConsumerReport(driver->diagnosticConsumer, kDiagnosticSeverityError, NULL, EFSTR("missing argument to '--target'"));
                return false;
            }

            EFStringRef targetStr = EFRetainTry(EFArrayGetValueAtIndex(driver->arguments, ++index));

            if(EFEqual(targetStr, EFSTR("la64-generic")))
            {
                //driver->driverOptions.isa = 15;
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
                    goto invalid_target;
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
                        //driver->driverOptions.isa = 15;
                        goto valid_target;
                    default:
                        goto invalid_target;
                }
            }

        invalid_target:
            //ETAssemblerDiagnosticConsumerReport(driver->diagnosticConsumer, kDiagnosticSeverityError, NULL, EFSTR("target '%@' is not supported by this version of EmexToolchain"), targetStr);
            return false;

        valid_target:
            continue;
        }
        else
        {
            //ETAssemblerDiagnosticConsumerReport(driver->diagnosticConsumer, kDiagnosticSeverityError, NULL, EFSTR("unknown option '%@'"), argument);
            return false;
        }
    }

    //ETAssemblerDiagnosticConsumerSetDiagnosticOptions(driver->diagnosticConsumer, driver->diagnosticOptions);

    if(EFArrayGetCount(driver->inputFiles) <= 0)
    {
        //ETAssemblerDiagnosticConsumerReport(driver->diagnosticConsumer, kDiagnosticSeverityError, NULL, EFSTR("no input files"));
        return false;
    }

    if(driver->outputPath == NULL)
    {
        //ETAssemblerDiagnosticConsumerReport(driver->diagnosticConsumer, kDiagnosticSeverityWarning, NULL, EFSTR("no output path provided, falling back to 'a.out'"));
        driver->outputPath = EFSTR("a.out");
    }

    return true;
}

static void ETCompilerDriverRefisterClass(void)
{
    EFClassRegister(&ETCompilerDriverClass);
}

EFTypeID ETCompilerDriverGetTypeID(void)
{
    pthread_once_t once = PTHREAD_ONCE_INIT;
    pthread_once(&once, ETCompilerDriverRefisterClass);
    return ETCompilerDriverClass.header.typeID;
}

ETCompilerDriverRef ETCompilerDriverCreate(EFAllocatorRef allocatorRef,
                                           EFArrayRef arguments)
{
    EFAUTOREL __ETCompilerDriver driver = (__ETCompilerDriver)EFObjectCreate(allocatorRef, ETCompilerDriverGetTypeID(), (EFIndex)sizeof(struct __ETCompilerDriver));
    if(driver == NULL)
    {
        return NULL;
    }

    /*driver->jobs = EFArrayCreateMutable(allocatorRef, kEFArrayCallbacksObjectCallbacks, 0);
    if(driver->jobs == NULL)
    {
        return NULL;
    }*/

    driver->arguments = EFArrayCreateCopy(allocatorRef, arguments);
    if(driver->arguments == NULL)
    {
        return NULL;
    }

    /*driver->driverOptions = driverOptions;
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
    }*/

    if(!__ETCompilerDriverPredrive(driver))
    {
        return NULL;
    }

    EFStringRef command = EFProcessGetCommand(EFProcessGetCurrentProcess());
    if(command == NULL)
    {
        command = EFSTR("emex64asm");
    }

    /*if(driver->driverOptions.verbose)
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
    }*/

    return (ETCompilerDriverRef)EFAUTOTRANSFER(driver);
}

Boolean ETCompilerDriverRun(ETCompilerDriverRef driverRef)
{
    /* not even the driver is finished yet */
    return false;
}
