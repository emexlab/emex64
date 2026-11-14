/** @file
  Copyright (C) 2026 emexlab
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Base.h>
#include <Library/IoLib.h>
#include <Library/SerialPortLib.h>
#include <Emex64Platform.h>

RETURN_STATUS EFIAPI SerialPortInitialize(VOID)
{
  return RETURN_SUCCESS;
}

UINTN EFIAPI SerialPortWrite(IN UINT8 *Buffer,
                             IN UINTN NumberOfBytes)
{
    UINTN  Index;

    if(Buffer == NULL)
    {
        return 0;
    }

    for(Index = 0; Index < NumberOfBytes; Index++)
    {
        while((MmioRead64(EMEX64_UART_STATUS) & EMEX64_UART_STATUS_TX_EMPTY) == 0)
        {
        }
        MmioWrite64(EMEX64_UART_DATA, Buffer[Index]);
    }

    return NumberOfBytes;
}

UINTN EFIAPI SerialPortRead(OUT UINT8 *Buffer,
                            IN UINTN NumberOfBytes)
{
    UINTN Index;

    if(Buffer == NULL)
    {
        return 0;
    }

    for(Index = 0; Index < NumberOfBytes; Index++)
    {
        while((MmioRead64 (EMEX64_UART_STATUS) & EMEX64_UART_STATUS_RX_READY) == 0)
        {
        }
        Buffer[Index] = (UINT8)MmioRead64 (EMEX64_UART_DATA);
        if(Buffer[Index] == '\n')
        {
            Buffer[Index] = '\r';
        }
    }

    return NumberOfBytes;
}

BOOLEAN EFIAPI SerialPortPoll(VOID)
{
    return (MmioRead64(EMEX64_UART_STATUS) & EMEX64_UART_STATUS_RX_READY) != 0;
}

RETURN_STATUS EFIAPI SerialPortSetControl(IN UINT32 Control)
{
    return RETURN_UNSUPPORTED;
}

RETURN_STATUS EFIAPI SerialPortGetControl(OUT UINT32 *Control)
{
    *Control = EFI_SERIAL_OUTPUT_BUFFER_EMPTY;
    if(!SerialPortPoll())
    {
        *Control |= EFI_SERIAL_INPUT_BUFFER_EMPTY;
    }
    return RETURN_SUCCESS;
}

RETURN_STATUS EFIAPI SerialPortSetAttributes(IN OUT UINT64 *BaudRate,
                                             IN OUT UINT32 *ReceiveFifoDepth,
                                             IN OUT UINT32 *Timeout,
                                             IN OUT EFI_PARITY_TYPE *Parity,
                                             IN OUT UINT8 *DataBits,
                                             IN OUT EFI_STOP_BITS_TYPE *StopBits)
{
    if(*BaudRate == 0)
    {
        *BaudRate = 115200;
    }

    if(*ReceiveFifoDepth == 0)
    {
        *ReceiveFifoDepth = 1;
    }

    if(*Parity == DefaultParity)
    {
        *Parity = NoParity;
    }

    if(*DataBits == 0)
    {
        *DataBits = 8;
    }

    if(*StopBits == DefaultStopBits)
    {
        *StopBits = OneStopBit;
    }

    return RETURN_SUCCESS;
}
