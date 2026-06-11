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
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>

#include <emex64lib/support/bitwalker.h>
#include <emex64lib/support/diag.h>
#include <emex64lib/support/parser.h>

#include <emex64lib/vm/machine.h>
#include <emex64lib/vm/device/display.h>

int main(int argc, char *argv[])
{
    int opt;
    const char *firmware_image_path = NULL;

    emex64_machine_options_t machine_options = emex64_machine_options_default();
    emex64_machine_support_t machine_support = emex64_machine_support_get();

    /* parse arguments */
    for(int i = 1; i < argc; i++)
    {
        if(strcmp(argv[i], "--help") == 0)
        {
        usage:
            fprintf(stderr, "%s [options]\n", argv[0]);
            fprintf(stderr, "\t--help                                       : showing help menu\n");
            fprintf(stderr, "\t--firmware <image path>                      : providing firmware image\n");
            fprintf(stderr, "\t--memory <memory size>                       : providing memory size in megabyte\n");
            fprintf(stderr, "\t--audio [on|off|required]                    : enables or disables AC97 audio\n");
            if(machine_support.display)
            {
                fprintf(stderr, "\t--display [on|off|required]                  : enables or disables display\n");
                fprintf(stderr, "\t--display:resolution [on|off|required]       : enables or disables display\n");
            }
            return 1;
        }
        else if(strcmp(argv[i], "--firmware") == 0 && i + 1 < argc)
        {
            firmware_image_path = argv[++i];
        }
        else if(strcmp(argv[i], "--memory") == 0 && i + 1 < argc)
        {
            parser_return_t pr = parse_value_from_string(argv[++i]);
            if(pr.type == emexParserValueTypeNumber)
            {
                machine_options.memory_size = pr.value * 1024 * 1024;
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
                if(!machine_support.display)
                {
                    diag_warn(NULL, "display support is not available in this distribution of the emex64 toolchain\n");
                }
                machine_options.display.enabled = true;
            }
            else if(strcmp(argv[i + 1], "off") == 0)
            {
                machine_options.display.enabled = false;
            }
            else if(strcmp(argv[i + 1], "required") == 0)
            {
                if(!machine_support.display)
                {
                    diag_error(NULL, "display support is not available in this distribution of the emex64 toolchain\n");
                    return 1;
                }
                machine_options.display.enabled = true;
            }
            else
            {
                diag_error(NULL, "unknown argument supplied to '--display': '%s'\n", argv[i + 1]);
                return 1;
            }
            i++;
        }
        else if(strcmp(argv[i], "--audio") == 0 && i + 1 < argc)
        {
            if(strcmp(argv[i + 1], "on") == 0)
            {
                machine_options.audio = true;
            }
            else if(strcmp(argv[i + 1], "off") == 0)
            {
                machine_options.audio = false;
            }
            else if(strcmp(argv[i + 1], "required") == 0)
            {
                if(!machine_support.audio)
                {
                    diag_error(NULL, "audio support is not available in this distribution of the emex64 toolchain\n");
                    return 1;
                }
                machine_options.audio = true;
            }
            else
            {
                diag_error(NULL, "unknown argument supplied to '--audio': '%s'\n", argv[i + 1]);
                return 1;
            }
            i++;
        }
        else if(strcmp(argv[i], "--display:resolution") == 0 && i + 2 < argc)
        {
            parser_return_t pr_width = parse_value_from_string(argv[++i]);
            parser_return_t pr_height = parse_value_from_string(argv[++i]);

            if(pr_width.type == emexParserValueTypeNumber && 
               pr_height.type == emexParserValueTypeNumber)
            {
                machine_options.display.width = pr_width.value;
                machine_options.display.height = pr_height.value;
            }
            else
            {
                diag_error(NULL, "illegal arguments supplied to '--display:resolution': '%s' and '%s'\n", argv[i - 1], argv[i - 2]);
                return 1;
            }
            i++;
        }
        else
        {
            diag_error(NULL, "unknown option '%s'\n", argv[i]);
            return 1;
        }
    }

    /* creating new emex64 virtual machine */
    emex64_machine_t *machine = emex64_machine_alloc(machine_options);
    if(machine == NULL)
    {
        diag_error(NULL, "failed to allocated machine\n");
        return 1;
    }

    /*
     * load boot image, maybe use a dirty private
     * mapping and open the image it self as memory,
     * just size it.
     */
    if(firmware_image_path != NULL && !emex64_memory_load_image(machine->memory, firmware_image_path))
    {
        diag_error(NULL, "failed to load firmware image\n");
        return 1;
    }
    
    /* executing virtual machines 1st core TODO: Implement threading */
    emex64_core_execute(machine->core);

    /* deallocating machine */
    emex64_machine_dealloc(machine);

    return 0;
}
