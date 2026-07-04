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
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>

#include <EmexToolchain/support/diagnostic/log.h>
#include <EmexToolchain/support/parser.h>

#include <EmexToolchain/vm/E64Machine.h>

int main(int argc, char *argv[])
{
    const char *firmware_image_path = NULL;

    E64MachineOptions machineOptions = E64MachineOptionsGetDefault();
    E64MachineSupport machineSupport = E64MachineSupportGet();

    /* TODO: we need a driver for the VM later */
    /* parse arguments */
    for(int i = 1; i < argc; i++)
    {
        if(strcmp(argv[i], "--help") == 0)
        {
            fprintf(stderr, "Usage: %s [options]\n", argv[0]);
            fprintf(stderr, "\n");
            fprintf(stderr, "Options:\n");
            fprintf(stderr, "  --help                 Shows this help menu.\n");
            fprintf(stderr, "  -f <image path>        Sets the firmware image path that will be loaded and\n");
            fprintf(stderr, "                         executed by the VM.\n");
            fprintf(stderr, "  -m:[kb|mb|gb] <size>   Providing memory size, this size will later be allocated\n");
            fprintf(stderr, "                         for the machine.\n");
            fprintf(stderr, "\n");
            if(machineSupport.display)
            {
                fprintf(stderr, "  --display [on|off|required]\n");
                fprintf(stderr, "                         Sets up the display, if it is required, but not available\n");
                fprintf(stderr, "                         on a certain distribution it will error.\n");
                fprintf(stderr, "  --display:resolution <height> <width>\n");
                fprintf(stderr, "                         Sets the display resolution.\n");
            }
            fprintf(stderr, "\n");
            fprintf(stderr, "  --keyboard [off|8042]  Sets up keyboard, when 8042 chip mode is used the display\n");
            fprintf(stderr, "                         is required.\n");
            fprintf(stderr, "  --mouse [off|8042]     Sets up mouse, when 8042 chip mode is used the display\n");
            fprintf(stderr, "                         is required.\n");
            return 1;
        }
        else if(strcmp(argv[i], "-f") == 0 && i + 1 < argc)
        {
            firmware_image_path = argv[++i];
        }
        else if(strcmp(argv[i], "-m:kb") == 0 && i + 1 < argc)
        {
            parser_return_t pr = parse_value_from_string(argv[++i]);
            if(pr.type == emexParserValueTypeNumber)
            {
                machineOptions.memoryLength = pr.value * 1024;
            }
            else
            {
                diag_error(NULL, "illegal value type used\n", argv[i]);
                return 1;
            }
        }
        else if((strcmp(argv[i], "-m:mb") == 0 || strcmp(argv[i], "--memory") == 0) && i + 1 < argc)
        {
            parser_return_t pr = parse_value_from_string(argv[++i]);
            if(pr.type == emexParserValueTypeNumber)
            {
                machineOptions.memoryLength = pr.value * 1024 * 1024;
            }
            else
            {
                diag_error(NULL, "illegal value type used\n", argv[i]);
                return 1;
            }
        }
        else if(strcmp(argv[i], "-m:gb") == 0 && i + 1 < argc)
        {
            parser_return_t pr = parse_value_from_string(argv[++i]);
            if(pr.type == emexParserValueTypeNumber)
            {
                machineOptions.memoryLength = pr.value * 1024 * 1024 * 1024;
            }
            else
            {
                diag_error(NULL, "illegal value type used\n", argv[i]);
                return 1;
            }
        }
        else if(strcmp(argv[i], "--display") == 0 && i + 1 < argc)
        {
            if(strcmp(argv[i + 1], "on") == 0)
            {
                if(!machineSupport.display)
                {
                    diag_warn(NULL, "--display flag is not supported in this distribution of the emex64 toolchain\n");
                }
                else
                {
                    machineOptions.displayOptions.enabled = true;
                }
            }
            else if(strcmp(argv[i + 1], "off") == 0)
            {
                machineOptions.displayOptions.enabled = false;
            }
            else if(strcmp(argv[i + 1], "required") == 0)
            {
                if(!machineOptions.displayOptions.enabled)
                {
                    diag_error(NULL, "-display flag is not supported in this distribution of the emex64 toolchain\n");
                    return 1;
                }
                machineOptions.displayOptions.enabled = true;
            }
            else
            {
                diag_error(NULL, "unknown argument supplied to '--display': '%s'\n", argv[i + 1]);
                return 1;
            }
            i++;
        }
        else if(strcmp(argv[i], "--keyboard") == 0 && i + 1 < argc)
        {
            if(strcmp(argv[i + 1], "off") == 0)
            {
                machineOptions.keyboardPeripheralMode = kE64PeripheralModeOff;
            }
            else if(strcmp(argv[i + 1], "8042") == 0)
            {
                machineOptions.keyboardPeripheralMode = kE64PeripheralMode8042;
            }
            else
            {
                diag_error(NULL, "unknown argument supplied to '--keyboard': '%s'\n", argv[i + 1]);
                return 1;
            }
            i++;
        }
        else if(strcmp(argv[i], "--mouse") == 0 && i + 1 < argc)
        {
            if(strcmp(argv[i + 1], "off") == 0)
            {
                machineOptions.mousePeripheralMode = kE64PeripheralModeOff;
            }
            else if(strcmp(argv[i + 1], "8042") == 0)
            {
                machineOptions.mousePeripheralMode = kE64PeripheralMode8042;
            }
            else
            {
                diag_error(NULL, "unknown argument supplied to '--mouse': '%s'\n", argv[i + 1]);
                return 1;
            }
            i++;
        }
        else if(strcmp(argv[i], "--display:resolution") == 0 && i + 2 < argc)
        {
            parser_return_t pr_width = parse_value_from_string(argv[++i]);
            parser_return_t pr_height = parse_value_from_string(argv[++i]);

            if(!machineSupport.display)
            {
                diag_warn(NULL, "--display:resolution flag is not supported in this distribution of the emex64 toolchain\n");
                continue;
            }

            if(pr_width.type == emexParserValueTypeNumber && 
               pr_height.type == emexParserValueTypeNumber)
            {
                machineOptions.displayOptions.width = pr_width.value;
                machineOptions.displayOptions.height = pr_height.value;
            }
            else
            {
                diag_error(NULL, "illegal arguments supplied to '--display:resolution': '%s' and '%s'\n", argv[i - 1], argv[i - 2]);
                return 1;
            }
        }
        else
        {
            diag_error(NULL, "unknown option '%s'\n", argv[i]);
            return 1;
        }
    }

    /* creating new emex64 virtual machine */
    E64MachineRef machine = E64MachineCreateWithOptions(kEFAllocatorDefault, machineOptions);
    if(machine == NULL)
    {
        diag_error(NULL, "failed to allocated machine\n");
        return 1;
    }

    if(firmware_image_path != NULL)
    {
        emex_file_t *file = emex_file_alloc(firmware_image_path, in_data_file_policy);
        if(file == NULL)
    fail:
        {
            diag_error(NULL, "failed to load firmware image\n");
            EFRelease(machine);
            return 1;
        }

        Boolean success = E64MemoryLoadImage(machine->memory, file);
        emex_file_dealloc(file);
        if(!success)
        {
            goto fail;
        }
    }
    
    /* executing virtual machines 1st core TODO: Implement multicore */
    emex64_core_execute(machine->core);

    /* deallocating machine */
    EFRelease(machine);

    return 0;
}
