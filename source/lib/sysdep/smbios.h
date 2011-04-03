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

#ifndef INCLUDED_SMBIOS
#define INCLUDED_SMBIOS

namespace SMBIOS {

// to introduce another structure:
// 1) add its name and ID here
// 2) add a <name>_FIELDS macro defining its fields
// 3) (optional) add a specialization of Fixup
#define STRUCTURES\
	STRUCTURE(Bios, 0)\
	STRUCTURE(System, 1)\
	STRUCTURE(Baseboard, 2)\
	STRUCTURE(Chassis, 3)\
	STRUCTURE(Processor, 4)\
	/* MemoryController (5) and MemoryModule (6) are obsolete */\
	STRUCTURE(Cache, 7)\
	/* PortConnector (8) is optional */\
	STRUCTURE(SystemSlot, 9)\
	/* OnBoardDevices (10) is obsolete */\
	/* OemStrings (11), SystemConfiguration (12), BiosLanguage (13), GroupAssociations (14), SystemEventLog (15) are optional */\
	STRUCTURE(MemoryArray, 16)\
	STRUCTURE(MemoryDevice, 17)\
	/* MemoryError32 (18) is optional */\
	STRUCTURE(MemoryArrayMappedAddress, 19)\
	STRUCTURE(MemoryDeviceMappedAddress, 20)\
	/* PointingDevice (21), PortableBattery (22), SystemReset (23), HardwareSecurity (24) are optional */\
	/* SystemPowerControls (25), VoltageProbe (26), CoolingDevice (27), TemperatureProbe (28) are optional */\
	/* ElectricalCurrentProbe (29), OutOfBandRemoteAccess (30), BootIntegrityServices (31) are optional */\
	STRUCTURE(SystemBoot, 32)
	/* MemoryError64 (33), ManagementDevice (34), ManagementDeviceComponent (35), ManagementDeviceThreshold (36) are optional */\
	/* MemoryChannel (37), IpmiDevice (38), SystemPowerSupply (39), Additional (40), OnboardDevices2 (41) are optional */\
	/* ManagementControllerHostInterface (42) is optional */

typedef u16 Handle;

// indicates a field (:= member of a structure) is:
enum FieldFlags
{
	// computed via other fields (i.e. should not be copied from the SMBIOS data).
	F_DERIVED  = 0x01,

	// not intended for use by applications (usually because it is
	// superseded by another - possibly derived - field).
	F_INTERNAL = 0x02,

	// an enum type; we'll read an SMBIOS byte and cast it to this type.
	// (could also be detected via is_enum, but that requires TR1/C++0x)
	// (using enum member variables instead of the raw u8 means the debugger
	// will display the enumerators)
	F_ENUM     = 0x04,

	// a collection of bit flags and should be displayed as such.
	F_FLAGS    = 0x08,

	// an SMBIOS Handle (allows nicer display of `unknown' handles).
	F_HANDLE   = 0x10
};

#if MSC_VERSION
# pragma pack(push, 1)
#endif


//-----------------------------------------------------------------------------
// Bios

#define Bios_FIELDS\
	FIELD(0, const char*, vendor, "")\
	FIELD(0, const char*, version, "")\
	FIELD(0, u16, startSegment, "")\
	FIELD(0, const char*, releaseDate, "")\
	FIELD(0, u8, size, " +1*64KB")\
	FIELD(0, u64, characteristics, "")\
	/* omit subsequent fields because we can't handle the variable-length characteristics extension */


//-----------------------------------------------------------------------------
// System

enum SystemWakeUpType
{
	SWT_APM_TIMER = 3,
	SWT_MODEM_RING,
	SWT_LAN_REMOTE,
	SWT_POWER_SWITCH,
	SWT_PCI_PME,
	SWT_AC_POWER_RESTORED
};

#define System_FIELDS\
	FIELD(0, const char*, manufacturer, "")\
	FIELD(0, const char*, productName, "")\
	FIELD(0, const char*, version, "")\
	FIELD(0, const char*, serialNumber, "")\
	FIELD(0, u64, uuid0, "")\
	FIELD(0, u64, uuid1, "")\
	FIELD(F_ENUM, SystemWakeUpType, wakeUpType, "")\
	FIELD(0, const char*, skuNumber, "")\
	FIELD(0, const char*, family, "")


//-----------------------------------------------------------------------------
// Baseboard

enum BaseboardFlags
{
	BB_IS_MOTHERBOARD    = 0x01,
	BB_REQUIRES_DAUGHTER = 0x02,
	BB_IS_REMOVEABLE     = 0x04,
	BB_IS_REPLACEABLE    = 0x08,
	BB_IS_HOT_SWAPPABLE  = 0x10
};

enum BaseboardType
{
	BB_BLADE = 3,
	BB_SWITCH,
	BB_SYSTEM_MANAGEMENT,
	BB_PROCESSOR,
	BB_IO,
	BB_MEMORY,
	BB_DAUGHTER,
	BB_MOTHERBOARD,
	BB_PROCESSOR_MEMORY,
	BB_PROCESSOR_IO,
	BB_INTERCONNECT,
};

#define Baseboard_FIELDS\
	FIELD(0, const char*, manufacturer, "")\
	FIELD(0, const char*, product, "")\
	FIELD(0, const char*, version, "")\
	FIELD(0, const char*, serialNumber, "")\
	FIELD(0, const char*, assetTag, "")\
	FIELD(F_FLAGS, u8, flags, "")\
	FIELD(0, const char*, location, "")\
	FIELD(F_HANDLE, Handle, hChassis, "")\
	FIELD(F_ENUM, BaseboardType, type, "")\
	/* omit subsequent fields because we can't handle the variable-length contained objects */


//-----------------------------------------------------------------------------
// Chassis

enum ChassisType
{
	CT_DESKTOP = 3,
	CT_LOW_PROFILE_DESKTOP,
	CT_PIZZA_BOX,
	CT_MINI_TOWER,
	CT_TOWER,
	CT_PORTABLE,
	CT_LAPTOP,
	CT_NOTEBOOK,
	CT_HANDHELD,
	CT_DOCKING_STATION,
	CT_ALL_IN_ONE,
	CT_SUBNOTEBOOK,
	CT_SPACE_SAVING,
	CT_LUNCHBOX,
	CT_MAIN_SERVER,
	CT_EXPANSION,
	CT_SUB,
	CT_BUS_EXPANSION,
	CT_PERIPHERAL,
	CT_RAID,
	CT_RACK_MOUNT,
	CT_SEALED_CASE,
	CT_MULTI_SYSTEM,
	CT_COMPACT_PCI,
	CT_ADVANCED_TCA,
	CT_BLADE,
	CT_BLADE_ENCLOSURE

};

enum ChassisState
{
	CS_SAFE = 3,
	CS_WARNING,
	CS_CRITICAL,
	CS_NON_RECOVERABLE
};

enum ChassisSecurityStatus
{
	CSS_NONE = 3,
	CSS_EXTERNAL_INTERFACE_LOCKED,
	CSS_EXTERNAL_INTERFACE_ENABLED
};

#define Chassis_FIELDS\
	FIELD(0, const char*, manufacturer, "")\
	FIELD(F_ENUM, ChassisType, type, "")\
	FIELD(0, const char*, version, "")\
	FIELD(0, const char*, serialNumber, "")\
	FIELD(0, const char*, assetTag, "")\
	FIELD(F_ENUM, ChassisState, state, "")\
	FIELD(F_ENUM, ChassisState, powerState, "")\
	FIELD(F_ENUM, ChassisState, thermalState, "")\
	FIELD(F_ENUM, ChassisSecurityStatus, securityStatus, "")\
	FIELD(0, u32, oemDefined, "")\
	FIELD(0, u8, height, "U")\
	FIELD(0, u8, numPowerCords, "U")\
	/* omit subsequent fields because we can't handle the variable-length contained objects */


//-----------------------------------------------------------------------------
// Processor

enum ProcessorType
{
	PT_CENTRAL = 3,
	PT_MATH,
	PT_DSP,
	PT_VIDEO
};

enum ProcessorStatus
{
	PS_UNKNOWN = 0,
	PS_ENABLED,
	PS_USER_DISABLED,
	PS_POST_DISABLED,
	PS_CPU_IDLE,
	PS_OTHER = 7,

	// NB: we mask off 0x40 ("populated") for convenience
};

enum ProcessorUpgrade
{
	PU_DAUGHTER = 3,
	PU_ZIF,
	PU_PIGGYBACK,
	PU_NONE,
	PU_LIF,
	PU_SLOT_1,
	PU_SLOT_2,
	PU_SOCKET_370,
	PU_SLOT_A,
	PU_SLOT_M,
	PU_SOCKET_423,
	PU_SOCKET_A,
	PU_SOCKET_478,
	PU_SOCKET_754,
	PU_SOCKET_940,
	PU_SOCKET_939,
	PU_SOCKET_604,
	PU_SOCKET_771,
	PU_SOCKET_775,
	PU_SOCKET_S1,
	PU_SOCKET_AM2,
	PU_SOCKET_1207,
	PU_SOCKET_1366,
	PU_SOCKET_G34,
	PU_SOCKET_AM3,
	PU_SOCKET_C32,
	PU_SOCKET_1156,
	PU_SOCKET_1567,
	PU_SOCKET_988A,
	PU_SOCKET_1288,
	PU_SOCKET_988B,
	PU_SOCKET_1023,
	PU_SOCKET_1224,
	PU_SOCKET_1155,
	PU_SOCKET_1356,
	PU_SOCKET_2011,
	PU_SOCKET_FS1,
	PU_SOCKET_FS2,
	PU_SOCKET_FM1,
	PU_SOCKET_FM2
};

enum ProcessorVoltage	// bitfield
{
	PV_5   = 1,
	PV_3_3 = 2,
	PV_2_9 = 4
};

enum ProcessorCharacteristics // bitfield
{
	PC_UNKNOWN                 = 0x2,
	PC_64_BIT                  = 0x4,
	PC_MULTI_CORE              = 0x8,	// (indicates cores are present; they might be disabled)
	PC_HARDWARE_THREAD         = 0x10, 
	PC_EXECUTE_PROTECTION      = 0x20,
	PC_ENHANCED_VIRTUALIZATION = 0x40,
	PC_POWER_CONTROL           = 0x80
};

#define Processor_FIELDS\
	FIELD(0, const char*, socket, "")\
	FIELD(F_ENUM, ProcessorType, type, "")\
	FIELD(0, u8, family, "") /* we don't bother providing enumerators for > 200 families */\
	FIELD(0, const char*, manufacturer, "")\
	FIELD(0, u64, id, "")\
	FIELD(0, const char*, version, "")\
	FIELD(F_FLAGS, u8, voltage, "")\
	FIELD(0, u16, externalClockFrequency, " Mhz")\
	FIELD(0, u16, maxFrequency, " Mhz")\
	FIELD(0, u16, bootFrequency, " Mhz")\
	FIELD(F_ENUM, ProcessorStatus, status, "")\
	FIELD(F_ENUM, ProcessorUpgrade, upgrade, "")\
	FIELD(F_HANDLE, Handle, hL1, "")\
	FIELD(F_HANDLE, Handle, hL2, "")\
	FIELD(F_HANDLE, Handle, hL3, "")\
	FIELD(0, const char*, serialNumber, "")\
	FIELD(0, const char*, assetTag, "")\
	FIELD(0, const char*, partNumber, "")\
	FIELD(0, u8, coresPerPackage, "")\
	FIELD(0, u8, enabledCores, "")\
	FIELD(0, u8, logicalPerPackage, "")\
	FIELD(F_FLAGS, u16, characteristics, "")\
	FIELD(0, u16, family2, "")


//-----------------------------------------------------------------------------
// Cache

enum CacheMode
{
	CM_WRITE_THROUGH = 0,
	CM_WRITE_BACK,
	CM_VARIES,
	CM_UNKNOWN
};

enum CacheLocation
{
	CL_INTERNAL = 0,
	CL_EXTERNAL,
	CL_RESERVED,
	CL_UNKNOWN
};

enum CacheConfiguration
{
	CC_SOCKETED = 0x08,
	CC_ENABLED  = 0x80
};

enum CacheFlags
{
	CF_OTHER          = 0x01,
	CF_UNKNOWN        = 0x02,
	CF_NON_BURST      = 0x04,
	CF_BURST          = 0x08,
	CF_PIPELINE_BURST = 0x10,
	CF_SYNCHRONOUS    = 0x20,
	CF_ASYNCHRONOUS   = 0x40
};

enum CacheECC
{
	CE_NONE = 3,
	CE_PARITY,
	CE_SINGLE_BIT,
	CE_MULTIPLE_BIT
};

enum CacheType
{
	CT_INSTRUCTION = 3,
	CT_DATA,
	CT_UNIFIED
};

enum CacheAssociativity
{
	CA_DIRECT_MAPPED = 3,
	CA_2,
	CA_4,
	CA_FULL,
	CA_8,
	CA_16,
	CA_12,
	CA_24,
	CA_32,
	CA_48,
	CA_64,
	CA_20
};

#define Cache_FIELDS\
	FIELD(0, const char*, designation, "")\
	FIELD(F_FLAGS, u16, configuration, "")\
	FIELD(F_INTERNAL, u16, maxSize16, "")\
	FIELD(F_INTERNAL, u16, installedSize16, "")\
	FIELD(F_FLAGS, u16, supportedFlags, "")\
	FIELD(F_FLAGS, u16, currentFlags, "")\
	FIELD(0, u8, speed, " ns")\
	FIELD(F_ENUM, CacheECC, ecc, "")\
	FIELD(F_ENUM, CacheType, type, "")\
	FIELD(F_ENUM, CacheAssociativity, associativity, "")\
	FIELD(F_DERIVED, size_t, level, "") /* 1..8 */\
	FIELD(F_DERIVED|F_ENUM, CacheLocation, location, "")\
	FIELD(F_DERIVED|F_ENUM, CacheMode, mode, "")\
	FIELD(F_DERIVED, size_t, maxSize, " bytes")\
	FIELD(F_DERIVED, size_t, installedSize, " bytes")


//-----------------------------------------------------------------------------
// SystemSlot

enum SystemSlotType
{
	ST_ISA = 3,
	ST_MCA,
	ST_EISA,
	ST_PCI,
	ST_PCMCIA,
	ST_VESA,
	ST_PROPRIETARY,
	ST_PROCESSOR,
	ST_MEMORY_CARD,
	ST_IO_RISER,
	ST_NUBUS,
	ST_PCI_66,
	ST_AGP,
	ST_AGP_2X,
	ST_AGP_4X,
	ST_PCIX,
	ST_AGP_8X,
	// (PC98 omitted)
	ST_PCIE = 0xA5,
	ST_PCIE_X1,
	ST_PCIE_X2,
	ST_PCIE_X4,
	ST_PCIE_X8,
	ST_PCIE_X16,
	ST_PCIE2,
	ST_PCIE2_X1,
	ST_PCIE2_X2,
	ST_PCIE2_X4,
	ST_PCIE2_X8,
	ST_PCIE2_X16,
	ST_PCIE3,
	ST_PCIE3_X1,
	ST_PCIE3_X2,
	ST_PCIE3_X4,
	ST_PCIE3_X8,
	ST_PCIE3_X16
};

enum SystemSlotBusWidth
{
	SBW_8 = 3,
	SBW_16,
	SBW_32,
	SBW_64,
	SBW_128,
	SBW_1X,
	SBW_2X,
	SBW_4X,
	SBW_8X,
	SBW_12X,
	SBW_16X,
	SBW_32X,
};

enum SystemSlotUsage
{
	SU_AVAILABLE = 3,
	SU_IN_USE
};

enum SystemSlotLength
{
	SL_SHORT = 3,
	SL_LONG
};

enum SystemSlotCharacteristics
{
	SC_UNKNOWN   = 1,
	SC_5_VOLT    = 2,
	SC_3_3_VOLT  = 4,
	SC_IS_SHARED = 8
	// unused "PC card" bits omitted
};

enum SystemSlotCharacteristics2
{
	SC2_SUPPORTS_PME      = 1,
	SC2_SUPPORTS_HOT_PLUG = 2,
	SC2_SUPPORTS_SMBUS    = 4
};

#define SystemSlot_FIELDS\
	FIELD(0, const char*, designation, "")\
	FIELD(F_ENUM, SystemSlotType,     type,     "")\
	FIELD(F_ENUM, SystemSlotBusWidth, busWidth, "")\
	FIELD(F_ENUM, SystemSlotUsage,    usage,    "")\
	FIELD(F_ENUM, SystemSlotLength,   length,   "")\
	FIELD(0, u16, id, "")\
	FIELD(F_FLAGS, u8, characteristics, "")\
	FIELD(F_FLAGS, u8, characteristics2, "")\
	FIELD(0, u8, busNumber, "")\
	FIELD(0, u8, deviceAndFunctionNumbers, "")


//-----------------------------------------------------------------------------
// MemoryArray

enum MemoryArrayLocation
{
	ML_MOTHERBOARD = 3,
	ML_ISA_ADDON,
	ML_EISA_ADDON,
	ML_PCI_ADDON,
	ML_MCA_ADDON,
	ML_PCMCIA_ADDON,
	ML_PROPRIETARY_ADDON,
	ML_NUBUS
	// (PC98 omitted)
};

enum MemoryArrayUse
{
	MU_SYSTEM = 3,
	MU_VIDEO,
	MU_FLASH,
	MU_NVRAM,
	MU_CACHE
};

enum MemoryArrayECC
{
	ME_NONE = 3,
	ME_PARITY,
	ME_SINGLE_BIT,
	ME_MULTIPLE_BIT,
	ME_CRC
};

#define MemoryArray_FIELDS\
	FIELD(F_ENUM, MemoryArrayLocation, location, "")\
	FIELD(F_ENUM, MemoryArrayUse, use, "")\
	FIELD(F_ENUM, MemoryArrayECC, ecc, "")\
	FIELD(F_INTERNAL, u32, maxCapacity32, "")\
	FIELD(F_HANDLE, Handle, hError, "")\
	FIELD(0, u16, numDevices, "")\
	FIELD(0, u64, maxCapacity, " bytes")


//-----------------------------------------------------------------------------
// MemoryDevice

enum MemoryDeviceFormFactor
{
	MF_SIMM = 3,
	MF_SIP,
	MF_CHIP,
	MF_DIP,
	MF_ZIP,
	MF_PROPRIETARY_CARD,
	MF_DIMM,
	MF_TSOP,
	MF_ROW_OF_CHIPS,
	MF_RIMM,
	MF_SODIMM,
	MF_SRIMM,
	MF_FBDIMM
};

enum MemoryDeviceType
{
	MT_DRAM = 3,
	MT_EDRAM,
	MT_VRAM,
	MT_SRAM,
	MT_RAM,
	MT_ROM,
	MT_FLASH,
	MT_EEPROM,
	MT_FEPROM,
	MT_EPROM,
	MT_CRRAM,
	MT_3DRAM,
	MT_SDRAM,
	MT_SGRAM,
	MT_RDRAM,
	MT_DDR,
	MT_DDR2,
	MT_DDR2_FBDIMM,
	MT_DDR3 = 0x18,
	MT_FBD2
};

enum MemoryDeviceTypeFlags
{
	MTF_OTHER         = 0x0002,
	MTF_UNKNOWN       = 0x0004,
	MTF_FAST_PAGED    = 0x0008,
	MTF_STATIC_COLUMN = 0x0010,
	MTF_PSEUDO_STATIC = 0x0020,
	MTF_RAMBUS        = 0x0040,
	MTF_SYNCHRONOUS   = 0x0080,
	MTF_CMOS          = 0x0100,
	MTF_EDO           = 0x0200,
	MTF_WINDOW_DRAM   = 0x0400,
	MTF_CACHE_DRAM    = 0x0800,
	MTF_NON_VOLATILE  = 0x1000,
	MTF_BUFFERED      = 0x2000,
	MTF_UNBUFFERED    = 0x4000,
};

#define MemoryDevice_FIELDS\
	FIELD(F_HANDLE, Handle, hMemoryArray, "")\
	FIELD(F_HANDLE, Handle, hError, "")\
	FIELD(0, u16, totalWidth, " bits")\
	FIELD(0, u16, dataWidth, " bits")\
	FIELD(F_INTERNAL, u16, size16, "")\
	FIELD(F_ENUM, MemoryDeviceFormFactor, formFactor, "")\
	FIELD(0, u8, deviceSet, "")\
	FIELD(0, const char*, locator, "")\
	FIELD(0, const char*, bank, "")\
	FIELD(F_ENUM, MemoryDeviceType, type, "")\
	FIELD(F_FLAGS, u16, typeFlags, "")\
	FIELD(0, u16, speed, " MHz")\
	FIELD(0, const char*, manufacturer, "")\
	FIELD(0, const char*, serialNumber, "")\
	FIELD(0, const char*, assetTag, "")\
	FIELD(0, const char*, partNumber, "")\
	FIELD(F_FLAGS, u8, attributes, "")\
	FIELD(F_INTERNAL, u32, size32, "")\
	FIELD(0, u16, configuredSpeed, " MHz")\
	FIELD(F_DERIVED, u64, size, " bytes")


//-----------------------------------------------------------------------------
// MemoryArrayMappedAddress

#define MemoryArrayMappedAddress_FIELDS\
	FIELD(F_INTERNAL, u32, startAddress32, " bits")\
	FIELD(F_INTERNAL, u32, endAddress32, " bits")\
	FIELD(F_HANDLE, Handle, hMemoryArray, "")\
	FIELD(0, u8, partitionWidth, "")\
	FIELD(0, u64, startAddress, "")\
	FIELD(0, u64, endAddress, "")\


//-----------------------------------------------------------------------------
// MemoryDeviceMappedAddress

#define MemoryDeviceMappedAddress_FIELDS\
	FIELD(F_INTERNAL, u32, startAddress32, " bits")\
	FIELD(F_INTERNAL, u32, endAddress32, " bits")\
	FIELD(F_HANDLE, Handle, hMemoryDevice, "")\
	FIELD(F_HANDLE, Handle, hMemoryArrayMappedAddress, "")\
	FIELD(0, u8, partitionRowPosition, "")\
	FIELD(0, u8, interleavePosition, "")\
	FIELD(0, u8, interleavedDataDepth, "")\
	FIELD(0, u64, startAddress, "")\
	FIELD(0, u64, endAddress, "")\


//-----------------------------------------------------------------------------
// SystemBoot

enum SystemBootStatus
{
	SBS_NO_ERROR = 0,
	SBS_NO_BOOTABLE_MEDIA,
	SBS_OS_LOAD_FAILED,
	SBS_HARDWARE_FAILURE_FIRMWARE,
	SBS_HARDWARE_FAILURE_OS,
	SBS_USER_REQUESTED_BOOT,
	SBS_SECURITY_VIOLATION,
	SBS_PREVIOUSLY_REQUESTED_IMAGE,
	SBS_WATCHDOG_EXPIRED
};

#define SystemBoot_FIELDS\
	FIELD(F_INTERNAL, u32, reserved32, "")\
	FIELD(F_INTERNAL, u16, reserved16, "")\
	FIELD(F_HANDLE, Handle, hError, "")\
	FIELD(0, u16, totalWidth, " bits")\


//-----------------------------------------------------------------------------

struct Header
{
	u8 id;
	u8 length;
	Handle handle;
};

// declare each structure
#define FIELD(flags, type, name, units) type name;
#define STRUCTURE(name, id) struct name { Header header; name* next; name##_FIELDS };
STRUCTURES
#undef STRUCTURE
#undef FIELD

// declare a UDT holding pointers (freed at exit) to each structure
struct Structures
{
#define STRUCTURE(name, id) name* name##_;
	STRUCTURES
#undef STRUCTURE
};

#if MSC_VERSION
# pragma pack(pop)
#endif

// define function templates that invoke a Visitor for each of a structure's fields
#define FIELD(flags, type, name, units) visitor(flags, p.name, #name, units);
#define STRUCTURE(name, id) template<class Visitor> void VisitFields(name& p, Visitor& visitor) { name##_FIELDS }
STRUCTURES
#undef STRUCTURE
#undef FIELD

LIB_API const Structures* GetStructures();

}	// namespace SMBIOS

#endif	// #ifndef INCLUDED_SMBIOS
