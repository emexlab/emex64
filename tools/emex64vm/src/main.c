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
#include <pthread.h>

#include <EmexToolchain/Support/diagnostic/log.h>
#include <EmexToolchain/Support/parser.h>

#include <EmexToolchain/VM/E64Machine.h>

SInt32 main(SInt32 argc, char *argv[])
{
    const char *firmware_image_path = NULL;

    E64MachineOptions machineOptions = E64MachineOptionsDefault;
    E64MachineSupport machineSupport = E64MachineSupportCurrent;

    /* TODO: we need a driver for the VM later */
    /* parse arguments */
    for(SInt32 i = 1; i < argc; i++)
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
            fprintf(stderr, "  --sata <image path>    Attaches a disk image to the next free port of the SATA\n");
            fprintf(stderr, "                         host controller on PCIe (up to %d disks).\n", E64_MACHINE_MAX_SATA_DISKS);
            fprintf(stderr, "  --sata:ro <image path> Same as --sata, but the disk is read only.\n");
            fprintf(stderr, "  --usb <image path>     Plugs a USB flash drive with this disk image into the\n");
            fprintf(stderr, "                         next free port of the USB host controller on PCIe\n");
            fprintf(stderr, "                         (up to %d drives).\n", E64_MACHINE_MAX_USB_DISKS);
            fprintf(stderr, "  --usb:ro <image path>  Same as --usb, but the drive is write protected.\n");
            fprintf(stderr, "  --nvram <file path>    Keeps the NVRAM (the firmware's settings) in this file\n");
            fprintf(stderr, "                         (default: the firmware image path + .nvram).\n");
            fprintf(stderr, "  --sound [on|off]       The sound card, a Sound Blaster 16 on PCIe (default: on).\n");
            fprintf(stderr, "  --sound:wav <file>     What the sound card plays goes into this WAV file\n");
            fprintf(stderr, "                         instead of the host's sound output.\n");
            if(machineSupport.network)
            {
                fprintf(stderr, "  --net user             Plugs in the network controller: user mode NAT to the\n");
                fprintf(stderr, "                         host's network (the guest is 10.0.2.15). Without --net:fwd\n");
                fprintf(stderr, "                         the host's localhost:2222 goes to the guest's SSH (22).\n");
                fprintf(stderr, "  --net:fwd [udp:]<host port>:<guest port>\n");
                fprintf(stderr, "                         Forwards a port on the host's localhost to the guest.\n");
            }
            fprintf(stderr, "\n");
            if(machineSupport.display)
            {
                fprintf(stderr, "  --display [on|off|required]\n");
                fprintf(stderr, "                         Sets up the display, if it is required, but not available\n");
                fprintf(stderr, "                         on a certain distribution it will error.\n");
                fprintf(stderr, "  --cores <n>            Number of cores, 1 to 16 (default 4); all but the first\n");
                fprintf(stderr, "                         wait for a STARTUP IPI from their Local APIC.\n");
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
        else if(strcmp(argv[i], "--nvram") == 0 && i + 1 < argc)
        {
            machineOptions.nvramPath = argv[++i];
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
                diag_error(NULL, "illegal value type used", argv[i]);
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
                diag_error(NULL, "illegal value type used", argv[i]);
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
                    diag_warn(NULL, "--display flag is not supported in this distribution of the emex64 toolchain");
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
                    diag_error(NULL, "-display flag is not supported in this distribution of the emex64 toolchain");
                    return 1;
                }
                machineOptions.displayOptions.enabled = true;
            }
            else
            {
                diag_error(NULL, "unknown argument supplied to '--display': '%s'", argv[i + 1]);
                return 1;
            }
            i++;
        }
        else if((strcmp(argv[i], "--sata") == 0 || strcmp(argv[i], "--sata:ro") == 0) && i + 1 < argc)
        {
            if(machineOptions.sataDiskCount >= E64_MACHINE_MAX_SATA_DISKS)
            {
                diag_error(NULL, "too many SATA disks, at most %d are supported", E64_MACHINE_MAX_SATA_DISKS);
                return 1;
            }
            machineOptions.sataDisks[machineOptions.sataDiskCount].path = argv[i + 1];
            machineOptions.sataDisks[machineOptions.sataDiskCount].readOnly = strcmp(argv[i], "--sata:ro") == 0;
            machineOptions.sataDiskCount++;
            i++;
        }
        else if((strcmp(argv[i], "--usb") == 0 || strcmp(argv[i], "--usb:ro") == 0) && i + 1 < argc)
        {
            if(machineOptions.usbDiskCount >= E64_MACHINE_MAX_USB_DISKS)
            {
                diag_error(NULL, "too many USB drives, at most %d are supported", E64_MACHINE_MAX_USB_DISKS);
                return 1;
            }
            machineOptions.usbDisks[machineOptions.usbDiskCount].path = argv[i + 1];
            machineOptions.usbDisks[machineOptions.usbDiskCount].readOnly = strcmp(argv[i], "--usb:ro") == 0;
            machineOptions.usbDiskCount++;
            i++;
        }
        else if(strcmp(argv[i], "--sound") == 0 && i + 1 < argc)
        {
            if(strcmp(argv[i + 1], "on") == 0)
            {
                machineOptions.soundOptions.enabled = true;
            }
            else if(strcmp(argv[i + 1], "off") == 0)
            {
                machineOptions.soundOptions.enabled = false;
            }
            else
            {
                diag_error(NULL, "unknown argument supplied to '--sound': '%s' (on or off)", argv[i + 1]);
                return 1;
            }
            i++;
        }
        else if(strcmp(argv[i], "--sound:wav") == 0 && i + 1 < argc)
        {
            machineOptions.soundOptions.enabled = true;
            machineOptions.soundOptions.wavPath = argv[i + 1];
            i++;
        }
        else if(strcmp(argv[i], "--net") == 0 && i + 1 < argc)
        {
            if(!machineSupport.network)
            {
                diag_error(NULL, "--net is not supported in this distribution of the emex64 toolchain (no libslirp)");
                return 1;
            }
            if(strcmp(argv[i + 1], "user") != 0)
            {
                diag_error(NULL, "unknown argument supplied to '--net': '%s' (user is the one there is)", argv[i + 1]);
                return 1;
            }
            machineOptions.networkOptions.enabled = true;
            i++;
        }
        else if(strcmp(argv[i], "--net:fwd") == 0 && i + 1 < argc)
        {
            const char *spec = argv[i + 1];
            Boolean udp = false;
            if(strncmp(spec, "udp:", 4) == 0)
            {
                udp = true;
                spec += 4;
            }
            else if(strncmp(spec, "tcp:", 4) == 0)
            {
                spec += 4;
            }
            unsigned int hostPort = 0, guestPort = 0;
            char end = 0;
            if(sscanf(spec, "%u:%u%c", &hostPort, &guestPort, &end) != 2 ||
               hostPort == 0 || hostPort > 65535 || guestPort == 0 || guestPort > 65535)
            {
                diag_error(NULL, "'--net:fwd' takes [udp:]<host port>:<guest port>, not '%s'", argv[i + 1]);
                return 1;
            }
            E64NetworkOptions *n = &machineOptions.networkOptions;
            if(n->forwardCount >= E64_MACHINE_MAX_HOST_FORWARDS)
            {
                diag_error(NULL, "too many forwarded ports, at most %d are supported", E64_MACHINE_MAX_HOST_FORWARDS);
                return 1;
            }
            n->forwards[n->forwardCount].udp = udp;
            n->forwards[n->forwardCount].hostPort = (UInt16)hostPort;
            n->forwards[n->forwardCount].guestPort = (UInt16)guestPort;
            n->forwardCount++;
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
                diag_error(NULL, "unknown argument supplied to '--keyboard': '%s'", argv[i + 1]);
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
                diag_error(NULL, "unknown argument supplied to '--mouse': '%s'", argv[i + 1]);
                return 1;
            }
            i++;
        }
        else if(strcmp(argv[i], "--cores") == 0 && i + 1 < argc)
        {
            parser_return_t pr = parse_value_from_string(argv[++i]);
            if(pr.type != emexParserValueTypeNumber || pr.value < 1 || pr.value > 16)
            {
                diag_error(NULL, "'--cores' takes a number from 1 to 16, not '%s'", argv[i]);
                return 1;
            }
            machineOptions.coreCount = (UInt8)pr.value;
        }
        else if(strcmp(argv[i], "--display:resolution") == 0 && i + 2 < argc)
        {
            parser_return_t pr_width = parse_value_from_string(argv[++i]);
            parser_return_t pr_height = parse_value_from_string(argv[++i]);

            if(!machineSupport.display)
            {
                diag_warn(NULL, "--display:resolution flag is not supported in this distribution of the emex64 toolchain");
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
                diag_error(NULL, "illegal arguments supplied to '--display:resolution': '%s' and '%s'", argv[i - 1], argv[i - 2]);
                return 1;
            }
        }
        else
        {
            diag_error(NULL, "unknown option '%s'", argv[i]);
            return 1;
        }
    }

    if(machineOptions.networkOptions.enabled && machineOptions.networkOptions.forwardCount == 0)
    {
        machineOptions.networkOptions.forwards[0].udp = false;
        machineOptions.networkOptions.forwards[0].hostPort = 2222;
        machineOptions.networkOptions.forwards[0].guestPort = 22;
        machineOptions.networkOptions.forwardCount = 1;
    }

    char *nvram_path = NULL;
    if(machineOptions.nvramPath == NULL && firmware_image_path != NULL)
    {
        size_t len = strlen(firmware_image_path) + sizeof(".nvram");
        nvram_path = malloc(len);
        if(nvram_path != NULL)
        {
            snprintf(nvram_path, len, "%s.nvram", firmware_image_path);
            machineOptions.nvramPath = nvram_path;
        }
    }

    /* creating new emex64 virtual machine */
    E64MachineRef machine = E64MachineCreateWithOptions(kEFAllocatorDefault, machineOptions);
    if(machine == NULL)
    {
        diag_error(NULL, "failed to allocated machine");
        return 1;
    }

    E64MemoryRef memory = E64MachineGetMemory(machine);
    if(memory == NULL)
    {
        diag_error(NULL, "failed to aquire memory from machine");
        EFRelease(machine);
        return 1;
    }

    if(firmware_image_path != NULL)
    {
        EFAUTOREL EFStringRef firmwareImagePathStr = EFStringCreateWithCString(kEFAllocatorDefault, firmware_image_path, kEFStringEncodingUTF8);
        EFAUTOREL EFFileRef file = EFFileCreateWithPath(kEFAllocatorDefault, EFFilePolicyInData, firmwareImagePathStr);
        if(file == NULL || !E64MemoryLoadImage(memory, file))
        {
            diag_error(NULL, "failed to load firmware image");
            EFRelease(machine);
            return 1;
        }
    }

    /* running core 0 */
    E64CoreRef core = E64MachineGetCore(machine);
    if(core == NULL)
    {
        diag_error(NULL, "failed to aquire core from machine");
        EFRelease(machine);
        return 1;
    }
    E64Exception exception = E64CoreExecute(core);
    EFRelease(machine);
    return exception == kE64ExceptionNone ? 0 : 1;
}
