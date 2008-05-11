#include "precompiled.h"
#include "ScriptableComplex.h"
#include "ScriptableComplex.inl"

#include "lib/allocators/bucket.h"

//-----------------------------------------------------------------------------
// suballocator for CJSComplex.m_Properties elements
// (must come after property defs, which are currently in the header)
//-----------------------------------------------------------------------------

static Bucket bucket;
// HACK: it needs to be created/destroyed; since there is no
// global init/shutdown call here, we keep a refcnt. this assumes that
// going to 0 <==> shutdown! if that proves wrong, bucket_alloc will warn.
static size_t suballoc_refs;	// initialized in suballoc_attach

void jscomplexproperty_suballoc_attach()
{
	ONCE(\
		size_t el_size = std::max(sizeof(CJSValComplexProperty), sizeof(CJSComplexProperty<int, true>));\
		(void)bucket_create(&bucket, el_size);\
		suballoc_refs = 0;\
	);
	suballoc_refs++;
}

void jscomplexproperty_suballoc_detach()
{
	suballoc_refs--;
	if(suballoc_refs == 0)
		bucket_destroy(&bucket);
}

void* jscomplexproperty_suballoc()
{
	return bucket_alloc(&bucket, 0);
}

void jscomplexproperty_suballoc_free(IJSComplexProperty* p)
{
	// explicit dtor since caller uses placement new
	p->~IJSComplexProperty();
	bucket_free(&bucket, p);
}
