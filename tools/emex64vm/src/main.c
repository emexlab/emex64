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

#include <emex64lib/support/diagnostic/legacy.h>
#include <emex64lib/support/parser.h>

#include <emex64lib/vm/machine.h>

int main(int argc, char *argv[])
{
    const char *firmware_image_path = NULL;

    emex64_machine_options_t machine_options = emex64_machine_options_default();
    emex64_machine_support_t machine_support = emex64_machine_support_get();

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
            if(machine_support.display)
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
                machine_options.memory_size = pr.value * 1024;
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
                machine_options.memory_size = pr.value * 1024 * 1024;
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
                machine_options.memory_size = pr.value * 1024 * 1024 * 1024;
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
                    diag_warn(NULL, "--display flag is not supported in this distribution of the emex64 toolchain\n");
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
                    diag_error(NULL, "-display flag is not supported in this distribution of the emex64 toolchain\n");
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
        else if(strcmp(argv[i], "--keyboard") == 0 && i + 1 < argc)
        {
            if(strcmp(argv[i + 1], "off") == 0)
            {
                machine_options.keyboard_mode = kKeyboardModeOff;
            }
            else if(strcmp(argv[i + 1], "8042") == 0)
            {
                machine_options.keyboard_mode = kKeyboardMode8042;
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
                machine_options.mouse_mode = kMouseModeOff;
            }
            else if(strcmp(argv[i + 1], "8042") == 0)
            {
                machine_options.mouse_mode = kMouseMode8042;
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

            if(!machine_support.display)
            {
                diag_warn(NULL, "--display:resolution flag is not supported in this distribution of the emex64 toolchain\n");
                continue;
            }

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

    if(firmware_image_path != NULL)
    {
        emex_file_t *file = emex_file_alloc(firmware_image_path, in_data_file_policy);
        if(file == NULL)
    fail:
        {
            diag_error(NULL, "failed to load firmware image\n");
            return 1;
        }

        bool success = emex64_memory_load_image(machine->memory, file);
        emex_file_dealloc(file);
        if(!success)
        {
            goto fail;
        }
    }
    
    /* executing virtual machines 1st core TODO: Implement multicore */
    emex64_core_execute(machine->core);

    /* deallocating machine */
    emex64_machine_dealloc(machine);

    return 0;
}
