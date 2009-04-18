/* Copyright (C) 2009 Wildfire Games.
 * This file is part of 0 A.D.
 *
 * 0 A.D. is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * 0 A.D. is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 0 A.D.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * Aken driver interface
 */

// Aken - custodian of the ferryboat to the underworld in Egyptian mythology,
// and a driver that shuttles between applications and kernel mode resources.

#ifndef INCLUDED_AKEN
#define INCLUDED_AKEN

#define AKEN_NAME "Aken"

// device type
#define FILE_DEVICE_AKEN 53498	// in the "User Defined" range."

#define AKEN_IOCTL 0x800	// 0x800..0xFFF are for 'customer' use.

#define IOCTL_AKEN_READ_PORT           CTL_CODE(FILE_DEVICE_AKEN, AKEN_IOCTL+0, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_AKEN_WRITE_PORT          CTL_CODE(FILE_DEVICE_AKEN, AKEN_IOCTL+1, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_AKEN_MAP                 CTL_CODE(FILE_DEVICE_AKEN, AKEN_IOCTL+2, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_AKEN_UNMAP               CTL_CODE(FILE_DEVICE_AKEN, AKEN_IOCTL+3, METHOD_BUFFERED, FILE_ANY_ACCESS)


// input and output data structures for the IOCTLs

#pragma pack(push, 1)

struct AkenReadPortIn
{
	USHORT port;
	UCHAR numBytes;
};

struct AkenReadPortOut
{
	DWORD32 value;
};

struct AkenWritePortIn
{
	DWORD32 value;
	USHORT port;
	UCHAR numBytes;
};

struct AkenMapIn
{
	// note: fixed-width types allow the 32 or 64-bit Mahaf wrapper to
	// interoperate with the 32 or 64-bit Aken driver.
	DWORD64 physicalAddress;
	DWORD64 numBytes;
};

struct AkenMapOut
{
	DWORD64 virtualAddress;
};

struct AkenUnmapIn
{
	DWORD64 virtualAddress;
};

#pragma pack(pop)

#endif	// #ifndef INCLUDED_AKEN
