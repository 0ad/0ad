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

#ifndef INCLUDED_SMBIOS
#define INCLUDED_SMBIOS

namespace SMBIOS {

// to introduce another enumeration:
// 1) add its name and underlying type here
// 2) define a <name>_ENUMERATORS macro specifying its enumerators
//    (prefer lower case to avoid conflicts with macros)
#define ENUMERATIONS\
	ENUMERATION(State, u8)\
	ENUMERATION(ECC, u8)\
	ENUMERATION(BiosFlags, u32)\
	ENUMERATION(BiosFlags1, u8)\
	ENUMERATION(BiosFlags2, u8)\
	ENUMERATION(SystemWakeUpType, u8)\
	ENUMERATION(BaseboardFlags, u8)\
	ENUMERATION(BaseboardType, u8)\
	ENUMERATION(ChassisType, u8)\
	ENUMERATION(ChassisSecurityStatus, u8)\
	ENUMERATION(ProcessorType, u8)\
	ENUMERATION(ProcessorStatus, u8)\
	ENUMERATION(ProcessorUpgrade, u8)\
	ENUMERATION(ProcessorFlags, u16)\
	ENUMERATION(CacheMode, u8)\
	ENUMERATION(CacheLocation, u8)\
	ENUMERATION(CacheConfigurationFlags, u16)\
	ENUMERATION(CacheFlags, u16)\
	ENUMERATION(CacheType, u8)\
	ENUMERATION(CacheAssociativity, u8)\
	ENUMERATION(PortConnectorType, u8)\
	ENUMERATION(PortType, u8)\
	ENUMERATION(SystemSlotType, u8)\
	ENUMERATION(SystemSlotBusWidth, u8)\
	ENUMERATION(SystemSlotUsage, u8)\
	ENUMERATION(SystemSlotLength, u8)\
	ENUMERATION(SystemSlotFlags1, u8)\
	ENUMERATION(SystemSlotFlags2, u8)\
	ENUMERATION(OnBoardDeviceType, u8)\
	ENUMERATION(MemoryArrayLocation, u8)\
	ENUMERATION(MemoryArrayUse, u8)\
	ENUMERATION(MemoryDeviceFormFactor, u8)\
	ENUMERATION(MemoryDeviceType, u8)\
	ENUMERATION(MemoryDeviceTypeFlags, u16)\
	ENUMERATION(PortableBatteryChemistry, u8)\
	ENUMERATION(VoltageProbeLocation, u8)\
	ENUMERATION(CoolingDeviceType, u8)\
	ENUMERATION(TemperatureProbeLocation, u8)\
	ENUMERATION(SystemBootStatus, u8)\
	ENUMERATION(ManagementDeviceType, u8)\
	ENUMERATION(ManagementDeviceAddressType, u8)\
	ENUMERATION(SystemPowerSupplyCharacteristics, u16)\
	ENUMERATION(SystemPowerSupplyType, u8)\
	ENUMERATION(SystemPowerSupplyInputSwitching, u8)

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
	STRUCTURE(SystemBoot, 32)\
	STRUCTURE(ManagementDevice, 34)\
	STRUCTURE(ManagementDeviceComponent, 35)\
	STRUCTURE(ManagementDeviceThreshold, 36)\
	STRUCTURE(SystemPowerSupply, 39)\
	STRUCTURE(OnboardDevices2, 41)\
	/* MemoryError64 (33), MemoryChannel (37), IpmiDevice (38) are optional */
	/* Additional (40), ManagementControllerHostInterface (42) are optional */


//-----------------------------------------------------------------------------
// declarations required for the fields

// indicates a field (:= member of a structure) is:
enum FieldFlags
{
	// computed via other fields (i.e. should not be copied from the SMBIOS data).
	F_DERIVED  = 1,

	// not intended for display / use by applications (usually because
	// it is superseded by another - possibly derived - field).
	F_INTERNAL = 2,

	// a number that should be displayed in hexadecimal form.
	F_HEX      = 4
};


// (wrapper classes allow special handling of certain fields via
// template specialization and function overloads)

// size [bytes] - displayed with auto-range
template<typename T>
struct Size
{
	Size(): value(0) {}
	Size(T value): value(value) {}
	T value;
	operator T() const { return value; }
};

// SMBIOS structure handle - only displayed if meaningful
struct Handle
{
	Handle(): value(0) {}
	Handle(u16 value): value(value) {}
	u16 value;
};


#define State_ENUMERATORS\
	ENUM(other, 1)\
	ENUM(unknown, 2)\
	ENUM(ok, 3)\
	ENUM(noncritical, 4)\
	ENUM(critical, 5)\
	ENUM(nonrecoverable, 6)

#define ECC_ENUMERATORS\
	ENUM(other, 1)\
	ENUM(unknown, 2)\
	ENUM(none, 3)\
	ENUM(parity, 4)\
	ENUM(single_bit, 5)\
	ENUM(multiple_bit, 6)\
	ENUM(crc, 7)


#pragma pack(push, 1)


//-----------------------------------------------------------------------------
// Bios

#define BiosFlags_ENUMERATORS\
	ENUM(isa, 0x10)\
	ENUM(mca, 0x20)\
	ENUM(eisa, 0x40)\
	ENUM(pci, 0x80)\
	ENUM(pcmcia, 0x100)\
	ENUM(plug_and_play, 0x200)\
	ENUM(apm, 0x400)\
	ENUM(upgradable, 0x800)\
	ENUM(shadowing, 0x1000)\
	ENUM(vl_vesa, 0x2000)\
	ENUM(escd, 0x4000)\
	ENUM(boot_cd, 0x8000)\
	ENUM(selectable_boot, 0x10000)\
	ENUM(socketed_rom, 0x20000)\
	ENUM(boot_pcmcia, 0x40000)\
	ENUM(edd, 0x80000)\
	ENUM(int13a, 0x100000)\
	ENUM(int13b, 0x200000)\
	ENUM(int13c, 0x400000)\
	ENUM(int13d, 0x800000)\
	ENUM(int13e, 0x1000000)\
	ENUM(int13f, 0x2000000)\
	ENUM(int5, 0x4000000)\
	ENUM(int9, 0x8000000)\
	ENUM(int14, 0x10000000)\
	ENUM(int17, 0x20000000)\
	ENUM(int10, 0x40000000)\
	ENUM(pc_98, 0x80000000)

#define BiosFlags1_ENUMERATORS\
	ENUM(acpi, 0x01)\
	ENUM(usb_legacy, 0x02)\
	ENUM(agp, 0x04)\
	ENUM(boot_i2o, 0x08)\
	ENUM(boot_ls_120, 0x10)\
	ENUM(boot_zip_drive, 0x20)\
	ENUM(boot_1394, 0x40)\
	ENUM(smart_battery, 0x80)

#define BiosFlags2_ENUMERATORS\
	ENUM(bios_boot, 0x01)\
	ENUM(function_key_boot, 0x02)\
	ENUM(targeted_content_distribution, 0x04)\
	ENUM(uefi, 0x08)\
	ENUM(virtual_machine, 0x10)

#define Bios_FIELDS\
	FIELD(0, const char*, vendor, "")\
	FIELD(0, const char*, version, "")\
	FIELD(F_HEX, u16, startSegment, "")\
	FIELD(0, const char*, releaseDate, "")\
	FIELD(F_INTERNAL, u8, encodedSize, "")\
	FIELD(0, BiosFlags, flags, "")\
	FIELD(F_HEX, u32, vendorFlags, "")\
	FIELD(0, BiosFlags1, flags1, "")\
	FIELD(0, BiosFlags2, flags2, "")\
	FIELD(F_DERIVED, Size<size_t>, size, "")


//-----------------------------------------------------------------------------
// System

#define SystemWakeUpType_ENUMERATORS\
	ENUM(other, 1)\
	ENUM(unknown, 2)\
	ENUM(apm_timer, 3)\
	ENUM(modem_ring, 4)\
	ENUM(lan_remote, 5)\
	ENUM(power_switch, 6)\
	ENUM(pci_pme, 7)\
	ENUM(ac_power_restored, 8)

#define System_FIELDS\
	FIELD(0, const char*, manufacturer, "")\
	FIELD(0, const char*, productName, "")\
	FIELD(0, const char*, version, "")\
	FIELD(0, const char*, serialNumber, "")\
	FIELD(F_HEX, u64, uuid0, "")\
	FIELD(F_HEX, u64, uuid1, "")\
	FIELD(0, SystemWakeUpType, wakeUpType, "")\
	FIELD(0, const char*, skuNumber, "")\
	FIELD(0, const char*, m_Family, "")


//-----------------------------------------------------------------------------
// Baseboard

#define BaseboardFlags_ENUMERATORS\
	ENUM(motherboard, 0x01)\
	ENUM(requires_add_in, 0x02)\
	ENUM(removeable, 0x04)\
	ENUM(replaceable, 0x08)\
	ENUM(hot_swappable, 0x10)

#define BaseboardType_ENUMERATORS\
	ENUM(other, 1)\
	ENUM(unknown, 2)\
	ENUM(blade, 3)\
	ENUM(connectivity_switch, 4)\
	ENUM(system_management, 5)\
	ENUM(processor, 6)\
	ENUM(io, 7)\
	ENUM(memory, 8)\
	ENUM(daughter, 9)\
	ENUM(motherboard, 10)\
	ENUM(processor_memory, 11)\
	ENUM(processor_io, 12)\
	ENUM(interconnect, 13)

#define Baseboard_FIELDS\
	FIELD(0, const char*, manufacturer, "")\
	FIELD(0, const char*, product, "")\
	FIELD(0, const char*, version, "")\
	FIELD(0, const char*, serialNumber, "")\
	FIELD(0, const char*, assetTag, "")\
	FIELD(0, BaseboardFlags, flags, "")\
	FIELD(0, const char*, location, "")\
	FIELD(0, Handle, hChassis, "")\
	FIELD(0, BaseboardType, type, "")\
	/* omit subsequent fields because we can't handle the variable-length contained objects */


//-----------------------------------------------------------------------------
// Chassis

#define ChassisType_ENUMERATORS\
	ENUM(other, 1)\
	ENUM(unknown, 2)\
	ENUM(desktop, 3)\
	ENUM(low_profile_desktop, 4)\
	ENUM(pizza_box, 5)\
	ENUM(mini_tower, 6)\
	ENUM(tower, 7)\
	ENUM(portable, 8)\
	ENUM(laptop, 9)\
	ENUM(notebook, 10)\
	ENUM(handheld, 11)\
	ENUM(docking_station, 12)\
	ENUM(all_in_one, 13)\
	ENUM(subnotebook, 14)\
	ENUM(space_saving, 15)\
	ENUM(lunchbox, 16)\
	ENUM(main_server, 17)\
	ENUM(expansion, 18)\
	ENUM(sub, 19)\
	ENUM(bus_expansion, 20)\
	ENUM(peripheral, 21)\
	ENUM(raid, 22)\
	ENUM(rack_mount, 23)\
	ENUM(sealed_case, 24)\
	ENUM(multi_system, 25)\
	ENUM(compact_pci, 26)\
	ENUM(advanced_tca, 27)\
	ENUM(blade, 28)\
	ENUM(blade_enclosure, 29)

#define ChassisSecurityStatus_ENUMERATORS\
	ENUM(other, 1)\
	ENUM(unknown, 2)\
	ENUM(none, 3)\
	ENUM(external_interface_locked, 4)\
	ENUM(external_interface_enabled, 5)

#define Chassis_FIELDS\
	FIELD(0, const char*, manufacturer, "")\
	FIELD(0, ChassisType, type, "")\
	FIELD(0, const char*, version, "")\
	FIELD(0, const char*, serialNumber, "")\
	FIELD(0, const char*, assetTag, "")\
	FIELD(0, State, state, "")\
	FIELD(0, State, powerState, "")\
	FIELD(0, State, thermalState, "")\
	FIELD(0, ChassisSecurityStatus, securityStatus, "")\
	FIELD(0, u32, oemDefined, "")\
	FIELD(0, u8, height, "U")\
	FIELD(0, u8, numPowerCords, "")\
	/* omit subsequent fields because we can't handle the variable-length contained objects */


//-----------------------------------------------------------------------------
// Processor

#define ProcessorType_ENUMERATORS\
	ENUM(other, 1)\
	ENUM(unknown, 2)\
	ENUM(CPU, 3)\
	ENUM(FPU, 4)\
	ENUM(DSP, 5)\
	ENUM(GPU, 6)

#define ProcessorStatus_ENUMERATORS\
	ENUM(unknown, 0)\
	ENUM(other, 7)\
	ENUM(enabled, 1)\
	ENUM(user_disabled, 2)\
	ENUM(post_disabled, 3)\
	ENUM(idle, 4)

#define ProcessorUpgrade_ENUMERATORS\
	ENUM(other, 1)\
	ENUM(unknown, 2)\
	ENUM(daughter, 3)\
	ENUM(zif, 4)\
	ENUM(piggyback, 5)\
	ENUM(none, 6)\
	ENUM(lif, 7)\
	ENUM(slot_1, 8)\
	ENUM(slot_2, 9)\
	ENUM(socket_370, 10)\
	ENUM(slot_a, 11)\
	ENUM(slot_m, 12)\
	ENUM(socket_423, 13)\
	ENUM(socket_a, 14)\
	ENUM(socket_478, 15)\
	ENUM(socket_754, 16)\
	ENUM(socket_940, 17)\
	ENUM(socket_939, 18)\
	ENUM(socket_604, 19)\
	ENUM(socket_771, 20)\
	ENUM(socket_775, 21)\
	ENUM(socket_s1, 22)\
	ENUM(socket_am2, 23)\
	ENUM(socket_1207, 24)\
	ENUM(socket_1366, 25)\
	ENUM(socket_g34, 26)\
	ENUM(socket_am3, 27)\
	ENUM(socket_c32, 28)\
	ENUM(socket_1156, 29)\
	ENUM(socket_1567, 30)\
	ENUM(socket_988a, 31)\
	ENUM(socket_1288, 32)\
	ENUM(socket_988b, 33)\
	ENUM(socket_1023, 34)\
	ENUM(socket_1224, 35)\
	ENUM(socket_1155, 36)\
	ENUM(socket_1356, 37)\
	ENUM(socket_2011, 38)\
	ENUM(socket_fs1, 39)\
	ENUM(socket_fs2, 40)\
	ENUM(socket_fm1, 41)\
	ENUM(socket_fm2, 42)

#define ProcessorFlags_ENUMERATORS\
	ENUM(unknown, 0x2)\
	ENUM(x64, 0x4)\
	ENUM(multi_core, 0x8)/* indicates cores are present, but they might be disabled*/\
	ENUM(ht, 0x10)\
	ENUM(execute_protection, 0x20)\
	ENUM(enhanced_virtualization, 0x40)\
	ENUM(power_control, 0x80)

#define Processor_FIELDS\
	FIELD(0, const char*, socket, "")\
	FIELD(0, ProcessorType, type, "")\
	FIELD(0, u8, m_Family, "") /* we don't bother providing enumerators for > 200 families */\
	FIELD(0, const char*, manufacturer, "")\
	FIELD(F_HEX, u64, id, "")\
	FIELD(0, const char*, version, "")\
	FIELD(0, u8, voltage, " dV")\
	FIELD(0, u16, externalClockFrequency, " MHz")\
	FIELD(0, u16, maxFrequency, " MHz")\
	FIELD(0, u16, bootFrequency, " MHz")\
	FIELD(0, ProcessorStatus, status, "")\
	FIELD(0, ProcessorUpgrade, upgrade, "")\
	FIELD(0, Handle, hL1, "")\
	FIELD(0, Handle, hL2, "")\
	FIELD(0, Handle, hL3, "")\
	FIELD(0, const char*, serialNumber, "")\
	FIELD(0, const char*, assetTag, "")\
	FIELD(0, const char*, partNumber, "")\
	FIELD(0, u8, coresPerPackage, "")\
	FIELD(0, u8, enabledCores, "")\
	FIELD(0, u8, logicalPerPackage, "")\
	FIELD(0, ProcessorFlags, flags, "")\
	FIELD(0, u16, family2, "")\
	FIELD(F_DERIVED, bool, populated, "")


//-----------------------------------------------------------------------------
// Cache

#define CacheMode_ENUMERATORS\
	ENUM(write_through, 0)\
	ENUM(write_back, 1)\
	ENUM(varies, 2)\
	ENUM(unknown, 3)

#define CacheLocation_ENUMERATORS\
	ENUM(internal, 0)\
	ENUM(external, 1)\
	ENUM(reserved, 2)\
	ENUM(unknown, 3)

#define CacheConfigurationFlags_ENUMERATORS\
	ENUM(socketed, 0x08)\
	ENUM(enabled, 0x80)

#define CacheFlags_ENUMERATORS\
	ENUM(other, 0x01)\
	ENUM(unknown, 0x02)\
	ENUM(non_burst, 0x04)\
	ENUM(burst, 0x08)\
	ENUM(pipeline_burst, 0x10)\
	ENUM(synchronous, 0x20)\
	ENUM(asynchronous, 0x40)

#define CacheType_ENUMERATORS\
	ENUM(other, 1)\
	ENUM(unknown, 2)\
	ENUM(instruction, 3)\
	ENUM(data, 4)\
	ENUM(unified, 5)

#define CacheAssociativity_ENUMERATORS\
	ENUM(other, 1)\
	ENUM(unknown, 2)\
	ENUM(direct_mapped, 3)\
	ENUM(A2, 4)\
	ENUM(A4, 5)\
	ENUM(full, 6)\
	ENUM(A8, 7)\
	ENUM(A16, 8)\
	ENUM(A12, 9)\
	ENUM(A24, 10)\
	ENUM(A32, 11)\
	ENUM(A48, 12)\
	ENUM(A64, 13)\
	ENUM(A20, 14)

#define Cache_FIELDS\
	FIELD(0, const char*, designation, "")\
	FIELD(0, CacheConfigurationFlags, configuration, "")\
	FIELD(F_INTERNAL, u16, maxSize16, "")\
	FIELD(F_INTERNAL, u16, installedSize16, "")\
	FIELD(0, CacheFlags, supportedFlags, "")\
	FIELD(0, CacheFlags, currentFlags, "")\
	FIELD(0, u8, speed, " ns")\
	FIELD(0, ECC, ecc, "")\
	FIELD(0, CacheType, type, "")\
	FIELD(0, CacheAssociativity, m_Associativity, "")\
	FIELD(F_DERIVED, size_t, level, "") /* 1..8 */\
	FIELD(F_DERIVED, CacheLocation, location, "")\
	FIELD(F_DERIVED, CacheMode, mode, "")\
	FIELD(F_DERIVED, Size<u64>, maxSize, "")\
	FIELD(F_DERIVED, Size<u64>, installedSize, "")


//-----------------------------------------------------------------------------
// PortConnector

#define PortConnectorType_ENUMERATORS\
	ENUM(other, 255)\
	ENUM(none, 0)\
	ENUM(centronics, 1)\
	ENUM(mini_centronics, 2)\
	ENUM(proprietary, 3)\
	ENUM(db25_male, 4)\
	ENUM(db25_pin_female, 5)\
	ENUM(db15_pin_male, 6)\
	ENUM(db15_pin_female, 7)\
	ENUM(db9_pin_male, 8)\
	ENUM(db9_pin_female, 9)\
	ENUM(rj11, 10)\
	ENUM(rj45, 11)\
	ENUM(mini_scsi, 12)\
	ENUM(mini_din, 13)\
	ENUM(micro_din, 14)\
	ENUM(ps2, 15)\
	ENUM(infrared, 16)\
	ENUM(hp_hil, 17)\
	ENUM(access_bus_usb, 18)\
	ENUM(pc_ssa_scsi, 19)\
	ENUM(din8_male, 20)\
	ENUM(din8_female, 21)\
	ENUM(on_board_ide, 22)\
	ENUM(on_board_floppy, 23)\
	ENUM(dual_inline_9, 24)\
	ENUM(dual_inline_25, 25)\
	ENUM(dual_inline_50, 26)\
	ENUM(dual_inline_68, 27)\
	ENUM(on_board_sound_input_from_cd, 28)\
	ENUM(mini_centronics_14, 29)\
	ENUM(mini_centronics_26, 30)\
	ENUM(headphones, 31)\
	ENUM(bnc, 32)\
	ENUM(pc_firewire, 33)\
	ENUM(sas_sata, 34)\
	ENUM(pc_98, 160)\
	ENUM(pc_98_hireso, 161)\
	ENUM(pc_h98, 162)\
	ENUM(pc_98_note, 163)\
	ENUM(pc_98_full, 164)

#define PortType_ENUMERATORS\
	ENUM(other, 255)\
	ENUM(none, 0)\
	ENUM(parallel_xt_at, 1)\
	ENUM(parallel_ps2, 2)\
	ENUM(parallel_ecp, 3)\
	ENUM(parallel_epp, 4)\
	ENUM(parallel_ecepp, 5)\
	ENUM(serial_xt_at, 6)\
	ENUM(serial_16450, 7)\
	ENUM(serial_16550, 8)\
	ENUM(serial_16550a, 9)\
	ENUM(scsi, 10)\
	ENUM(midi, 11)\
	ENUM(joystick, 12)\
	ENUM(keyboard, 13)\
	ENUM(mouse, 14)\
	ENUM(ssa_scsi, 15)\
	ENUM(usb, 16)\
	ENUM(firewire, 17)\
	ENUM(pcmcia_i, 18)\
	ENUM(pcmcia_ii, 19)\
	ENUM(pcmcia_iii, 20)\
	ENUM(cardbus, 21)\
	ENUM(access_bus, 22)\
	ENUM(scsi_ii, 23)\
	ENUM(scsi_wide, 24)\
	ENUM(pc_98, 25)\
	ENUM(pc_98_hireso, 26)\
	ENUM(pc_h98, 27)\
	ENUM(video, 28)\
	ENUM(audio, 29)\
	ENUM(modem, 30)\
	ENUM(network, 31)\
	ENUM(sata, 32)\
	ENUM(sas, 33)\
	ENUM(_8251_compatible, 160)\
	ENUM(_8251_fifo_compatible, 161)

#define PortConnector_FIELDS\
	FIELD(0, const char*, internalDesignator, "")\
	FIELD(0, PortConnectorType, internalConnectorType, "")\
	FIELD(0, const char*, externalDesignator, "")\
	FIELD(0, PortConnectorType, externalConnectorType, "")\
	FIELD(0, PortType, portType, "")


//-----------------------------------------------------------------------------
// SystemSlot

#define SystemSlotType_ENUMERATORS\
	ENUM(other, 1)\
	ENUM(unknown, 2)\
	ENUM(isa, 3)\
	ENUM(mca, 4)\
	ENUM(eisa, 5)\
	ENUM(pci, 6)\
	ENUM(pcmcia, 7)\
	ENUM(vesa, 8)\
	ENUM(proprietary, 9)\
	ENUM(processor, 10)\
	ENUM(memory_card, 11)\
	ENUM(io_riser, 12)\
	ENUM(nubus, 13)\
	ENUM(pci_66, 14)\
	ENUM(agp, 15)\
	ENUM(agp_2x, 16)\
	ENUM(agp_4x, 17)\
	ENUM(pcix, 18)\
	ENUM(agp_8x, 19)\
	ENUM(pc_98_c20, 160)\
	ENUM(pc_98_c24, 161)\
	ENUM(pc_98_e, 162)\
	ENUM(pc_98_local_bus, 163)\
	ENUM(pc_98_card, 164)\
	ENUM(pcie, 165)\
	ENUM(pcie_x1, 166)\
	ENUM(pcie_x2, 167)\
	ENUM(pcie_x4, 168)\
	ENUM(pcie_x8, 169)\
	ENUM(pcie_x16, 170)\
	ENUM(pcie2, 171)\
	ENUM(pcie2_x1, 172)\
	ENUM(pcie2_x2, 173)\
	ENUM(pcie2_x4, 174)\
	ENUM(pcie2_x8, 175)\
	ENUM(pcie2_x16, 176)\
	ENUM(pcie3, 177)\
	ENUM(pcie3_x1, 178)\
	ENUM(pcie3_x2, 179)\
	ENUM(pcie3_x4, 180)\
	ENUM(pcie3_x8, 181)\
	ENUM(pcie3_x16, 182)

#define SystemSlotBusWidth_ENUMERATORS\
	ENUM(other, 1)\
	ENUM(unknown, 2)\
	ENUM(_8, 3)\
	ENUM(_16, 4)\
	ENUM(_32, 5)\
	ENUM(_64, 6)\
	ENUM(_128, 7)\
	ENUM(x1, 8)\
	ENUM(x2, 9)\
	ENUM(x4, 10)\
	ENUM(x8, 11)\
	ENUM(x12, 12)\
	ENUM(x16, 13)\
	ENUM(x32, 14)

#define SystemSlotUsage_ENUMERATORS\
	ENUM(other, 1)\
	ENUM(unknown, 2)\
	ENUM(available, 3)\
	ENUM(in_use, 4)

#define SystemSlotLength_ENUMERATORS\
	ENUM(other, 1)\
	ENUM(unknown, 2)\
	ENUM(_short, 3)\
	ENUM(_long, 4)

#define SystemSlotFlags1_ENUMERATORS\
	ENUM(unknown, 0x1)\
	ENUM(v5, 0x2)\
	ENUM(v3_3, 0x4)\
	ENUM(shared, 0x8)\
	ENUM(pc_card_16, 0x10)\
	ENUM(pc_cardbus, 0x20)\
	ENUM(pc_zoom_video, 0x40)\
	ENUM(pc_modem_ring_resume, 0x80)

#define SystemSlotFlags2_ENUMERATORS\
	ENUM(pme, 0x1)\
	ENUM(hot_plug, 0x2)\
	ENUM(smbus, 0x4)\

#define SystemSlot_FIELDS\
	FIELD(0, const char*, designation, "")\
	FIELD(0, SystemSlotType, type, "")\
	FIELD(0, SystemSlotBusWidth, busWidth, "")\
	FIELD(0, SystemSlotUsage, usage, "")\
	FIELD(0, SystemSlotLength, length, "")\
	FIELD(0, u16, id, "")\
	FIELD(0, SystemSlotFlags1, flags1, "")\
	FIELD(0, SystemSlotFlags2, flags2, "")\
	FIELD(0, u16, segmentGroupNumber, "")\
	FIELD(0, u8, busNumber, "")\
	FIELD(F_INTERNAL, u8, functionAndDeviceNumber, "")\
	FIELD(F_DERIVED, u8, deviceNumber, "")\
	FIELD(F_DERIVED, u8, functionNumber, "")


//-----------------------------------------------------------------------------
// OnBoardDevices

#define OnBoardDeviceType_ENUMERATORS\
	ENUM(other, 1)\
	ENUM(unknown, 2)\
	ENUM(video, 3)\
	ENUM(scsi_controller, 4)\
	ENUM(ethernet, 5)\
	ENUM(token_ring, 6)\
	ENUM(sound, 7)\
	ENUM(pata_controller, 8)\
	ENUM(sata_controller, 9)\
	ENUM(sas_controller, 10)

#define OnBoardDevices_FIELDS\
	FIELD(0, OnBoardDeviceType, type, "")\
	FIELD(0, const char*, description, "")\
	FIELD(F_DERIVED, bool, enabled, "")\
	/* NB: this structure could contain any number of type/description pairs, but Dell BIOS only provides 1 */


//-----------------------------------------------------------------------------
// MemoryArray

#define MemoryArrayLocation_ENUMERATORS\
	ENUM(other, 1)\
	ENUM(unknown, 2)\
	ENUM(motherboard, 3)\
	ENUM(isa_addon, 4)\
	ENUM(eisa_addon, 5)\
	ENUM(pci_addon, 6)\
	ENUM(mca_addon, 7)\
	ENUM(pcmcia_addon, 8)\
	ENUM(proprietary_addon, 9)\
	ENUM(nubus, 10)\
	ENUM(pc_98_c20, 160)\
	ENUM(pc_98_c24, 161)\
	ENUM(pc_98_e, 162)\
	ENUM(pc_98_local_bus, 163)

#define MemoryArrayUse_ENUMERATORS\
	ENUM(other, 1)\
	ENUM(unknown, 2)\
	ENUM(system, 3)\
	ENUM(video, 4)\
	ENUM(flash, 5)\
	ENUM(nvram, 6)\
	ENUM(cache, 7)

#define MemoryArray_FIELDS\
	FIELD(0, MemoryArrayLocation, location, "")\
	FIELD(0, MemoryArrayUse, use, "")\
	FIELD(0, ECC, ecc, "")\
	FIELD(F_INTERNAL, u32, maxCapacity32, "")\
	FIELD(0, Handle, hError, "")\
	FIELD(0, u16, numDevices, "")\
	FIELD(0, Size<u64>, maxCapacity, "")


//-----------------------------------------------------------------------------
// MemoryDevice

#define MemoryDeviceFormFactor_ENUMERATORS\
	ENUM(other, 1)\
	ENUM(unknown, 2)\
	ENUM(SIMM, 3)\
	ENUM(SIP, 4)\
	ENUM(chip, 5)\
	ENUM(DIP, 6)\
	ENUM(ZIP, 7)\
	ENUM(proprietary_card, 8)\
	ENUM(DIMM, 9)\
	ENUM(TSOP, 10)\
	ENUM(row_of_chips, 11)\
	ENUM(RIMM, 12)\
	ENUM(SODIMM, 13)\
	ENUM(SRIMM, 14)\
	ENUM(FBDIMM, 15)

#define MemoryDeviceType_ENUMERATORS\
	ENUM(other, 1)\
	ENUM(unknown, 2)\
	ENUM(DRAM, 3)\
	ENUM(EDRAM, 4)\
	ENUM(VRAM, 5)\
	ENUM(SRAM, 6)\
	ENUM(RAM, 7)\
	ENUM(ROM, 8)\
	ENUM(FLASH, 9)\
	ENUM(EEPROM, 10)\
	ENUM(FEPROM, 11)\
	ENUM(EPROM, 12)\
	ENUM(CRRAM, 13)\
	ENUM(_3DRAM, 14)\
	ENUM(SDRAM, 15)\
	ENUM(SGRAM, 16)\
	ENUM(RDRAM, 17)\
	ENUM(DDR, 18)\
	ENUM(DDR2, 19)\
	ENUM(DDR2_FBDIMM, 20)\
	ENUM(DDR3, 24)\
	ENUM(FBD2, 25)

#define MemoryDeviceTypeFlags_ENUMERATORS\
	ENUM(other, 0x0002)\
	ENUM(unknown, 0x0004)\
	ENUM(fast_paged, 0x0008)\
	ENUM(static_column, 0x0010)\
	ENUM(pseudo_static, 0x0020)\
	ENUM(rambus, 0x0040)\
	ENUM(synchronous, 0x0080)\
	ENUM(cmos, 0x0100)\
	ENUM(edo, 0x0200)\
	ENUM(window_dram, 0x0400)\
	ENUM(cache_dram, 0x0800)\
	ENUM(non_volatile, 0x1000)\
	ENUM(buffered, 0x2000)\
	ENUM(unbuffered, 0x4000)

#define MemoryDevice_FIELDS\
	FIELD(0, Handle, hMemoryArray, "")\
	FIELD(0, Handle, hError, "")\
	FIELD(0, u16, totalWidth, " bits")\
	FIELD(0, u16, dataWidth, " bits")\
	FIELD(F_INTERNAL, u16, size16, "")\
	FIELD(0, MemoryDeviceFormFactor, formFactor, "")\
	FIELD(0, u8, deviceSet, "")\
	FIELD(0, const char*, locator, "")\
	FIELD(0, const char*, bank, "")\
	FIELD(0, MemoryDeviceType, type, "")\
	FIELD(0, MemoryDeviceTypeFlags, typeFlags, "")\
	FIELD(0, u16, speed, " MHz")\
	FIELD(0, const char*, manufacturer, "")\
	FIELD(0, const char*, serialNumber, "")\
	FIELD(0, const char*, assetTag, "")\
	FIELD(0, const char*, partNumber, "")\
	FIELD(F_INTERNAL, u8, attributes, "")\
	FIELD(F_INTERNAL, u32, size32, "")\
	FIELD(0, u16, configuredSpeed, " MHz")\
	FIELD(F_DERIVED, Size<u64>, size, "")\
	FIELD(F_DERIVED, u8, rank, "")\


//-----------------------------------------------------------------------------
// MemoryArrayMappedAddress

#define MemoryArrayMappedAddress_FIELDS\
	FIELD(F_INTERNAL, u32, startAddress32, "")\
	FIELD(F_INTERNAL, u32, endAddress32, "")\
	FIELD(0, Handle, hMemoryArray, "")\
	FIELD(0, u8, partitionWidth, "")\
	FIELD(F_HEX, u64, startAddress, "")\
	FIELD(F_HEX, u64, endAddress, "")


//-----------------------------------------------------------------------------
// MemoryDeviceMappedAddress

#define MemoryDeviceMappedAddress_FIELDS\
	FIELD(F_INTERNAL, u32, startAddress32, "")\
	FIELD(F_INTERNAL, u32, endAddress32, "")\
	FIELD(0, Handle, hMemoryDevice, "")\
	FIELD(0, Handle, hMemoryArrayMappedAddress, "")\
	FIELD(0, u8, partitionRowPosition, "")\
	FIELD(0, u8, interleavePosition, "")\
	FIELD(0, u8, interleavedDataDepth, "")\
	FIELD(F_HEX, u64, startAddress, "")\
	FIELD(F_HEX, u64, endAddress, "")


//----------------------------------------------------------------------------
// PortableBattery

#define PortableBatteryChemistry_ENUMERATORS\
	ENUM(other, 1)\
	ENUM(unknown, 2)\
	ENUM(lead_acid, 3)\
	ENUM(nickel_cadmium, 4)\
	ENUM(nickel_metal_hydride, 5)\
	ENUM(lithium_ion, 6)\
	ENUM(zinc_air, 7)\
	ENUM(lithium_polymer, 8)

#define PortableBattery_FIELDS\
	FIELD(0, const char*, location, "")\
	FIELD(0, const char*, manufacturer, "")\
	FIELD(0, const char*, date, "")\
	FIELD(0, const char*, serialNumber, "")\
	FIELD(0, const char*, deviceName, "")\
	FIELD(0, PortableBatteryChemistry, chemistry, "")\
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

#define VoltageProbeLocation_ENUMERATORS\
	ENUM(other, 1)\
	ENUM(unknown, 2)\
	ENUM(processor, 3)\
	ENUM(disk, 4)\
	ENUM(peripheral_bay, 5)\
	ENUM(system_management_module, 6)\
	ENUM(motherboard, 7)\
	ENUM(memory_module, 8)\
	ENUM(processor_module, 9)\
	ENUM(power_unit, 10)\
	ENUM(add_in_card, 11)

#define VoltageProbe_FIELDS\
	FIELD(0, const char*, description, "")\
	FIELD(F_INTERNAL, u8, locationAndStatus, "")\
	FIELD(0, i16, maxValue, " mV")\
	FIELD(0, i16, minValue, " mV")\
	FIELD(0, i16, resolution, " x 0.1 mV")\
	FIELD(0, i16, tolerance, " mV")\
	FIELD(0, i16, accuracy, " x 0.01%")\
	FIELD(0, u32, oemDefined, "")\
	FIELD(0, i16, nominalValue, " mv")\
	FIELD(F_DERIVED, VoltageProbeLocation, location, "")\
	FIELD(F_DERIVED, State, status, "")


//----------------------------------------------------------------------------
// CoolingDevice

#define CoolingDeviceType_ENUMERATORS\
	ENUM(other, 1)\
	ENUM(unknown, 2)\
	ENUM(fan, 3)\
	ENUM(centrifugal_blower, 4)\
	ENUM(chip_fan, 5)\
	ENUM(cabinet_fan, 6)\
	ENUM(power_supply_fan, 7)\
	ENUM(heat_pipe, 8)\
	ENUM(integrated_refrigeration, 9)\
	ENUM(active_cooling, 16)\
	ENUM(passive_cooling, 17)

#define CoolingDevice_FIELDS\
	FIELD(0, Handle, hTemperatureProbe, "")\
	FIELD(F_INTERNAL, u8, typeAndStatus, "")\
	FIELD(0, u8, group, "")\
	FIELD(0, u32, oemDefined, "")\
	FIELD(0, u16, nominalSpeed, " rpm")\
	FIELD(0, const char*, description, "")\
	FIELD(F_DERIVED, CoolingDeviceType, type, "")\
	FIELD(F_DERIVED, State, status, "")


//----------------------------------------------------------------------------
// TemperatureProbe

#define TemperatureProbeLocation_ENUMERATORS\
	ENUM(other, 1)\
	ENUM(unknown, 2)\
	ENUM(processor, 3)\
	ENUM(disk, 4)\
	ENUM(peripheral_bay, 5)\
	ENUM(system_management_module, 6)\
	ENUM(motherboard, 7)\
	ENUM(memory_module, 8)\
	ENUM(processor_module, 9)\
	ENUM(power_unit, 10)\
	ENUM(add_in_card, 11)\
	ENUM(front_panel_board, 12)\
	ENUM(back_panel_board, 13)\
	ENUM(power_system_board, 14)\
	ENUM(drive_backplane, 15)

#define TemperatureProbe_FIELDS\
	FIELD(0, const char*, description, "")\
	FIELD(F_INTERNAL, u8, locationAndStatus, "")\
	FIELD(0, i16, maxValue, " dDegC")\
	FIELD(0, i16, minValue, " dDegC")\
	FIELD(0, i16, resolution, " mDegC")\
	FIELD(0, i16, tolerance, " dDegC")\
	FIELD(0, i16, accuracy, " x 0.01%")\
	FIELD(0, u32, oemDefined, "")\
	FIELD(0, i16, nominalValue, " dDegC")\
	FIELD(F_DERIVED, TemperatureProbeLocation, location, "")\
	FIELD(F_DERIVED, State, status, "")


//----------------------------------------------------------------------------
// SystemBoot

#define SystemBootStatus_ENUMERATORS\
	ENUM(no_error, 0)\
	ENUM(no_bootable_media, 1)\
	ENUM(os_load_failed, 2)\
	ENUM(hardware_failure_firmware, 3)\
	ENUM(hardware_failure_os, 4)\
	ENUM(user_requested_boot, 5)\
	ENUM(security_violation, 6)\
	ENUM(previously_requested_image, 7)\
	ENUM(watchdog_expired, 8)

#define SystemBoot_FIELDS\
	FIELD(F_INTERNAL, u32, reserved32, "")\
	FIELD(F_INTERNAL, u16, reserved16, "")\
	FIELD(0, SystemBootStatus, status, "")\


//----------------------------------------------------------------------------
// ManagementDevice

#define ManagementDeviceType_ENUMERATORS\
	ENUM(other, 1)\
	ENUM(unknown, 2)\
	ENUM(LM75, 3)\
	ENUM(LM78, 4)\
	ENUM(LM79, 5)\
	ENUM(LM80, 6)\
	ENUM(LM81, 7)\
	ENUM(ADM9240, 8)\
	ENUM(DS1780, 9)\
	ENUM(M1617, 0xA)\
	ENUM(GL518SM, 0xB)\
	ENUM(W83781D, 0xC)\
	ENUM(HT82H791, 0xD)\

#define ManagementDeviceAddressType_ENUMERATORS\
	ENUM(other, 1)\
	ENUM(unknown, 2)\
	ENUM(port, 3)\
	ENUM(memory, 4)\
	ENUM(smbus, 5)

#define ManagementDevice_FIELDS\
	FIELD(0, const char*, description, "")\
	FIELD(0, ManagementDeviceType, type, "")\
	FIELD(0, u32, address, "")\
	FIELD(0, ManagementDeviceAddressType, addressType, "")


//----------------------------------------------------------------------------
// ManagementDeviceComponent

#define ManagementDeviceComponent_FIELDS\
	FIELD(0, const char*, description, "")\
	FIELD(0, Handle, hDevice, "")\
	FIELD(0, Handle, hComponent, "")\
	FIELD(0, Handle, hThreshold, "")


//----------------------------------------------------------------------------
// ManagementDeviceThreshold

#define ManagementDeviceThreshold_FIELDS\
	FIELD(0, i16, nonCriticalLo, "")\
	FIELD(0, i16, nonCriticalHi, "")\
	FIELD(0, i16, criticalLo, "")\
	FIELD(0, i16, criticalHi, "")\
	FIELD(0, i16, nonrecoverableLo, "")\
	FIELD(0, i16, nonrecoverableHi, "")


//----------------------------------------------------------------------------
// SystemPowerSupply

#define SystemPowerSupplyCharacteristics_ENUMERATORS\
	ENUM(hot_replaceable, 1)\
	ENUM(present, 2)\
	ENUM(unplugged, 4)

#define SystemPowerSupplyType_ENUMERATORS\
	ENUM(other, 1)\
	ENUM(unknown, 2)\
	ENUM(linear, 3)\
	ENUM(switching, 4)\
	ENUM(battery, 5)\
	ENUM(ups, 6)\
	ENUM(converter, 7)\
	ENUM(regulator, 8)

#define SystemPowerSupplyInputSwitching_ENUMERATORS\
	ENUM(other, 1)\
	ENUM(unknown, 2)\
	ENUM(manual, 3)\
	ENUM(auto_switch, 4)\
	ENUM(wide_range, 5)\
	ENUM(none, 6)

#define SystemPowerSupply_FIELDS\
	FIELD(0, u8, group, "")\
	FIELD(0, const char*, location, "")\
	FIELD(0, const char*, deviceName, "")\
	FIELD(0, const char*, manufacturer, "")\
	FIELD(0, const char*, serialNumber, "")\
	FIELD(0, const char*, assetTag, "")\
	FIELD(0, const char*, partNumber, "")\
	FIELD(0, const char*, revisionLevel, "")\
	FIELD(0, i16, maxPower, " mW")\
	FIELD(0, SystemPowerSupplyCharacteristics, characteristics, "")\
	FIELD(0, Handle, hVoltageProbe, "")\
	FIELD(0, Handle, hCoolingDevice, "")\
	FIELD(0, Handle, hCurrentProbe, "")\
	FIELD(F_DERIVED, SystemPowerSupplyType, type, "")\
	FIELD(F_DERIVED, State, status, "")\
	FIELD(F_DERIVED, SystemPowerSupplyInputSwitching, inputSwitching, "")\


//----------------------------------------------------------------------------
// OnboardDevices2

#define OnboardDevices2_FIELDS\
	FIELD(0, const char*, referenceDesignation, "")\
	FIELD(0, OnBoardDeviceType, type, "")\
	FIELD(0, u8, instance, "")\
	FIELD(0, u16, groupNumber, "")\
	FIELD(0, u8, busNumber, "")\
	FIELD(F_INTERNAL, u8, functionAndDeviceNumber, "")\
	FIELD(F_DERIVED, bool, enabled, "")\
	FIELD(F_DERIVED, u8, deviceNumber, "")\
	FIELD(F_DERIVED, u8, functionNumber, "")\


//-----------------------------------------------------------------------------

struct Header
{
	u8 id;
	u8 length;
	Handle handle;
};

// define each enumeration:
#define ENUM(enumerator, value) enumerator = value,
// (a struct wrapper allows reusing common enumerator names such as `other'.)
#define ENUMERATION(name, type)\
	struct name\
	{\
		/* (determines how much data to read from SMBIOS) */\
		typedef type T;\
		name(): value((Enum)0) {}\
		name(size_t num): value((Enum)num) {}\
		/* (the existence of this member type indicates the field is an enum) */\
		enum Enum { name##_ENUMERATORS sentinel } value;\
		/* (allows generic Field comparison against numeric_limits) */\
		operator size_t() const { return value; }\
	};
ENUMERATIONS
#undef ENUMERATION
#undef ENUM

// declare each structure
#define FIELD(flags, type, name, units) type name;
#define STRUCTURE(name, id)\
	struct name\
	{\
		/* (allows searching for a structure with a given handle) */\
		Header header;\
		/* (defines a linked list of instances of this structure type) */\
		name* next;\
		name##_FIELDS\
	};
STRUCTURES
#undef STRUCTURE
#undef FIELD

// declare a struct holding pointers (freed at exit) to each structure
struct Structures
{
#define STRUCTURE(name, id) name* name##_;
	STRUCTURES
#undef STRUCTURE
};

#pragma pack(pop)


//-----------------------------------------------------------------------------

/**
 * @return a pointer to a static Structures (i.e. always valid), with its
 * member pointers non-zero iff SMBIOS information is available and includes
 * the corresponding structure.
 *
 * thread-safe; return value should be cached (if possible) to avoid an
 * atomic comparison.
 **/
LIB_API const Structures* GetStructures();

/**
 * @return a string describing all structures (omitting fields with
 * meaningless or dummy values).
 **/
LIB_API std::string StringizeStructures(const Structures*);

}	// namespace SMBIOS

#endif	// #ifndef INCLUDED_SMBIOS
