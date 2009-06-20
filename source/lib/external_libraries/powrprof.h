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
 * bring in powrprof library.
 */

#ifndef INCLUDED_POWRPROF
#define INCLUDED_POWRPROF

#include "lib/sysdep/os/win/win.h"

#include <powrprof.h>

// the VC7 headers are missing some parts:

// MinGW headers are already correct; only change on VC
#if MSC_VERSION && MSC_VERSION < 1400

#ifndef NTSTATUS
#define NTSTATUS long
#endif
#ifndef STATUS_SUCCESS
#define STATUS_SUCCESS 0
#endif

#if WINVER < 0x500

typedef enum {
	SystemPowerPolicyAc,
	SystemPowerPolicyDc,
	VerifySystemPolicyAc,
	VerifySystemPolicyDc,
	SystemPowerCapabilities,
	SystemBatteryState,
	SystemPowerStateHandler,
	ProcessorStateHandler,
	SystemPowerPolicyCurrent,
	AdministratorPowerPolicy,
	SystemReserveHiberFile,
	ProcessorInformation,
	SystemPowerInformation,
	ProcessorStateHandler2,
	LastWakeTime,                                   // Compare with KeQueryInterruptTime()
	LastSleepTime,                                  // Compare with KeQueryInterruptTime()
	SystemExecutionState,
	SystemPowerStateNotifyHandler,
	ProcessorPowerPolicyAc,
	ProcessorPowerPolicyDc,
	VerifyProcessorPowerPolicyAc,
	VerifyProcessorPowerPolicyDc,
	ProcessorPowerPolicyCurrent,
	SystemPowerStateLogging,
	SystemPowerLoggingEntry
} POWER_INFORMATION_LEVEL;


typedef struct {
	DWORD       Granularity;
	DWORD       Capacity;
} BATTERY_REPORTING_SCALE, *PBATTERY_REPORTING_SCALE;

typedef enum _SYSTEM_POWER_STATE {
	PowerSystemUnspecified = 0,
	PowerSystemWorking     = 1,
	PowerSystemSleeping1   = 2,
	PowerSystemSleeping2   = 3,
	PowerSystemSleeping3   = 4,
	PowerSystemHibernate   = 5,
	PowerSystemShutdown    = 6,
	PowerSystemMaximum     = 7
} SYSTEM_POWER_STATE, *PSYSTEM_POWER_STATE;

typedef struct {
	// Misc supported system features
	BOOLEAN             PowerButtonPresent;
	BOOLEAN             SleepButtonPresent;
	BOOLEAN             LidPresent;
	BOOLEAN             SystemS1;
	BOOLEAN             SystemS2;
	BOOLEAN             SystemS3;
	BOOLEAN             SystemS4;           // hibernate
	BOOLEAN             SystemS5;           // off
	BOOLEAN             HiberFilePresent;
	BOOLEAN             FullWake;
	BOOLEAN             VideoDimPresent;
	BOOLEAN             ApmPresent;
	BOOLEAN             UpsPresent;

	// Processors
	BOOLEAN             ThermalControl;
	BOOLEAN             ProcessorThrottle;
	BYTE                ProcessorMinThrottle;
	BYTE                ProcessorMaxThrottle;
	BYTE                spare2[4];

	// Disk
	BOOLEAN             DiskSpinDown;
	BYTE                spare3[8];

	// System Battery
	BOOLEAN             SystemBatteriesPresent;
	BOOLEAN             BatteriesAreShortTerm;
	BATTERY_REPORTING_SCALE BatteryScale[3];

	// Wake
	SYSTEM_POWER_STATE  AcOnLineWake;
	SYSTEM_POWER_STATE  SoftLidWake;
	SYSTEM_POWER_STATE  RtcWake;
	SYSTEM_POWER_STATE  MinDeviceWakeState; // note this may change on driver load
	SYSTEM_POWER_STATE  DefaultLowLatencyWake;
} SYSTEM_POWER_CAPABILITIES, *PSYSTEM_POWER_CAPABILITIES;

#endif	// WINVER < 0x500

typedef struct _SYSTEM_POWER_INFORMATION
{
	ULONG MaxIdlenessAllowed;
	ULONG Idleness;
	ULONG TimeRemaining;
	UCHAR CoolingMode;
} SYSTEM_POWER_INFORMATION, *PSYSTEM_POWER_INFORMATION;

// SPI.CoolingMode
#define PO_TZ_INVALID_MODE 0 // The system does not support CPU throttling,
// or there is no thermal zone defined [..]

#endif	// #if MSC_VERSION

// neither VC7.1 nor MinGW define this
typedef struct _PROCESSOR_POWER_INFORMATION
{
	ULONG Number;
	ULONG MaxMhz;
	ULONG CurrentMhz;
	ULONG MhzLimit;
	ULONG MaxIdleState;
	ULONG CurrentIdleState;
} PROCESSOR_POWER_INFORMATION, *PPROCESSOR_POWER_INFORMATION;

#endif	// #ifndef INCLUDED_POWRPROF
