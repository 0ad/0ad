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

/**
 * =========================================================================
 * File        : dynarray.cpp
 * Project     : 0 A.D.
 * Description : dynamic (expandable) array
 * =========================================================================
 */

#include "precompiled.h"
#include "dynarray.h"

#include "lib/posix/posix_mman.h"	// PROT_* constants for da_set_prot
#include "lib/sysdep/cpu.h"
#include "mem_util.h"


// indicates that this DynArray must not be resized or freed
// (e.g. because it merely wraps an existing memory range).
// stored in da->prot to reduce size; doesn't conflict with any PROT_* flags.
const int DA_NOT_OUR_MEM = 0x40000000;

static LibError validate_da(DynArray* da)
{
	if(!da)
		WARN_RETURN(ERR::INVALID_PARAM);
//	u8* const base           = da->base;
	const size_t max_size_pa = da->max_size_pa;
	const size_t cur_size    = da->cur_size;
	const size_t pos         = da->pos;
	const int prot           = da->prot;

	// note: this happens if max_size == 0
//	if(debug_IsPointerBogus(base))
//		WARN_RETURN(ERR::_1);
	// note: don't check if base is page-aligned -
	// might not be true for 'wrapped' mem regions.
//	if(!mem_IsPageMultiple((uintptr_t)base))
//		WARN_RETURN(ERR::_2);
	if(!mem_IsPageMultiple(max_size_pa))
		WARN_RETURN(ERR::_3);
	if(cur_size > max_size_pa)
		WARN_RETURN(ERR::_4);
	if(pos > cur_size || pos > max_size_pa)
		WARN_RETURN(ERR::_5);
	if(prot & ~(PROT_READ|PROT_WRITE|PROT_EXEC|DA_NOT_OUR_MEM))
		WARN_RETURN(ERR::_6);

	return INFO::OK;
}

#define CHECK_DA(da) RETURN_ERR(validate_da(da))


LibError da_alloc(DynArray* da, size_t max_size)
{
	const size_t max_size_pa = mem_RoundUpToPage(max_size);

	u8* p = 0;
	if(max_size_pa)	// (avoid mmap failure)
		RETURN_ERR(mem_Reserve(max_size_pa, &p));

	da->base        = p;
	da->max_size_pa = max_size_pa;
	da->cur_size    = 0;
	da->cur_size_pa = 0;
	da->prot        = PROT_READ|PROT_WRITE;
	da->pos         = 0;
	CHECK_DA(da);
	return INFO::OK;
}


LibError da_free(DynArray* da)
{
	CHECK_DA(da);

	u8* p            = da->base;
	size_t size_pa   = da->max_size_pa;
	bool was_wrapped = (da->prot & DA_NOT_OUR_MEM) != 0;

	// wipe out the DynArray for safety
	// (must be done here because mem_Release may fail)
	memset(da, 0, sizeof(*da));

	// skip mem_Release if <da> was allocated via da_wrap_fixed
	// (i.e. it doesn't actually own any memory). don't complain;
	// da_free is supposed to be called even in the above case.
	if(!was_wrapped && size_pa)
		RETURN_ERR(mem_Release(p, size_pa));
	return INFO::OK;
}


LibError da_set_size(DynArray* da, size_t new_size)
{
	CHECK_DA(da);

	if(da->prot & DA_NOT_OUR_MEM)
		WARN_RETURN(ERR::LOGIC);

	// determine how much to add/remove
	const size_t cur_size_pa = mem_RoundUpToPage(da->cur_size);
	const size_t new_size_pa = mem_RoundUpToPage(new_size);
	const ssize_t size_delta_pa = (ssize_t)new_size_pa - (ssize_t)cur_size_pa;

	// not enough memory to satisfy this expand request: abort.
	// note: do not complain - some allocators (e.g. file_cache)
	// legitimately use up all available space.
	if(new_size_pa > da->max_size_pa)
		return ERR::LIMIT;	// NOWARN

	u8* end = da->base + cur_size_pa;
	// expanding
	if(size_delta_pa > 0)
		RETURN_ERR(mem_Commit(end, size_delta_pa, da->prot));
	// shrinking
	else if(size_delta_pa < 0)
		RETURN_ERR(mem_Decommit(end+size_delta_pa, -size_delta_pa));
	// else: no change in page count, e.g. if going from size=1 to 2
	// (we don't want mem_* to have to handle size=0)

	da->cur_size = new_size;
	da->cur_size_pa = new_size_pa;
	CHECK_DA(da);
	return INFO::OK;
}


LibError da_reserve(DynArray* da, size_t size)
{
	if(da->pos+size > da->cur_size_pa)
		RETURN_ERR(da_set_size(da, da->cur_size_pa+size));
	da->cur_size = std::max(da->cur_size, da->pos+size);
	return INFO::OK;
}


LibError da_set_prot(DynArray* da, int prot)
{
	CHECK_DA(da);

	// somewhat more subtle: POSIX mprotect requires the memory have been
	// mmap-ed, which it probably wasn't here.
	if(da->prot & DA_NOT_OUR_MEM)
		WARN_RETURN(ERR::LOGIC);

	da->prot = prot;
	RETURN_ERR(mem_Protect(da->base, da->cur_size_pa, prot));

	CHECK_DA(da);
	return INFO::OK;
}


LibError da_wrap_fixed(DynArray* da, u8* p, size_t size)
{
	da->base        = p;
	da->max_size_pa = mem_RoundUpToPage(size);
	da->cur_size    = size;
	da->cur_size_pa = da->max_size_pa;
	da->prot        = PROT_READ|PROT_WRITE|DA_NOT_OUR_MEM;
	da->pos         = 0;
	CHECK_DA(da);
	return INFO::OK;
}


LibError da_read(DynArray* da, void* data, size_t size)
{
	// make sure we have enough data to read
	if(da->pos+size > da->cur_size)
		WARN_RETURN(ERR::FAIL);

	cpu_memcpy(data, da->base+da->pos, size);
	da->pos += size;
	return INFO::OK;
}


LibError da_append(DynArray* da, const void* data, size_t size)
{
	RETURN_ERR(da_reserve(da, size));
	cpu_memcpy(da->base+da->pos, data, size);
	da->pos += size;
	return INFO::OK;
}
