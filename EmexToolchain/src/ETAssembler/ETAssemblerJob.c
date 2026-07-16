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

#include <EmexToolchain/ETAssembler/ETAssemblerJob.h>

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
    job->type = type;

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
