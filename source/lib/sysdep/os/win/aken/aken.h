/* Copyright (C) 2010 Wildfire Games.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/*
 * Aken driver interface
 */

// Aken - custodian of the ferryboat to the underworld in Egyptian mythology,
// and a driver that shuttles between applications and kernel mode resources.

#ifndef INCLUDED_AKEN
#define INCLUDED_AKEN

#define AKEN_NAME L"Aken"

// device type
#define FILE_DEVICE_AKEN 53498	// in the "User Defined" range."

#define AKEN_IOCTL 0x800	// 0x800..0xFFF are for 'customer' use.

#define IOCTL_AKEN_READ_PORT           CTL_CODE(FILE_DEVICE_AKEN, AKEN_IOCTL+0, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_AKEN_WRITE_PORT          CTL_CODE(FILE_DEVICE_AKEN, AKEN_IOCTL+1, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_AKEN_MAP                 CTL_CODE(FILE_DEVICE_AKEN, AKEN_IOCTL+2, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_AKEN_UNMAP               CTL_CODE(FILE_DEVICE_AKEN, AKEN_IOCTL+3, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_AKEN_READ_MSR            CTL_CODE(FILE_DEVICE_AKEN, AKEN_IOCTL+4, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_AKEN_WRITE_MSR           CTL_CODE(FILE_DEVICE_AKEN, AKEN_IOCTL+5, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_AKEN_READ_PMC            CTL_CODE(FILE_DEVICE_AKEN, AKEN_IOCTL+6, METHOD_BUFFERED, FILE_ANY_ACCESS)


// input and output data structures for the IOCTLs

#pragma pack(push, 1)

typedef struct AkenReadPortIn_
{
	USHORT port;
	UCHAR numBytes;
}
AkenReadPortIn;

typedef struct AkenReadPortOut_
{
	DWORD32 value;
}
AkenReadPortOut;

typedef struct AkenWritePortIn_
{
	DWORD32 value;
	USHORT port;
	UCHAR numBytes;
}
AkenWritePortIn;

typedef struct AkenMapIn_
{
	// note: fixed-width types allow the 32 or 64-bit Mahaf wrapper to
	// interoperate with the 32 or 64-bit Aken driver.
	DWORD64 physicalAddress;
	DWORD64 numBytes;
}
AkenMapIn;

typedef struct AkenMapOut_
{
	DWORD64 virtualAddress;
}
AkenMapOut;

typedef struct AkenUnmapIn_
{
	DWORD64 virtualAddress;
}
AkenUnmapIn;

typedef struct AkenReadRegisterIn_
{
	DWORD64 reg;
}
AkenReadRegisterIn;

typedef struct AkenReadRegisterOut_
{
	DWORD64 value;
}
AkenReadRegisterOut;

typedef struct AkenWriteRegisterIn_
{
	DWORD64 reg;
	DWORD64 value;
}
AkenWriteRegisterIn;

#pragma pack(pop)

#endif	// #ifndef INCLUDED_AKEN
