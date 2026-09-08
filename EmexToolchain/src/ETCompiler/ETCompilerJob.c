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
#include <EmexFoundation/EmexFoundation.h>
#include <EmexToolchain/ETCompiler/ETCompilerJob.h>

typedef struct __ETCompilerJob {
    EFObject header;
    ETCompilerJobType type;
    EFStringRef command;
    EFArrayRef arguments;
} *__ETCompilerJob;

static void __ETCompilerJobDeinit(EFObjectRef jobRef)
{
    __ETCompilerJob job = (__ETCompilerJob)jobRef;
    EFRelease(job->command);
    EFRelease(job->arguments);
}

static EFStringRef __ETCompilerJobCopyDescription(EFObjectRef jobRef)
{
    __ETCompilerJob job = (__ETCompilerJob)jobRef;
    return EFStringCreateWithFormat(EFGetAllocator(jobRef), EFSTR("<ETCompilerJob %p>{command = %@, arguments = %@}"), jobRef, job->command, job->arguments);
}

static EFClassDefinitionV2 ETCompilerJobClass = {
    .header = {
        .version = 2,
        .typeID = kEFTypeIDNone,
        .name = NULL,
    },
    .name = "ETCompilerJob",
    .init = NULL,
    .deinit = __ETCompilerJobDeinit,
    .equal = NULL,
    .copyDescription = __ETCompilerJobCopyDescription,
};

static void ETCompilerJobRegisterClass(void)
{
    EFClassRegister(&ETCompilerJobClass);
}

EFTypeID ETCompilerJobGetTypeID(void)
{
    static pthread_once_t once = PTHREAD_ONCE_INIT;
    pthread_once(&once, ETCompilerJobRegisterClass);
    return ETCompilerJobClass.header.typeID;
}

ETCompilerJobRef ETCompilerJobCreate(EFAllocatorRef allocatorRef,
                                     ETCompilerJobType type,
                                     EFStringRef command,
                                     EFArrayRef arguments)
{
    EFAUTOREL EFStringRef ownedCommand = EFRetainTry(command);
    if(ownedCommand == NULL)
    {
        return NULL;
    }

    EFAUTOREL EFArrayRef ownedArguments = EFArrayCreateCopy(allocatorRef, arguments);
    if(ownedArguments == NULL)
    {
        return NULL;
    }

    __ETCompilerJob job = (__ETCompilerJob)EFObjectCreate(allocatorRef, ETCompilerJobGetTypeID(), (EFIndex)sizeof(struct __ETCompilerJob));
    if(job == NULL)
    {
        return NULL;
    }

    job->command = EFAUTOTRANSFER(ownedCommand);
    job->arguments = EFAUTOTRANSFER(ownedArguments);
    job->type = type;

    return (ETCompilerJobRef)job;
}

ETCompilerJobType ETCompilerJobGetType(ETCompilerJobRef jobRef)
{
    __ETCompilerJob job = (__ETCompilerJob)jobRef;
    return job != NULL ? job->type : kETCompilerJobTypeUnknown;
}

EFStringRef ETCompilerJobGetCommand(ETCompilerJobRef jobRef)
{
    __ETCompilerJob job = (__ETCompilerJob)jobRef;
    return job != NULL ? job->command : NULL;
}

EFArrayRef ETCompilerJobGetArguments(ETCompilerJobRef jobRef)
{
    __ETCompilerJob job = (__ETCompilerJob)jobRef;
    return job != NULL ? job->arguments : NULL;
}
