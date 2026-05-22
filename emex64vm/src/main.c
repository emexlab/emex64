/*
 * MIT License
 *
 * Copyright (c) 2024 cr4zyengineer
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
#include <unistd.h>
#include <string.h>
#include <pthread.h>

#include <emex64vm/machine.h>
#include <emex64vm/device/display.h>

#include <emexutils/bitwalker.h>

int main(int argc, char *argv[])
{
    if(argc < 2 || argv == NULL || argv[1] == NULL)
    {
        goto usage;
    }

    /* creating new la16 virtual machine */
    la64_machine_t *machine = la64_machine_alloc(0x20000000);

    if(machine == NULL)
    {
        fprintf(stderr, "[!] failed to allocated machine\n");
        return 1;
    }

    /* load boot image */
    if(!la64_memory_load_image(machine->memory, argv[1]))
    {
        goto usage;
    }

    /*
     * getting entry point of boot image of virtual machine
     * and setting program pointer of first core to it
     */
    bitwalker_t bw;
    bitwalker_init_read(&bw, machine->memory->memory, 8, BW_LITTLE_ENDIAN);
    machine->core->rl[LA64_REGISTER_PC] = bitwalker_read(&bw, 64);

    /* setting stack pointer of  */
    machine->core->rl[LA64_REGISTER_SP] = machine->memory->memory_size - 8;

    /* setting elevation to system monitor */
    machine->core->rl[LA64_REGISTER_CR0] = LA64_ELEVATION_SECURE_MONITOR;

    /* executing virtual machines 1st core TODO: Implement threading */
    la64_core_execute(machine->core);

    /* deallocating machine */
    la64_machine_dealloc(machine);

    return 0;

usage:
    printf("%s <boot image>\n", (argv == NULL || argv[0] == NULL) ? "(nil)" : argv[0]);
    return 1;
}
