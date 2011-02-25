# CPUID feature bits, from LSB to MSB:
# (Names and descriptions gathered from various Intel and AMD sources)

cap_raw = (
# EAX=01H ECX:
"""SSE3
PCLMULQDQ
DTES64: 64-bit debug store
MONITOR: MONITOR/MWAIT
DS-CPL: CPL qualified debug store
VMX: virtual machine extensions
SMX: safer mode extensions
EST: enhanced SpeedStep
TM2: thermal monitor 2
SSSE3
CNXT-ID: L1 context ID
?(ecx11)
FMA: fused multiply add
CMPXCHG16B
xTPR: xTPR update control
PDCM: perfmon and debug capability
?(ecx16)
PCID: process context identifiers
DCA: direct cache access
SSE4_1
SSE4_2
x2APIC: extended xAPIC support
MOVBE
POPCNT
TSC-DEADLINE
AES
XSAVE: XSAVE instructions supported
OSXSAVE: XSAVE instructions enabled
AVX
F16C: half-precision convert
?(ecx30)
RAZ: used by hypervisor to indicate guest status
""" +

# EAX=01H EDX:
"""FPU
VME: virtual 8086 mode enhancements
DE: debugging extension
PSE: page size extension
TSC: time stamp counter
MSR: model specific registers
PAE: physical address extension
MCE: machine-check exception
CMPXCHG8
APIC
?(edx10)
SEP: fast system call
MTRR: memory type range registers
PGE: page global enable
MCA: machine-check architecture
CMOV
PAT: page attribute table
PSE-36: 36-bit page size extension
PSN: processor serial number
CLFSH: CLFLUSH
?(edx20)
DS: debug store
ACPI
MMX
FXSR: FXSAVE and FXSTOR
SSE
SSE2
SS: self-snoop
HTT: hyper-threading
TM: thermal monitor
?(edx30)
PBE: pending break enable
""" +

# EAX=80000001H ECX:
"""LAHF: LAHF/SAHF instructions
CMP: core multi-processing legacy mode
SVM: secure virtual machine
ExtApic
AltMovCr8
ABM: LZCNT instruction
SSE4A
MisAlignSse
3DNowPrefetch
OSVW: OS visible workaround
IBS: instruction based sampling
XOP: extended operation support
SKINIT
WDT: watchdog timer support
?(ext:ecx14)
LWP: lightweight profiling support
FMA4: 4-operand FMA
?(ext:ecx17)
?(ext:ecx18)
NodeId
?(ext:ecx20)
TBM: trailing bit manipulation extensions
TopologyExtensions
?(ext:ecx23)
?(ext:ecx24)
?(ext:ecx25)
?(ext:ecx26)
?(ext:ecx27)
?(ext:ecx28)
?(ext:ecx29)
?(ext:ecx30)
?(ext:ecx31)
""" +

# EAX=80000001H ECX:
"""FPU[2]
VME[2]
DE[2]
PSE[2]
TSC[2]
MSR[2]
PAE[2]
MCE[2]
CMPXCHG8[2]
APIC[2]
?(ext:edx10)
SYSCALL: SYSCALL/SYSRET instructions
MTRR[2]
PGE[2]
MCA[2]
CMOV[2]
PAT[2]
PSE36[2]
?(ext:edx18)
MP: MP-capable
NX: no execute bit
?(ext:edx21)
MmxExt
MMX[2]
FXSR[2]
FFXSR
1GB: 1GB pages
RDTSCP
?(ext:edx28)
x86-64
3DNowExt
3DNow
"""
)

cap_bits = []
cap_descs = {}
idx = 0
for c in cap_raw.strip().split('\n'):
    s = c.split(':')
    if len(s) == 1:
        cap_bits.append((s[0], None, idx))
    else:
        cap_bits.append((s[0], s[1], idx))
        cap_descs[s[0]] = s[1]
    idx += 1
