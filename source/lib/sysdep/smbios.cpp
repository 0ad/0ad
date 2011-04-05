/* Copyright (c) 2011 Wildfire Games
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

#include "precompiled.h"
#include "smbios.h"

#include "lib/bits.h"
#include "lib/byte_order.h"	// FOURCC_BE
#include "lib/module_init.h"

#if OS_WIN
# include "lib/sysdep/os/win/wutil.h"
# include "lib/sysdep/os/win/wfirmware.h"
#endif

namespace SMBIOS {

//-----------------------------------------------------------------------------

#if OS_WIN

static LibError GetTable(wfirmware::Table& table)
{
	// (MSDN mentions 'RSMB', but multi-character literals are implementation-defined.)
	const DWORD provider = FOURCC_BE('R','S','M','B');

	// (MSDN says this will be 0, but we'll retrieve it for 100% correctness.)
	wfirmware::TableIds tableIds = wfirmware::GetTableIDs(provider);
	if(tableIds.empty())
		WARN_RETURN(ERR::_1);

	table = wfirmware::GetTable(provider, tableIds[0]);
	if(table.empty())
		WARN_RETURN(ERR::_2);

	// strip the WmiHeader
	struct WmiHeader
	{
		u8 used20CallingMethod;
		u8 majorVersion;
		u8 minorVersion;
		u8 dmiRevision;
		u32 length;
	};
	const WmiHeader* wmiHeader = (const WmiHeader*)&table[0];
	debug_assert(table.size() == sizeof(WmiHeader) + wmiHeader->length);
	memmove(&table[0], &table[sizeof(WmiHeader)], table.size()-sizeof(WmiHeader));

	return INFO::OK;
}

#endif	// OS_WIN


//-----------------------------------------------------------------------------

// pointers to the strings (if any) at the end of an SMBIOS structure
typedef std::vector<const char*> Strings;

static Strings ExtractStrings(const Header* header, const char* end, const Header*& next)
{
	Strings strings;

	const char* pos = ((const char*)header) + header->length;
	while(pos <= end-2)
	{
		if(*pos == '\0')
		{
			pos++;
			if(*pos == 0)
			{
				pos++;
				break;
			}
		}

		strings.push_back(pos);
		pos += strlen(pos);
	}

	next = (const Header*)pos;
	return strings;
}


// storage for all structures' strings
static char* stringStorage;
static char* stringStoragePos;

// pointers to dynamically allocated structures
static Structures structures;

static void Cleanup()
{
	SAFE_FREE(stringStorage);
	stringStoragePos = 0;

	// free each allocated structure
#define STRUCTURE(name, id) SAFE_FREE(structures.name##_);
	STRUCTURES
#undef STRUCTURE
}


// define function templates that invoke a Visitor for each of a structure's fields
#define FIELD(flags, type, name, units) visitor(flags, p.name, #name, units);
#define STRUCTURE(name, id) template<class Visitor> void VisitFields(name& p, Visitor& visitor) { name##_FIELDS }
STRUCTURES
#undef STRUCTURE
#undef FIELD


// initialize each of a structure's fields by copying from the SMBIOS data
class FieldInitializer
{
	NONCOPYABLE(FieldInitializer);	// reference member
public:
	FieldInitializer(const Header* header, const Strings& strings)
		: data((const u8*)(header+1))
		, end((const u8*)header + header->length)
		, strings(strings)
	{
	}

	template<typename T>
	void operator()(size_t flags, T& t, const char* UNUSED(name), const char* UNUSED(units))
	{
		if((flags & F_DERIVED) || data >= end)
		{
			t = T();
			return;
		}

		if(flags & F_ENUM)
			t = T(*data++);
		else
		{
			memcpy(&t, data, sizeof(t));
			data += sizeof(t);
		}
	}

private:
	const u8* data;
	const u8* end;
	const Strings& strings;
};


// C++03 14.7.3(2): "An explicit specialization shall be declared [..] in the
// namespace of which the enclosing class [..] is a member.

// avoid "forcing value to bool true or false" warning
template<>
void FieldInitializer::operator()<bool>(size_t flags, bool& UNUSED(t), const char* UNUSED(name), const char* UNUSED(units))
{
	// SMBIOS doesn't specify any single booleans, so we're only called for
	// derived fields and don't need to do anything.
	debug_assert(flags & F_DERIVED);
}

template<>
void FieldInitializer::operator()<const char*>(size_t flags, const char*& t, const char* UNUSED(name), const char* UNUSED(units))
{
	u8 number;
	operator()(flags, number, 0, 0);
	if(number == 0)	// no string given
	{
		t = 0;
		return;
	}

	if(number > strings.size())
	{
		debug_printf(L"SMBIOS: invalid string number %d (count=%d)\n", number, strings.size());
		t = 0;
		return;
	}

	if(strings[number-1] == 0)
	{
		t = 0;
		return;
	}

	// copy to stringStorage
	strcpy(stringStoragePos, strings[number-1]);
	t = stringStoragePos;
	stringStoragePos += strlen(t)+1;
}


//-----------------------------------------------------------------------------
// Fixup (e.g. compute derived fields)

template<class Structure>
void Fixup(Structure& UNUSED(structure))
{
	// primary template: do nothing
}

template<>
void Fixup<Bios>(Bios& p)
{
	p.size = u64(p.encodedSize+1) * 64*KiB;
}

template<>
void Fixup<Processor>(Processor& p)
{
	p.populated = (p.status.value & 0x40) != 0;
	p.status = (ProcessorStatus)bits((size_t)p.status.value, 0, 2);

	if(p.voltage & 0x80)
		p.voltage &= ~0x80;
	else
	{
		// (arbitrarily) report the lowest supported value
		if(IsBitSet(p.voltage, 0))
			p.voltage = 50;
		if(IsBitSet(p.voltage, 1))
			p.voltage = 33;
		if(IsBitSet(p.voltage, 2))
			p.voltage = 29;
	}
}

template<>
void Fixup<Cache>(Cache& p)
{
	struct DecodeSize
	{
		size_t operator()(u16 size) const
		{
			const size_t granularity = IsBitSet(size, 15)? 64*KiB : 1*KiB;
			return size_t(bits(size, 0, 14)) * granularity;
		}
	};
	p.maxSize = DecodeSize()(p.maxSize16);
	p.installedSize = DecodeSize()(p.installedSize16);
	p.level = bits(p.configuration, 0, 2)+1;
	p.location = (CacheLocation)bits(p.configuration, 5, 6);
	p.mode = (CacheMode)bits(p.configuration, 8, 9);
	p.configuration &= ~0x367;
}

template<>
void Fixup<SystemSlot>(SystemSlot& p)
{
	p.functionNumber = bits(p.functionAndDeviceNumber, 0, 2);
	p.deviceNumber = bits(p.functionAndDeviceNumber, 3, 7);
}

template<>
void Fixup<OnBoardDevices>(OnBoardDevices& p)
{
	p.enabled = (p.type.value & 0x80) != 0;
	p.type = (OnBoardDeviceType)(p.type.value & ~0x80);
}

template<>
void Fixup<MemoryArray>(MemoryArray& p)
{
	if(p.maxCapacity32 != (u32)INT32_MIN)
		p.maxCapacity = u64(p.maxCapacity32) * KiB;
}

template<>
void Fixup<MemoryDevice>(MemoryDevice& p)
{
	if(p.size16 != INT16_MAX)
		p.size = u64(bits(p.size16, 0, 14)) * (IsBitSet(p.size16, 15)? 1*KiB : 1*MiB);
	else
		p.size = u64(bits(p.size32, 0, 30)) * MiB;
}

template<>
void Fixup<MemoryArrayMappedAddress>(MemoryArrayMappedAddress& p)
{
	if(p.startAddress32 != UINT32_MAX)
		p.startAddress = u64(p.startAddress32) * KiB;
	if(p.endAddress32 != UINT32_MAX)
		p.endAddress = u64(p.endAddress32) * KiB;
}

template<>
void Fixup<MemoryDeviceMappedAddress>(MemoryDeviceMappedAddress& p)
{
	if(p.startAddress32 != UINT32_MAX)
		p.startAddress = u64(p.startAddress32) * KiB;
	if(p.endAddress32 != UINT32_MAX)
		p.endAddress = u64(p.endAddress32) * KiB;
}

template<>
void Fixup<VoltageProbe>(VoltageProbe& p)
{
	p.location = (VoltageProbeLocation)bits(p.locationAndStatus, 0, 4);
	p.status = (Status)bits(p.locationAndStatus, 5, 7);
}

template<>
void Fixup<CoolingDevice>(CoolingDevice& p)
{
	p.type = (CoolingDeviceType)bits(p.typeAndStatus, 0, 4);
	p.status = (Status)bits(p.typeAndStatus, 5, 7);
}

template<>
void Fixup<TemperatureProbe>(TemperatureProbe& p)
{
	p.location = (TemperatureProbeLocation)bits(p.locationAndStatus, 0, 4);
	p.status = (Status)bits(p.locationAndStatus, 5, 7);
}


//-----------------------------------------------------------------------------

template<class Structure>
void AddStructure(const Header* header, const Strings& strings, Structure*& listHead)
{
	Structure* const p = (Structure*)calloc(1, sizeof(Structure));	// freed in Cleanup
	p->header = *header;

	if(listHead)
	{
		// insert at end of list to preserve order of caches/slots
		Structure* last = listHead;
		while(last->next)
			last = last->next;
		last->next = p;
	}
	else
		listHead = p;

	FieldInitializer fieldInitializer(header, strings);
	VisitFields(*p, fieldInitializer);

	Fixup(*p);
}


static LibError InitStructures()
{
#if OS_WIN
	wfirmware::Table table;
	RETURN_ERR(GetTable(table));
#else
	std::vector<u8> table;
	return ERR::NOT_IMPLEMENTED;
#endif

	// (instead of counting the total string size, just use the
	// SMBIOS size - typically 1-2 KB - as an upper bound.) 
	stringStoragePos = stringStorage = (char*)calloc(table.size(), sizeof(char));	// freed in Cleanup
	if(!stringStorage)
		WARN_RETURN(ERR::NO_MEM);

	atexit(Cleanup);

	const Header* header = (const Header*)&table[0];
	const Header* const end = (const Header*)(&table[0] + table.size());
	for(;;)
	{
		if(header+1 > end)
		{
			debug_printf(L"SMBIOS: table not terminated\n");
			break;
		}
		if(header->id == 127)	// end
			break;
		if(header->length < sizeof(Header))
			WARN_RETURN(ERR::_3);

		const Header* next;
		const Strings strings = ExtractStrings(header, (const char*)end, next);

		switch(header->id)
		{
#define STRUCTURE(name, id) case id: AddStructure(header, strings, structures.name##_); break;
			STRUCTURES
#undef STRUCTURE

		default:
			if(32 < header->id && header->id < 126)	// only mention non-proprietary structures of which we are not aware
				debug_printf(L"SMBIOS: unknown structure type %d\n", header->id);
			break;
		}

		header = next;
	}

	return INFO::OK;
}


const Structures* GetStructures()
{
	static ModuleInitState initState;
	LibError ret = ModuleInit(&initState, InitStructures);
	if(ret != INFO::OK)
		return 0;
	return &structures;
}


//-----------------------------------------------------------------------------

template<class Enum>
static inline const char* EnumeratorFromValue(Enum UNUSED(value))
{
	return 0;
}

// define specializations of EnumeratorFromValue
#define ENUM(enumerator, value) case value: return #enumerator;
#define ENUMERATION(name)\
	template<>\
	inline const char* EnumeratorFromValue<name>(name value)\
	{\
		switch(value.value)\
		{\
			name##_ENUMERATORS \
			default: return 0;\
		}\
	}
ENUMERATIONS
#undef ENUMERATION
#undef ENUM


class FieldStringizer
{
	NONCOPYABLE(FieldStringizer);	// reference member
public:
	FieldStringizer(std::stringstream& ss)
		: ss(ss)
	{
	}

	template<typename T>
	void operator()(size_t flags, T& value, const char* name, const char* units)
	{
		if(flags & F_INTERNAL)
			return;

		static u64 zero;
		cassert_dependent(sizeof(value) <= sizeof(zero));
		if((flags & F_ENUM) == 0 && memcmp(&value, &zero, sizeof(value)) == 0)
			return;

		ss << "  ";	// indent
		ss << name << ": ";
		if(flags & (F_HEX|F_FLAGS))
			ss << std::hex << std::uppercase;
		else
			ss << std::dec;

		if(flags & F_ENUM)
		{
			const char* name = EnumeratorFromValue(value);
			if(name)
				ss << name;
			else
				ss << value;
		}
		else if(flags & F_SIZE)
		{
			u64 value64 = (u64)value;
			if(value64 > GiB)
				ss << value64/GiB << " GiB";
			else if(value64 > MiB)
				ss << value64/MiB << " MiB";
			else if(value64 > KiB)
				ss << value64/KiB << " KiB";
			else
				ss << value << " bytes";
		}
		else if(sizeof(value) == 1)	// avoid printing as a character
			ss << (unsigned)value;
		else
			ss << value;

		ss << units << "\n";
	}

private:
	std::stringstream& ss;
};

template<>
void FieldStringizer::operator()<bool>(size_t flags, bool& value, const char* name, const char* units)
{
	debug_assert(units[0] == '\0');	// why would this be specified?
	if(flags & F_INTERNAL)
		return;
	ss << "  ";	// indent
	ss << name << ": \"" << (value? "true" : "false") << "\"\n";
}

template<>
void FieldStringizer::operator()<Handle>(size_t flags, Handle& handle, const char* name, const char* units)
{
	debug_assert(units[0] == '\0');	// why would this be specified?
	if(flags & F_INTERNAL || handle.value == 0xFFFE || handle.value == 0xFFFF)
		return;
	ss << "  ";	// indent
	ss << name << ": " << handle.value << "\n";
}

template<>
void FieldStringizer::operator()<const char*>(size_t flags, const char*& value, const char* name, const char* units)
{
	debug_assert(units[0] == '\0');	// why would this be specified for strings?
	if((flags & F_INTERNAL) || value == 0)
		return;

	// don't display useless strings
	const ssize_t length = (ssize_t)strlen(value);
	if(std::count(value, value+length, ' ') == length)	// all spaces
		return;
	if(strcmp(value, "To Be Filled By O.E.M.") == 0)
		return;

	ss << "  ";	// indent
	ss << name << ": \"" << value << "\"\n";
}


template<class Structure>
void StringizeStructure(const char* name, Structure* p, std::stringstream& ss)
{
	for(; p; p = p->next)
	{
		ss << "\n[" << name << "]\n";
		FieldStringizer fieldStringizer(ss);
		VisitFields(*p, fieldStringizer);
	}
}


std::string StringizeStructures(const Structures* structures)
{
	if(!structures)
		return "(null)";

	std::stringstream ss;
#define STRUCTURE(name, id) StringizeStructure(#name, structures->name##_, ss);
	STRUCTURES
#undef STRUCTURE

	return ss.str();
}

}	// namespace SMBIOS
