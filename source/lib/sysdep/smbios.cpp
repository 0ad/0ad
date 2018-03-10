/* Copyright (C) 2018 Wildfire Games.
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
 * provide access to System Management BIOS information
 */

#include "precompiled.h"
#include "lib/sysdep/smbios.h"

#include "lib/bits.h"
#include "lib/alignment.h"
#include "lib/byte_order.h"	// FOURCC_BE
#include "lib/module_init.h"

#if OS_WIN
# include "lib/sysdep/os/win/wutil.h"
# include "lib/sysdep/os/win/wfirmware.h"
#endif

#include <sstream>

namespace SMBIOS {

//-----------------------------------------------------------------------------
// GetTable

#if OS_WIN

static Status GetTable(wfirmware::Table& table)
{
	// (MSDN mentions 'RSMB', but multi-character literals are implementation-defined.)
	const DWORD provider = FOURCC_BE('R','S','M','B');

	// (MSDN says this will be 0, but we'll retrieve it for 100% correctness.)
	wfirmware::TableIds tableIds = wfirmware::GetTableIDs(provider);
	if(tableIds.empty())
		return ERR::_1;	// NOWARN (happens on 32-bit XP)

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
	ENSURE(table.size() == sizeof(WmiHeader) + wmiHeader->length);
	memmove(&table[0], &table[sizeof(WmiHeader)], table.size()-sizeof(WmiHeader));

	return INFO::OK;
}

#endif	// OS_WIN


//-----------------------------------------------------------------------------
// strings

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


// storage for all structures' strings (must be copied from the original
// wfirmware table since its std::vector container cannot be stored in a
// static variable because we may be called before _cinit)
static char* stringStorage;
static char* stringStoragePos;

// pointers to dynamically allocated structures
static Structures g_Structures;

static void Cleanup()	// called via atexit
{
	SAFE_FREE(stringStorage);
	stringStoragePos = 0;

	// free each allocated structure
#define STRUCTURE(name, id)\
	while(g_Structures.name##_)\
	{\
		name* next = g_Structures.name##_->next;\
		SAFE_FREE(g_Structures.name##_);\
		g_Structures.name##_ = next;\
	}
	STRUCTURES
#undef STRUCTURE
}


//-----------------------------------------------------------------------------
// FieldInitializer

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

	template<typename Field>
	void operator()(size_t flags, Field& field, const char* UNUSED(name), const char* UNUSED(units))
	{
		if((flags & F_DERIVED) || data >= end)
		{
			field = Field();
			return;
		}

		Read(field, 0);	// SFINAE
	}

private:
	template<typename T>
	T ReadValue()
	{
		T value;
		memcpy(&value, data, sizeof(value));
		data += sizeof(value);
		return value;
	}

	// construct from SMBIOS representations that don't match the
	// actual type (e.g. enum)
	template<typename Field>
	void Read(Field& field, typename Field::T*)
	{
		field = Field(ReadValue<typename Field::T>());
	}

	template<typename Field>
	void Read(Field& field, ...)
	{
		field = ReadValue<Field>();
	}

	const u8* data;
	const u8* end;
	const Strings& strings;
};


// C++03 14.7.3(2): "An explicit specialization shall be declared [..] in the
// namespace of which the enclosing class [..] is a member.

// (this specialization avoids a "forcing value to bool true or false" warning)
template<>
void FieldInitializer::operator()<bool>(size_t flags, bool& UNUSED(t), const char* UNUSED(name), const char* UNUSED(units))
{
	// SMBIOS doesn't specify any individual booleans, so we're only called for
	// derived fields and don't need to do anything.
	ENSURE(flags & F_DERIVED);
}

template<>
void FieldInitializer::operator()<const char*>(size_t flags, const char*& t, const char* UNUSED(name), const char* UNUSED(units))
{
	t = 0;	// (allow immediate `return' when the string is found to be invalid)

	u8 number;
	operator()(flags, number, 0, 0);
	if(number == 0)	// no string given
		return;

	if(number > strings.size())
	{
		debug_printf("SMBIOS: invalid string number %d (count=%d)\n", number, (int)strings.size());
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
	p.size = size_t(p.encodedSize+1) * 64*KiB;
}

template<>
void Fixup<Processor>(Processor& p)
{
	p.populated = (p.status & 0x40) != 0;
	p.status = (ProcessorStatus)bits(p.status, 0, 2);

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
		u64 operator()(u16 size) const
		{
			const size_t granularity = IsBitSet(size, 15)? 64*KiB : 1*KiB;
			return u64(bits(size, 0, 14)) * granularity;
		}
	};
	p.maxSize = DecodeSize()(p.maxSize16);
	p.installedSize = DecodeSize()(p.installedSize16);
	p.level = bits(p.configuration, 0, 2)+1;
	p.location = (CacheLocation)bits(p.configuration, 5, 6);
	p.mode = (CacheMode)bits(p.configuration, 8, 9);
	p.configuration = (CacheConfigurationFlags)(p.configuration & ~0x367);
}

template<>
void Fixup<SystemSlot>(SystemSlot& p)
{
	// (only initialize function and device numbers if functionAndDeviceNumber is valid)
	if(p.functionAndDeviceNumber != 0xFF)
	{
		p.functionNumber = bits(p.functionAndDeviceNumber, 0, 2);
		p.deviceNumber = bits(p.functionAndDeviceNumber, 3, 7);
	}
}

template<>
void Fixup<OnBoardDevices>(OnBoardDevices& p)
{
	p.enabled = (p.type.value & 0x80) != 0;
	p.type = (OnBoardDeviceType)(p.type & ~0x80);
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
	p.rank = bits(p.attributes, 0, 3);
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
	p.status = (State)bits(p.locationAndStatus, 5, 7);
}

template<>
void Fixup<CoolingDevice>(CoolingDevice& p)
{
	p.type = (CoolingDeviceType)bits(p.typeAndStatus, 0, 4);
	p.status = (State)bits(p.typeAndStatus, 5, 7);
}

template<>
void Fixup<TemperatureProbe>(TemperatureProbe& p)
{
	p.location = (TemperatureProbeLocation)bits(p.locationAndStatus, 0, 4);
	p.status = (State)bits(p.locationAndStatus, 5, 7);
}

template<>
void Fixup<SystemPowerSupply>(SystemPowerSupply& p)
{
	p.type = (SystemPowerSupplyType)bits(p.characteristics, 10, 13);
	p.status = (State)bits(p.characteristics, 7, 9);
	p.inputSwitching = (SystemPowerSupplyInputSwitching)bits(p.characteristics, 3, 6);
	p.characteristics = bits(p.characteristics, 0, 2);
}

template<>
void Fixup<OnboardDevices2>(OnboardDevices2& p)
{
	p.enabled = IsBitSet(p.type, 7);
	p.type = (OnBoardDeviceType)bits(p.type, 0, 6);
	p.deviceNumber = bits(p.functionAndDeviceNumber, 3, 7);
	p.functionNumber = bits(p.functionAndDeviceNumber, 0, 2);
}


//-----------------------------------------------------------------------------
// InitStructures

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


static Status InitStructures()
{
#if OS_WIN
	wfirmware::Table table;
	RETURN_STATUS_IF_ERR(GetTable(table));
#else
	std::vector<u8> table;
	return ERR::NOT_SUPPORTED;
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
			debug_printf("SMBIOS: table not terminated\n");
			break;
		}
		if(header->id == 127)	// end
			break;
		if(header->length < sizeof(Header))
			return ERR::_3; // NOWARN (happens on some unknown BIOS, see http://trac.wildfiregames.com/ticket/2985)

		const Header* next;
		const Strings strings = ExtractStrings(header, (const char*)end, next);

		switch(header->id)
		{
#define STRUCTURE(name, id) case id: AddStructure(header, strings, g_Structures.name##_); break;
			STRUCTURES
#undef STRUCTURE

		default:
			if(32 < header->id && header->id < 126)	// only mention non-proprietary structures of which we are not aware
				debug_printf("SMBIOS: unknown structure type %d\n", header->id);
			break;
		}

		header = next;
	}

	return INFO::OK;
}


//-----------------------------------------------------------------------------
// StringFromEnum

template<class Enum>
std::string StringFromEnum(Enum UNUSED(field))
{
	return "(unknown enumeration)";
}

#define ENUM(enumerator, VALUE)\
	if(field.value == VALUE) /* single bit flag or matching enumerator */\
		return #enumerator;\
	if(!is_pow2(VALUE)) /* these aren't bit flags */\
	{\
		allowFlags = false;\
		string.clear();\
	}\
	if(allowFlags && (field.value & (VALUE)))\
	{\
		if(!string.empty())\
			string += "|";\
		string += #enumerator;\
	}
#define ENUMERATION(name, type)\
	template<>\
	std::string StringFromEnum<name>(name field)\
	{\
		std::string string;\
		bool allowFlags = true;\
		name##_ENUMERATORS\
		/* (don't warn about the value 0, e.g. optional fields) */\
		if(string.empty() && field != 0)\
		{\
			std::stringstream ss;\
			ss << "(unknown " << #name << " " << field.value << ")";\
			return ss.str();\
		}\
		return string;\
	}
ENUMERATIONS
#undef ENUMERATION
#undef ENUM


//-----------------------------------------------------------------------------
// FieldStringizer

class FieldStringizer
{
	NONCOPYABLE(FieldStringizer);	// reference member
public:
	FieldStringizer(std::stringstream& ss)
		: ss(ss)
	{
	}

	template<typename Field>
	void operator()(size_t flags, Field& field, const char* name, const char* units)
	{
		if(flags & F_INTERNAL)
			return;

		Write(flags, field, name, units, 0);	// SFINAE
	}

	// special case for sizes [bytes]
	template<typename T>
	void operator()(size_t flags, Size<T>& size, const char* name, const char* units)
	{
		if(flags & F_INTERNAL)
			return;

		const u64 value = (u64)size.value;
		if(value == 0)
			return;

		u64 divisor;
		if(value > GiB)
		{
			divisor = GiB;
			units = " GiB";
		}
		else if(value > MiB)
		{
			divisor = MiB;
			units = " MiB";
		}
		else if(value > KiB)
		{
			divisor = KiB;
			units = " KiB";
		}
		else
		{
			divisor = 1;
			units = " bytes";
		}

		WriteName(name);

		// (avoid floating-point output unless division would truncate the value)
		if(value % divisor == 0)
			ss << (value/divisor);
		else
			ss << (double(value)/divisor);

		WriteUnits(units);
	}

private:
	void WriteName(const char* name)
	{
		ss << "  ";	// indent
		ss << name << ": ";
	}

	void WriteUnits(const char* units)
	{
		ss << units;
		ss << "\n";
	}

	// enumerations and bit flags
	template<typename Field>
	void Write(size_t UNUSED(flags), Field& field, const char* name, const char* units, typename Field::Enum*)
	{
		// 0 usually means "not included in structure", but some packed
		// enumerations actually use that value. therefore, only skip this
		// field if it is zero AND no matching enumerator is found.
		const std::string string = StringFromEnum(field);
		if(string.empty())
			return;

		WriteName(name);
		ss << StringFromEnum(field);
		WriteUnits(units);
	}

	// all other field types
	template<typename Field>
	void Write(size_t flags, Field& field, const char* name, const char* units, ...)
	{
		// SMBIOS uses the smallest and sometimes also largest representable
		// signed/unsigned value to indicate `unknown' (except enumerators -
		// but those are handled in the other function overload), so skip them.
		if(field == std::numeric_limits<Field>::min() || field == std::numeric_limits<Field>::max())
			return;

		WriteName(name);

		if(flags & F_HEX)
			ss << std::hex << std::uppercase;

		if(sizeof(field) == 1)	// avoid printing as a character
			ss << unsigned(field);
		else
			ss << field;

		if(flags & F_HEX)
			ss << std::dec;	// (revert to decimal, e.g. for displaying sizes)

		WriteUnits(units);
	}

	std::stringstream& ss;
};

template<>
void FieldStringizer::operator()<bool>(size_t flags, bool& value, const char* name, const char* units)
{
	if(flags & F_INTERNAL)
		return;

	WriteName(name);
	ss << (value? "true" : "false");
	WriteUnits(units);
}

template<>
void FieldStringizer::operator()<Handle>(size_t flags, Handle& handle, const char* name, const char* units)
{
	if(flags & F_INTERNAL)
		return;

	// don't display useless handles
	if(handle.value == 0 || handle.value == 0xFFFE || handle.value == 0xFFFF)
		return;

	WriteName(name);
	ss << handle.value;
	WriteUnits(units);
}


template<>
void FieldStringizer::operator()<const char*>(size_t flags, const char*& value, const char* name, const char* units)
{
	if(flags & F_INTERNAL)
		return;

	// don't display useless strings
	if(value == 0)
		return;
	std::string string(value);
	const size_t lastChar = string.find_last_not_of(' ');
	if(lastChar == std::string::npos)	// nothing but spaces
		return;
	string.resize(lastChar+1);	// strip trailing spaces
	if(!strcasecmp(value, "To Be Filled By O.E.M."))
		return;

	WriteName(name);
	ss << "\"" << string << "\"";
	WriteUnits(units);
}


//-----------------------------------------------------------------------------
// public interface

const Structures* GetStructures()
{
	static ModuleInitState initState;
	Status ret = ModuleInit(&initState, InitStructures);
	// (callers have to check if member pointers are nonzero anyway, so
	// we always return a valid pointer to simplify most use cases.)
	UNUSED2(ret);
	return &g_Structures;
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
	std::stringstream ss;
#define STRUCTURE(name, id) StringizeStructure(#name, structures->name##_, ss);
	STRUCTURES
#undef STRUCTURE

	return ss.str();
}

}	// namespace SMBIOS
