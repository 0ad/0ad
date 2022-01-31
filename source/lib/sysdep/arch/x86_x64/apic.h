/* Copyright (C) 2022 Wildfire Games.
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

#ifndef INCLUDED_X86_X64_APIC
#define INCLUDED_X86_X64_APIC

typedef u8 ApicId;	// not necessarily contiguous values

/**
 * @return APIC ID of the currently executing processor or zero if the
 * platform does not have an xAPIC (i.e. 7th generation x86 or below).
 *
 * rationale: the alternative of accessing the APIC mmio registers is not
 * feasible. We also don't want to interfere with the OS's constant use of
 * the APIC registers.
 **/
ApicId GetApicId();

// if this returns false, apicId = contiguousId = processor.
// otherwise, there are unspecified but bijective mappings between
// apicId<->contiguousId and apicId<->processor.
bool AreApicIdsReliable();

// we may get the apicId of a processor we don't have access to.
bool IsProcessorKnown(ApicId apicId);

size_t ProcessorFromApicId(ApicId apicId);
size_t ContiguousIdFromApicId(ApicId apicId);

ApicId ApicIdFromProcessor(size_t contiguousId);
ApicId ApicIdFromContiguousId(size_t contiguousId);

#endif	// #ifndef INCLUDED_X86_X64_APIC
