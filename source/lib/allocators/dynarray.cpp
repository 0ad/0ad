/* Copyright (C) 2010 Wildfire Games.
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
 * dynamic (expandable) array
 */

#include "precompiled.h"
#include "lib/allocators/dynarray.h"

#include "lib/alignment.h"
#include "lib/sysdep/vm.h"


static Status validate_da(DynArray* da)
{
	if(!da)
		WARN_RETURN(ERR::INVALID_POINTER);
//	u8* const base           = da->base;
	const size_t max_size_pa = da->max_size_pa;
	const size_t cur_size    = da->cur_size;
	const size_t pos         = da->pos;

	// note: this happens if max_size == 0
//	if(debug_IsPointerBogus(base))
//		WARN_RETURN(ERR::_1);
	// note: don't check if base is page-aligned -
	// might not be true for 'wrapped' mem regions.
	if(!IsAligned(max_size_pa, pageSize))
		WARN_RETURN(ERR::_3);
	if(cur_size > max_size_pa)
		WARN_RETURN(ERR::_4);
	if(pos > cur_size || pos > max_size_pa)
		WARN_RETURN(ERR::_5);

	return INFO::OK;
}

#define CHECK_DA(da) RETURN_STATUS_IF_ERR(validate_da(da))


Status da_alloc(DynArray* da, size_t max_size)
{
	ENSURE(max_size != 0);
	const size_t max_size_pa = Align<pageSize>(max_size);

	u8* p = (u8*)vm::ReserveAddressSpace(max_size_pa);
	if(!p)
		return ERR::NO_MEM;	// NOWARN (already done in vm)

	da->base        = p;
	da->max_size_pa = max_size_pa;
	da->cur_size    = 0;
	da->cur_size_pa = 0;
	da->pos         = 0;
	CHECK_DA(da);
	return INFO::OK;
}


Status da_free(DynArray* da)
{
	CHECK_DA(da);

	vm::ReleaseAddressSpace(da->base, da->max_size_pa);

	// wipe out the DynArray for safety
	memset(da, 0, sizeof(*da));

	return INFO::OK;
}


Status da_set_size(DynArray* da, size_t new_size)
{
	CHECK_DA(da);

	// determine how much to add/remove
	const size_t cur_size_pa = Align<pageSize>(da->cur_size);
	const size_t new_size_pa = Align<pageSize>(new_size);
	const ssize_t size_delta_pa = (ssize_t)new_size_pa - (ssize_t)cur_size_pa;

	// not enough memory to satisfy this expand request: abort.
	// note: do not complain - some allocators (e.g. file_cache)
	// legitimately use up all available space.
	if(new_size_pa > da->max_size_pa)
		return ERR::LIMIT;	// NOWARN

	u8* end = da->base + cur_size_pa;
	bool ok = true;
	// expanding
	if(size_delta_pa > 0)
	{
		ok = vm::Commit(uintptr_t(end), size_delta_pa);
		if(!ok)
			debug_printf("Commit failed (%p %lld)\n", (void *)end, (long long)size_delta_pa);
	}
	// shrinking
	else if(size_delta_pa < 0)
		ok = vm::Decommit(uintptr_t(end+size_delta_pa), -size_delta_pa);
	// else: no change in page count, e.g. if going from size=1 to 2
	// (we don't want mem_* to have to handle size=0)

	da->cur_size = new_size;
	da->cur_size_pa = new_size_pa;
	CHECK_DA(da);
	return ok? INFO::OK : ERR::FAIL;
}


Status da_reserve(DynArray* da, size_t size)
{
	if(da->pos+size > da->cur_size_pa)
		RETURN_STATUS_IF_ERR(da_set_size(da, da->cur_size_pa+size));
	da->cur_size = std::max(da->cur_size, da->pos+size);
	return INFO::OK;
}


Status da_append(DynArray* da, const void* data, size_t size)
{
	RETURN_STATUS_IF_ERR(da_reserve(da, size));
	memcpy(da->base+da->pos, data, size);
	da->pos += size;
	return INFO::OK;
}
