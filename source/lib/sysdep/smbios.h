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
// 2) define a <name>_FIELDS macro specifying its fields
// 3) (optional) add a specialization of Fixup
#define STRUCTURES\
	STRUCTURE(Bios, 0)\
	STRUCTURE(System, 1)\
	STRUCTURE(Baseboard, 2)\
	STRUCTURE(Chassis, 3)\
	STRUCTURE(Processor, 4)\
	/* MemoryController (5) and MemoryModule (6) are obsolete */\
	STRUCTURE(Cache, 7)\
	STRUCTURE(PortConnector, 8)\
	STRUCTURE(SystemSlot, 9)\
	STRUCTURE(OnBoardDevices, 10)\
	/* OemStrings (11), SystemConfiguration (12), BiosLanguage (13), GroupAssociations (14), SystemEventLog (15) are optional */\
	STRUCTURE(MemoryArray, 16)\
	STRUCTURE(MemoryDevice, 17)\
	/* MemoryError32 (18) is optional */\
	STRUCTURE(MemoryArrayMappedAddress, 19)\
	STRUCTURE(MemoryDeviceMappedAddress, 20)\
	/* PointingDevice (21) is optional */\
	STRUCTURE(PortableBattery, 22)\
	/* SystemReset (23), HardwareSecurity (24), SystemPowerControls (25) are optional */\
	STRUCTURE(VoltageProbe, 26)\
	STRUCTURE(CoolingDevice, 27)\
	STRUCTURE(TemperatureProbe, 28)\
	/* ElectricalCurrentProbe (29), OutOfBandRemoteAccess (30), BootIntegrityServices (31) are optional */\
	STRUCTURE(SystemBoot, 32)
	/* MemoryError64 (33), ManagementDevice (34), ManagementDeviceComponent (35), ManagementDeviceThreshold (36) are optional */
	/* MemoryChannel (37), IpmiDevice (38), SystemPowerSupply (39), Additional (40), OnboardDevices2 (41) are optional */
	/* ManagementControllerHostInterface (42) is optional */

// to introduce another enumeration:
// 1) add its name here
// 2) define a <name>_ENUMERATORS macro specifying its enumerators
#define ENUMERATIONS\
	ENUMERATION(Status)\
	ENUMERATION(SystemWakeUpType)\
	ENUMERATION(BaseboardFlags)\
	ENUMERATION(BaseboardType)\
	ENUMERATION(ChassisType)\
	ENUMERATION(ChassisSecurityStatus)\
	ENUMERATION(ProcessorType)\
	ENUMERATION(ProcessorStatus)\
	ENUMERATION(ProcessorUpgrade)\
	ENUMERATION(ProcessorFlags)


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

	// a collection of bit flags.
	F_FLAGS    = 0x08,

	// an SMBIOS Handle (allows nicer display of `unknown' handles).
	F_HANDLE   = 0x10,

	// a number that should be displayed in hexadecimal form.
	F_HEX      = 0x20,

	F_SIZE     = 0x40
};


#pragma pack(push, 1)

// shared by several structures
#define Status_ENUMERATORS\
	ENUM(STATUS_OK, 3)\
	ENUM(STATUS_NON_CRITICAL, 4)\
	ENUM(STATUS_CRITICAL, 5)\
	ENUM(STATUS_NON_RECOVERABLE, 6)


//-----------------------------------------------------------------------------
// Bios

#define Bios_FIELDS\
	FIELD(0, const char*, vendor, "")\
	FIELD(0, const char*, version, "")\
	FIELD(F_HEX, u16, startSegment, "")\
	FIELD(0, const char*, releaseDate, "")\
	FIELD(F_INTERNAL, u8, encodedSize, "")\
	FIELD(F_HEX, u64, characteristics, "")\
	/* omit subsequent fields because we can't handle the variable-length characteristics extension */\
	FIELD(F_DERIVED|F_SIZE, u64, size, "")


//-----------------------------------------------------------------------------
// System

#define SystemWakeUpType_ENUMERATORS\
	ENUM(SWT_APM_TIMER, 3)\
	ENUM(SWT_MODEM_RING, 4)\
	ENUM(SWT_LAN_REMOTE, 5)\
	ENUM(SWT_POWER_SWITCH, 6)\
	ENUM(SWT_PCI_PME, 7)\
	ENUM(SWT_AC_POWER_RESTORED, 8)

#define System_FIELDS\
	FIELD(0, const char*, manufacturer, "")\
	FIELD(0, const char*, productName, "")\
	FIELD(0, const char*, version, "")\
	FIELD(0, const char*, serialNumber, "")\
	FIELD(F_HEX, u64, uuid0, "")\
	FIELD(F_HEX, u64, uuid1, "")\
	FIELD(F_ENUM, SystemWakeUpType, wakeUpType, "")\
	FIELD(0, const char*, skuNumber, "")\
	FIELD(0, const char*, family, "")


//-----------------------------------------------------------------------------
// Baseboard

#define BaseboardFlags_ENUMERATORS\
	ENUM(BF_IS_MOTHERBOARD, 0x01)\
	ENUM(BF_REQUIRES_DAUGHTER, 0x02)\
	ENUM(BF_IS_REMOVEABLE, 0x04)\
	ENUM(BF_IS_REPLACEABLE, 0x08)\
	ENUM(BF_IS_HOT_SWAPPABLE, 0x10)

#define BaseboardType_ENUMERATORS\
	ENUM(BT_BLADE, 3)\
	ENUM(BT_SWITCH, 4)\
	ENUM(BT_SYSTEM_MANAGEMENT, 5)\
	ENUM(BT_PROCESSOR, 6)\
	ENUM(BT_IO, 7)\
	ENUM(BT_MEMORY, 8)\
	ENUM(BT_DAUGHTER, 9)\
	ENUM(BT_MOTHERBOARD, 10)\
	ENUM(BT_PROCESSOR_MEMORY, 11)\
	ENUM(BT_PROCESSOR_IO, 12)\
	ENUM(BT_INTERCONNECT, 13)

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

#define ChassisType_ENUMERATORS\
	ENUM(CT_DESKTOP, 3)\
	ENUM(CT_LOW_PROFILE_DESKTOP, 4)\
	ENUM(CT_PIZZA_BOX, 5)\
	ENUM(CT_MINI_TOWER, 6)\
	ENUM(CT_TOWER, 7)\
	ENUM(CT_PORTABLE, 8)\
	ENUM(CT_LAPTOP, 9)\
	ENUM(CT_NOTEBOOK, 10)\
	ENUM(CT_HANDHELD, 11)\
	ENUM(CT_DOCKING_STATION, 12)\
	ENUM(CT_ALL_IN_ONE, 13)\
	ENUM(CT_SUBNOTEBOOK, 14)\
	ENUM(CT_SPACE_SAVING, 15)\
	ENUM(CT_LUNCHBOX, 16)\
	ENUM(CT_MAIN_SERVER, 17)\
	ENUM(CT_EXPANSION, 18)\
	ENUM(CT_SUB, 19)\
	ENUM(CT_BUS_EXPANSION, 20)\
	ENUM(CT_PERIPHERAL, 21)\
	ENUM(CT_RAID, 22)\
	ENUM(CT_RACK_MOUNT, 23)\
	ENUM(CT_SEALED_CASE, 24)\
	ENUM(CT_MULTI_SYSTEM, 25)\
	ENUM(CT_COMPACT_PCI, 26)\
	ENUM(CT_ADVANCED_TCA, 27)\
	ENUM(CT_BLADE, 28)\
	ENUM(CT_BLADE_ENCLOSURE, 29)

#define ChassisSecurityStatus_ENUMERATORS\
	ENUM(CSS_NONE, 3)\
	ENUM(CSS_EXTERNAL_INTERFACE_LOCKED, 4)\
	ENUM(CSS_EXTERNAL_INTERFACE_ENABLED, 5)

#define Chassis_FIELDS\
	FIELD(0, const char*, manufacturer, "")\
	FIELD(F_ENUM, ChassisType, type, "")\
	FIELD(0, const char*, version, "")\
	FIELD(0, const char*, serialNumber, "")\
	FIELD(0, const char*, assetTag, "")\
	FIELD(F_ENUM, Status, state, "")\
	FIELD(F_ENUM, Status, powerState, "")\
	FIELD(F_ENUM, Status, thermalState, "")\
	FIELD(F_ENUM, ChassisSecurityStatus, securityStatus, "")\
	FIELD(0, u32, oemDefined, "")\
	FIELD(0, u8, height, "U")\
	FIELD(0, u8, numPowerCords, "")\
	/* omit subsequent fields because we can't handle the variable-length contained objects */


//-----------------------------------------------------------------------------
// Processor

#define ProcessorType_ENUMERATORS\
	ENUM(PT_CPU, 3)\
	ENUM(PT_MATH, 4)\
	ENUM(PT_DSP, 5)\
	ENUM(PT_GPU, 6)

#define ProcessorStatus_ENUMERATORS\
	ENUM(PS_UNKNOWN, 0)\
	ENUM(PS_ENABLED, 1)\
	ENUM(PS_USER_DISABLED, 2)\
	ENUM(PS_POST_DISABLED, 3)\
	ENUM(PS_CPU_IDLE, 4)\
	ENUM(PS_OTHER, 7)

#define ProcessorUpgrade_ENUMERATORS\
	ENUM(PU_DAUGHTER, 3)\
	ENUM(PU_ZIF, 4)\
	ENUM(PU_PIGGYBACK, 5)\
	ENUM(PU_NONE, 6)\
	ENUM(PU_LIF, 7)\
	ENUM(PU_SLOT_1, 8)\
	ENUM(PU_SLOT_2, 9)\
	ENUM(PU_SOCKET_370, 10)\
	ENUM(PU_SLOT_A, 11)\
	ENUM(PU_SLOT_M, 12)\
	ENUM(PU_SOCKET_423, 13)\
	ENUM(PU_SOCKET_A, 14)\
	ENUM(PU_SOCKET_478, 15)\
	ENUM(PU_SOCKET_754, 16)\
	ENUM(PU_SOCKET_940, 17)\
	ENUM(PU_SOCKET_939, 18)\
	ENUM(PU_SOCKET_604, 19)\
	ENUM(PU_SOCKET_771, 20)\
	ENUM(PU_SOCKET_775, 21)\
	ENUM(PU_SOCKET_S1, 22)\
	ENUM(PU_SOCKET_AM2, 23)\
	ENUM(PU_SOCKET_1207, 24)\
	ENUM(PU_SOCKET_1366, 25)\
	ENUM(PU_SOCKET_G34, 26)\
	ENUM(PU_SOCKET_AM3, 27)\
	ENUM(PU_SOCKET_C32, 28)\
	ENUM(PU_SOCKET_1156, 29)\
	ENUM(PU_SOCKET_1567, 30)\
	ENUM(PU_SOCKET_988A, 31)\
	ENUM(PU_SOCKET_1288, 32)\
	ENUM(PU_SOCKET_988B, 33)\
	ENUM(PU_SOCKET_1023, 34)\
	ENUM(PU_SOCKET_1224, 35)\
	ENUM(PU_SOCKET_1155, 36)\
	ENUM(PU_SOCKET_1356, 37)\
	ENUM(PU_SOCKET_2011, 38)\
	ENUM(PU_SOCKET_FS1, 39)\
	ENUM(PU_SOCKET_FS2, 40)\
	ENUM(PU_SOCKET_FM1, 41)\
	ENUM(PU_SOCKET_FM2, 42)

#define ProcessorFlags_ENUMERATORS\
	ENUM(PF_UNKNOWN, 0x2)\
	ENUM(PF_64_BIT, 0x4)\
	ENUM(PF_MULTI_CORE, 0x8)/* indicates cores are present, but they might be disabled*/\
	ENUM(PF_HARDWARE_THREAD, 0x10)\
	ENUM(PF_EXECUTE_PROTECTION, 0x20)\
	ENUM(PF_ENHANCED_VIRTUALIZATION, 0x40)\
	ENUM(PF_POWER_CONTROL, 0x80)

#define Processor_FIELDS\
	FIELD(0, const char*, socket, "")\
	FIELD(F_ENUM, ProcessorType, type, "")\
	FIELD(0, u8, family, "") /* we don't bother providing enumerators for > 200 families */\
	FIELD(0, const char*, manufacturer, "")\
	FIELD(F_HEX, u64, id, "")\
	FIELD(0, const char*, version, "")\
	FIELD(0, u8, voltage, " dV")\
	FIELD(0, u16, externalClockFrequency, " MHz")\
	FIELD(0, u16, maxFrequency, " MHz")\
	FIELD(0, u16, bootFrequency, " MHz")\
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
	FIELD(0, u16, family2, "")\
	FIELD(F_DERIVED, u8, isPopulated, "")


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
	FIELD(F_DERIVED|F_SIZE, size_t, maxSize, "")\
	FIELD(F_DERIVED|F_SIZE, size_t, installedSize, "")


//-----------------------------------------------------------------------------
// PortConnector

enum PortConnectorType
{
	PCT_OTHER = 0xFF,
	PCT_NONE = 0,
	PCT_CENTRONICS,
	PCT_MINI_CENTRONICS,
	PCT_PROPRIETARY,
	PCT_DB25_MALE,
	PCT_DB25_PIN_FEMALE,
	PCT_DB15_PIN_MALE,
	PCT_DB15_PIN_FEMALE,
	PCT_DB9_PIN_MALE,
	PCT_DB9_PIN_FEMALE,
	PCT_RJ11,
	PCT_RJ45,
	PCT_MINI_SCSI,
	PCT_MINI_DIN,
	PCT_MICRO_DIN,
	PCT_PS2,
	PCT_INFRARED,
	PCT_HP_HIL,
	PCT_ACCESS_BUS_USB,
	PCT_SSA_SCSI,
	PCT_DIN8_MALE,
	PCT_DIN8_FEMALE,
	PCT_ON_BOARD_IDE,
	PCT_ON_BOARD_FLOPPY,
	PCT_DUAL_INLINE_9,
	PCT_DUAL_INLINE_25,
	PCT_DUAL_INLINE_50 = 0x1A,
	PCT_DUAL_INLINE_68,
	PCT_ON_BOARD_SOUND_INPUT_FROM_CDROM,
	PCT_MINI_CENTRONICS_14,
	PCT_MINI_CENTRONICS_26,
	PCT_HEADPHONES,
	PCT_BNC,
	PCT_1394,
	PCT_SAS_SATA
	/* PC-98 omitted */
};

enum PortType
{
	PT_OTHER = 0xFF,
	PT_NONE = 0,
	PT_PARALLEL_XT_AT,
	PT_PARALLEL_PS2,
	PT_PARALLEL_ECP,
	PT_PARALLEL_EPP,
	PT_PARALLEL_ECP_EPP,
	PT_SERIAL_XT_AT,
	PT_SERIAL_16450,
	PT_SERIAL_16550,
	PT_SERIAL_16550A,
	PT_SCSI,
	PT_MIDI,
	PT_JOYSTICK,
	PT_KEYBOARD,
	PT_MOUSE,
	PT_SSA_SCSI,
	PT_USB,
	PT_1394,
	PT_PCMCIA_TYPE_I,
	PT_PCMCIA_TYPE_II,
	PT_PCMCIA_TYPE_III,
	PT_CARDBUS,
	PT_ACCESS_BUS,
	PT_SCSI_II,
	PT_SCSI_WIDE,
	PT_PC98,
	PT_PC98_HIRESO,
	PT_PC_H98,
	PT_VIDEO,
	PT_AUDIO,
	PT_MODEM,
	PT_NETWORK,
	PT_SATA,
	PT_SAS,
	PT_8251_COMPATIBLE = 0xA0,
	PT_8251_FIFO_COMPATIBLE
};

#define PortConnector_FIELDS\
	FIELD(0, const char*, internalDesignator, "")\
	FIELD(F_ENUM, PortConnectorType, internalConnectorType, "")\
	FIELD(0, const char*, externalDesignator, "")\
	FIELD(F_ENUM, PortConnectorType, externalConnectorType, "")\
	FIELD(F_ENUM, PortType, portType, "")


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
// OnBoardDevices

enum OnBoardDeviceType
{
	OBDT_OTHER = 1,
	OBDT_VIDEO = 3,
	OBDT_SCSI_CONTROLLER,
	OBDT_ETHERNET,
	OBDT_TOKEN_RING,
	OBDT_SOUND,
	OBDT_PATA_CONTROLLER,
	OBDT_SATA_CONTROLLER,
	OBDT_SAS_CONTROLLER
};

#define OnBoardDevices_FIELDS\
	FIELD(F_ENUM, OnBoardDeviceType, type, "")\
	FIELD(0, const char*, description, "")\
	FIELD(F_DERIVED, u8, isEnabled, "")\
	/* NB: this structure could contain any number of type/description pairs, but Dell BIOS only provides 1 */


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
	FIELD(F_SIZE, u64, maxCapacity, "")


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
	FIELD(F_DERIVED|F_SIZE, u64, size, "")


//-----------------------------------------------------------------------------
// MemoryArrayMappedAddress

#define MemoryArrayMappedAddress_FIELDS\
	FIELD(F_INTERNAL, u32, startAddress32, " bits")\
	FIELD(F_INTERNAL, u32, endAddress32, " bits")\
	FIELD(F_HANDLE, Handle, hMemoryArray, "")\
	FIELD(0, u8, partitionWidth, "")\
	FIELD(F_HEX, u64, startAddress, "")\
	FIELD(F_HEX, u64, endAddress, "")


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
	FIELD(F_HEX, u64, startAddress, "")\
	FIELD(F_HEX, u64, endAddress, "")


//----------------------------------------------------------------------------
// PortableBattery

enum PortableBatteryChemistry
{
	PBC_LEAD_ACID = 3,
	PBC_NICKEL_CADMIUM,
	PBC_NICKEL_METAL_HYDRIDE,
	PBC_LITHIUM_ION,
	PBC_ZINC_AIR,
	PBC_LITHIUM_POLYMER
};

#define PortableBattery_FIELDS\
	FIELD(0, const char*, location, "")\
	FIELD(0, const char*, manufacturer, "")\
	FIELD(0, const char*, date, "")\
	FIELD(0, const char*, serialNumber, "")\
	FIELD(0, const char*, deviceName, "")\
	FIELD(F_ENUM, PortableBatteryChemistry, chemistry, "")\
	FIELD(0, u16, capacity, " mWh")\
	FIELD(0, u16, voltage, " mV")\
	FIELD(0, const char*, sbdsVersion, "")\
	FIELD(0, u8, maxError, "%")\
	FIELD(0, u16, sbdsSerialNumber, "")\
	FIELD(0, u16, sbdsDate, "")\
	FIELD(0, const char*, sbdsChemistry, "")\
	FIELD(0, u8, capacityMultiplier, "")\
	FIELD(0, u32, oemSpecific, "")


//----------------------------------------------------------------------------
// VoltageProbe

enum VoltageProbeLocation
{
	VPL_PROCESSOR = 3,
	VPL_DISK,
	VPL_PERIPHERAL_BAY,
	VPL_SYSTEM_MANAGEMENT_MODULE,
	VPL_MOTHERBOARD,
	VPL_MEMORY_MODULE,
	VPL_PROCESSOR_MODULE,
	VPL_POWER_UNIT,
	VPL_ADD_IN_CARD
};

#define VoltageProbe_FIELDS\
	FIELD(0, const char*, description, "")\
	FIELD(F_INTERNAL, u8, locationAndStatus, "")\
	FIELD(0, u16, maxValue, " mV")\
	FIELD(0, u16, minValue, " mV")\
	FIELD(0, u16, resolution, " x 0.1 mV")\
	FIELD(0, u16, tolerance, " mV")\
	FIELD(0, u16, accuracy, " x 100 ppm")\
	FIELD(0, u32, oemDefined, "")\
	FIELD(0, u16, nominalValue, " mv")\
	FIELD(F_DERIVED, VoltageProbeLocation, location, "")\
	FIELD(F_DERIVED, Status, status, "")


//----------------------------------------------------------------------------
// CoolingDevice

enum CoolingDeviceType
{
	CDT_FAN = 3,
	CDT_CENTRIFUGAL_BLOWER,
	CDT_CHIP_FAN,
	CDT_CABINET_FAN,
	CDT_POWER_SUPPLY_FAN,
	CDT_HEAT_PIPE,
	CDT_INTEGRATED_REFRIGERATION,
	CDT_ACTIVE_COOLING = 0x10,
	CDT_PASSIVE_COOLING
};

#define CoolingDevice_FIELDS\
	FIELD(F_HANDLE, Handle, hTemperatureProbe, "")\
	FIELD(F_INTERNAL, u8, typeAndStatus, "")\
	FIELD(0, u8, group, "")\
	FIELD(0, u32, oemDefined, "")\
	FIELD(0, u16, nominalSpeed, " rpm")\
	FIELD(0, const char*, description, "")\
	FIELD(F_DERIVED, CoolingDeviceType, type, "")\
	FIELD(F_DERIVED, Status, status, "")


//----------------------------------------------------------------------------
// TemperatureProbe

enum TemperatureProbeLocation
{
	TPL_PROCESSOR = 3,
	TPL_DISK,
	TPL_PERIPHERAL_BAY,
	TPL_SYSTEM_MANAGEMENT_MODULE,
	TPL_MOTHERBOARD,
	TPL_MEMORY_MODULE,
	TPL_PROCESSOR_MODULE,
	TPL_POWER_UNIT,
	TPL_ADD_IN_CARD,
	TPL_FRONT_PANEL_BOARD,
	TPL_BACK_PANEL_BOARD,
	TPL_POWER_SYSTEM_BOARD,
	TPL_DRIVE_BACK_PLANE
};

#define TemperatureProbe_FIELDS\
	FIELD(0, const char*, description, "")\
	FIELD(F_INTERNAL, u8, locationAndStatus, "")\
	FIELD(0, i16, maxValue, " dDegC")\
	FIELD(0, i16, minValue, " dDegC")\
	FIELD(0, u16, resolution, " mDegC")\
	FIELD(0, u16, tolerance, " dDegC")\
	FIELD(0, u16, accuracy, " x 100 ppm")\
	FIELD(0, u32, oemDefined, "")\
	FIELD(0, i16, nominalValue, " dDegC")\
	FIELD(F_DERIVED, TemperatureProbeLocation, location, "")\
	FIELD(F_DERIVED, Status, status, "")


//----------------------------------------------------------------------------
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
	FIELD(0, u16, totalWidth, " bits")


//-----------------------------------------------------------------------------

typedef u16 Handle;

struct Header
{
	u8 id;
	u8 length;
	Handle handle;
};

// define each enumeration
#define ENUM(enumerator, value) enumerator = value,
#define ENUMERATION(name) enum name { name##_ENUMERATORS };
ENUMERATIONS
#undef ENUMERATION
#undef ENUM

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

#pragma pack(pop)

LIB_API const Structures* GetStructures();
LIB_API std::string StringizeStructures(const Structures*);

}	// namespace SMBIOS

#endif	// #ifndef INCLUDED_SMBIOS
