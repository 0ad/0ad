// memory manager and tracker
//
// Copyright (c) 2005 Jan Wassenberg
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// Contact info:
//   Jan.Wassenberg@stud.uni-karlsruhe.de
//   http://www.stud.uni-karlsruhe.de/~urkt/

#include "precompiled.h"

// for easy removal in release builds, so that we don't cause any overhead.
// note that any application calls to our functions must be removed also,
// but this is preferable to stubbing them out here ("least surprise").
#ifdef USE_MMGR

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>

#include <new>

#include "mmgr.h"
#include "lib.h"
#include "posix.h"
#include "sysdep/debug.h"

// remove macro hooks (we need to use the actual malloc/new etc. routines)
#include "nommgr.h"


//////////////////////////////////////////////////////////////////////////////
// locking for thread safety
//////////////////////////////////////////////////////////////////////////////

static pthread_mutex_t mutex;

// prevents using uninitialized lock before init (due to undefined
// NLSO ctor call order) or after shutdown
static bool lock_initialized;

static void lock_init() throw()
{
	if(pthread_mutex_init(&mutex, 0) == 0)
		lock_initialized = true;
}

static void lock_shutdown() throw()
{
	int ret = pthread_mutex_destroy(&mutex);
	assert2(ret == 0);
	lock_initialized = false;
}

static void lock() throw()
{
	if(lock_initialized)
	{
		int ret = pthread_mutex_lock(&mutex);
		assert2(ret == 0);
	}
}

static void unlock() throw()
{
	if(lock_initialized)
	{
		int ret = pthread_mutex_unlock(&mutex);
		assert2(ret == 0);
	}
}


//////////////////////////////////////////////////////////////////////////////
// options (enable/disable additional checks)
//////////////////////////////////////////////////////////////////////////////

// we can induce allocations to fail, for testing the application's
// error handling. uncomment and set to percentage that should fail.
// note: we use #define to make absolutely sure no
//       failures are induced unless desired.
//#define RANDOM_FAILURE 10.0

// note: padding size is in bytes, and is added before and after the
// user's buffer. pattern_set assumes it's an integral number of ulongs.

// enable all checks (slow!)
#ifdef PARANOIA
static uint options = MMGR_ALL;
static bool random_fill = true;
static const size_t padding_size  = 256 * sizeof(ulong);
// normal settings
#else
static uint options = MMGR_FILL;	// required for unused memory tracking
static bool random_fill = false;
static const size_t padding_size = 1 * sizeof(ulong);
#endif


uint mmgr_set_options(uint new_options)
{
	lock();

	if(new_options != MMGR_QUERY)
	{
		assert2(!(new_options & ~MMGR_ALL) && "unrecognized options set");
		options = new_options;
	}
	uint ret = options;

	unlock();
	return ret;
}


//////////////////////////////////////////////////////////////////////////////
// string formatting routines for log and reports
//////////////////////////////////////////////////////////////////////////////

extern const char* debug_get_symbol_string(void* symbol, const char* name, const char* file, int line);

const size_t NUM_SIZE = 32;
	// enough to cover even 64 bit numbers

static const char* insert_commas(char* out, size_t value)
{
	char num[NUM_SIZE];
	sprintf(num, "%u", value);
	const size_t num_len = strlen(num);
	assert2(num_len != 0);	// messes up #comma calc below

	const size_t out_len = num_len + (num_len-1)/3;
	char* pos = out+out_len;
	*pos-- = '\0';

	uint digits = 0;
	for(int i = (int)num_len-1; i >= 0; i--)
	{
		*pos-- = num[i];
		if(++digits == 3 && i != 0)
		{
			*pos-- = ',';
			digits = 0;
		}
	}

	return out;
}


static const char* format_size_string(char* str, size_t value)
{
	char num[NUM_SIZE];
	(void)insert_commas(num, value);
	if(value > GiB)
		sprintf(str, "%10s (%7.2fGi)", num, value / (float)GiB);
	else if(value > MiB)
		sprintf(str, "%10s (%7.2fMi)", num, value / (float)MiB);
	else if(value > KiB)
		sprintf(str, "%10s (%7.2fKi)", num, value / (float)KiB);
	else
		sprintf(str, "%10s bytes    ", num);
	return str;
}


//////////////////////////////////////////////////////////////////////////////
// allocator for Alloc objects
//////////////////////////////////////////////////////////////////////////////

enum AllocType
{
	AT_UNKNOWN      = 0,

	AT_MALLOC       = 1,
	AT_CALLOC       = 2,
	AT_REALLOC      = 3,
	AT_FREE         = 4,

	AT_NEW          = 5,
	AT_NEW_ARRAY    = 6,
	AT_DELETE       = 7,
	AT_DELETE_ARRAY = 8,

	AT_INVALID      = 9
};

// must match enum AllocType!
static const char* types[] =
{
	"unknown",
	"malloc",
	"calloc",
	"realloc",
	"free",
	"new",
	"new[]",
	"delete",
	"delete[]",
};

struct Alloc
{
	void* p;
	size_t size;
	uint num;
	Alloc* next;
	Alloc* prev;
	const char* owner;

	uint type : 4;
	uint break_on_free : 1;
	uint break_on_realloc : 1;

	void* user_p() const { return (char*)p + padding_size; }
	size_t user_size() const { return size - padding_size*2; }
};


static Alloc* freelist;
static Alloc** reservoirs = 0;	// array of pointers to runs of 256 Allocs
static size_t num_reservoirs   = 0;	// # entries

static Alloc* alloc_new()
{
	// If necessary, grow the freelist of unused Allocs
	if(!freelist)
	{
		freelist = (Alloc*)calloc(256, sizeof(Alloc));
		if(!freelist)
		{
			assert2(0 && "mmgr: failed to allocate freelist (out of memory)");
			return 0;
		}

		for(uint i = 0; i < 256 - 1; i++)
			freelist[i].next = &freelist[i+1];

		const size_t bytes = (num_reservoirs + 1) * sizeof(Alloc*);
		Alloc* *temp = (Alloc* *) realloc(reservoirs, bytes);
		assert2(temp);
		if(temp)
		{
			reservoirs = temp;
			reservoirs[num_reservoirs++] = freelist;
		}
	}

	// Grab a new Alloc from the front of the freelist
	Alloc* a = freelist;
	freelist = a->next;

	return a;
}

static void alloc_delete(Alloc* a)
{
	memset(a, 0, sizeof(Alloc));

	// add to the front of our freelist of unused Allocs
	a->next = freelist;
	freelist = a;
}

static void alloc_shutdown()
{
	if(reservoirs)
	{
		for(uint i = 0; i < num_reservoirs; i++)
			free(reservoirs[i]);
		free(reservoirs);
		reservoirs = 0;
		num_reservoirs = 0;
		freelist = 0;
	}
}


//////////////////////////////////////////////////////////////////////////////
// user allocation pointer -> Alloc lookup data structure
//////////////////////////////////////////////////////////////////////////////

// rationale:
// - split into separate routines (as opposed to exposing details to
//   user code) for easier modification.
// - may as well have used STL hash_map, but it's non-standard
//   thus far, and we have a hand-rolled container already.

static const size_t hash_entries = (1u << 11)+1;
	// ~8kb memory used; prime for better distribution
static Alloc* hash_table[hash_entries];

// return pointer to list of all Allocs with the same pointer hash.
static Alloc*& hash_chain(const void* pointer)
{
	uintptr_t address = reinterpret_cast<uintptr_t>(pointer);
	size_t index = ((size_t)address >> 4) % hash_entries;
	// many allocations are 16-byte aligned, so shift off lower 4 bits
	// (=> better hash distribution)
	return hash_table[index];
}

static void allocs_remove(const Alloc* a)
{
	Alloc*& chain = hash_chain(a->user_p());
	// it was at head of chain
	if(chain == a)
		chain = a->next;
	// in middle of chain
	else
	{
		if(a->prev)
			a->prev->next = a->next;
		if(a->next)
			a->next->prev = a->prev;
	}
}

static void allocs_add(Alloc* a)
{
	Alloc*& chain = hash_chain(a->user_p());
	if(chain)
		chain->prev = a;
	a->next = chain;
	a->prev = 0;
	chain = a;
}

static Alloc* allocs_find(const void* user_p)
{
	if(!user_p)
		assert2(user_p);

	Alloc* a = hash_chain(user_p);
	while(a)
	{
		if(a->user_p() == user_p)
			break;
		a = a->next;
	}
	return a;
}

static void allocs_foreach(void(*cb)(const Alloc*, void*), void* arg)
{
	for(uint i = 0; i < hash_entries; i++)
	{
		const Alloc* a = hash_table[i];
		while(a)
		{
			cb(a, arg);
			a = a->next;
		}
	}
}


//////////////////////////////////////////////////////////////////////////////
// padding: make sure the user hasn't over/underrun their buffer.
//////////////////////////////////////////////////////////////////////////////

static const ulong prefixPattern   = 0xbaadf00d;
static const ulong postfixPattern  = 0xdeadc0de;
static const ulong unusedPattern   = 0xfeedface;	// newly allocated
static const ulong releasedPattern = 0xdeadbeef;

static void pattern_set(const Alloc* a, ulong pattern)
{
	// For a serious test run, we use wipes of random a random value.
	// However, if this causes a crash, we don't want it to crash in a
	// different place each time, so we specifically DO NOT call srand.
	// If, by chance your program calls srand(), you may wish to disable
	// that when running with a random wipe test. This will make any
	// crashes more consistent so they can be tracked down.

	if(random_fill)
	{
		// note: rand typically returns 16 bits,
		// so one call isn't enough.
		pattern = 0;
		for(size_t shift = 0; shift < sizeof(ulong)*8; shift += 8)
			pattern |= (rand() & 0xff) << shift;
	}

	// We should wipe with zero if we're not in debug mode, so we can
	// help hide bugs if possible when we release the product.
	//
	// Note that options & MMGR_FILL should be turned on for this to have
	// any effect, otherwise it won't do much good. But we'll leave it
	// this way (as an option) because this does slow things down.
#ifndef NDEBUG
	pattern = 0;
#endif

	// fill user's data (optional)
	// note: don't use memset, because we want multi-byte patterns
	if(options & MMGR_FILL && a->user_size() > 0)
	{
		u8* p = (u8*)a->user_p();
		const size_t size = a->user_size();

		// write whole ulongs
		for(size_t i = 0; i < size / sizeof(ulong); i++)
		{
			*(ulong*)p = pattern;
			p += sizeof(ulong);
		}

		// remainder
		for(size_t shift = 0; shift < (size % sizeof(ulong)) * 8; shift += 8)
			*p++ = (u8)((pattern >> shift) & 0xff);
	}

	// fill prefix/postfix bytes (note: integral number of ulongs)
	ulong* pre  = (ulong*)a->p;
	ulong* post = (ulong*)( (char*)a->p + a->size - padding_size );
	for(uint i = 0; i < padding_size / sizeof(ulong); i++)
	{
		*pre++  = prefixPattern;
		*post++ = postfixPattern;
		// note: doesn't need to be split into 2 loops; cache is A2
	}
}


static bool padding_is_intact(const Alloc* a)
{
	ulong* pre  = (ulong*)a->p;
	ulong* post = (ulong*)( (char*)a->p + a->size - padding_size );

	for(uint i = 0; i < padding_size / sizeof(ulong); i++)
		if(*pre++ != prefixPattern || *post++ != postfixPattern)
			return false;

	return true;
}


static inline size_t calc_actual_size(const size_t user_size)
{
	return user_size + padding_size*2u;
}

static inline void* calc_user_p(const void* actual_p)
{
	if(!actual_p)
		return 0;
	return (char*)actual_p + padding_size;
}


//////////////////////////////////////////////////////////////////////////////
// unused memory reporting
//////////////////////////////////////////////////////////////////////////////

// return byte size of all the ulongs in the allocation whose contents
// haven't changed from the "unused" pattern since allocation.
// called from calc_all_unused_cb and log_this_alloc.
static size_t calc_unused(const Alloc* a)
{
	size_t total = 0;
	const ulong* p = (const ulong*)a->user_p();
	for(uint i = 0; i < a->user_size(); i += sizeof(ulong))
		if(*p++ == unusedPattern)
			total += sizeof(long);

	return total;
}


static void calc_all_unused_cb(const Alloc* a, void* arg)
{
	size_t* ptotal = (size_t*)arg;
	*ptotal += calc_unused(a);
}

// return total unused size in all allocations.
static size_t calc_all_unused()
{
	size_t total = 0;
	allocs_foreach(calc_all_unused_cb, &total);
	return total;
}


//////////////////////////////////////////////////////////////////////////////
// statistics: track current, cumulative, and peak allocations
//////////////////////////////////////////////////////////////////////////////

static struct Stats
{
	size_t cur_user_mem;
	size_t peak_user_mem;
	size_t total_user_mem;

	size_t cur_mem;
	size_t peak_mem;
	size_t total_mem;

	size_t cur_allocs;
	size_t peak_allocs;
	size_t total_allocs;
} stats;

// note: also called when reallocating. cumulative # allocations
// will increase, but that makes sense.
static void stats_add(const Alloc* a)
{
	const size_t size = a->size;
	const size_t user_size = a->user_size();

	stats.cur_user_mem += user_size;
	stats.cur_mem      += size;
	stats.cur_allocs++;

	stats.total_user_mem += user_size;
	stats.total_mem      += size;
	stats.total_allocs++;

	stats.peak_user_mem = MAX(stats.peak_user_mem, stats.cur_user_mem);
	stats.peak_mem      = MAX(stats.peak_mem, stats.cur_mem);
	stats.peak_allocs   = MAX(stats.peak_allocs, stats.cur_allocs);
}

static void stats_remove(const Alloc* a)
{
	const size_t size = a->size;
	const size_t user_size = a->user_size();

	stats.cur_user_mem -= user_size;
	stats.cur_mem      -= size;
	stats.cur_allocs--;
}


//////////////////////////////////////////////////////////////////////////////
// logging to file
//////////////////////////////////////////////////////////////////////////////

static void log_init();
static const char* const log_filename = "mem_log.txt";

static FILE* log_fp;

// open/append/close every call to make sure nothing gets lost when crashing.
// split out of log() to allow locked_log without code duplication.
static void vlog(const char* fmt, va_list args)
{
	log_init();

	(void)vfprintf(log_fp, fmt, args);

	// user requested each log line go directly to disk.
	if(options & MMGR_FLUSH_LOG)
		fflush(log_fp);
}

static void log(const char* fmt, ...)
{
	va_list	args;
	va_start(args, fmt);
	vlog(fmt, args);
	va_end(args);
}

// convenience function for overloaded new/delete that generate warnings
static void locked_log(const char* fmt, ...)
{
	lock();

	va_list	args;
	va_start(args, fmt);
	vlog(fmt, args);
	va_end(args);

	unlock();
}



static void log_init()
{
	// open => we're already initialized.
	if(log_fp)
		return;

	log_fp = fopen(log_filename, "w");
	if(!log_fp)
	{
		assert2(0 && "log file open failed");
		return;
	}

	//
	// write header
	//

	// get current time as string.
	// doesn't need to be reentrant, but we need to use strftime
	// elsewhere, so stick with it for consistency.
	char time_str[100];
	time_t t = time(0);
	const struct tm* tm_ = localtime(&t);
	(void)strftime(time_str, sizeof(time_str), "%#c", tm_);

	log("Memory manager log, started on %s.\n", time_str);
	log("\n");
	log("Logs errors, operations, and mmgr calls, depending on settings.\n");
	log("Entry types:\n");
	log("\n");
	log("  [!] - Error\n");
	log("  [?] - Warning\n");
	log("  [+] - Allocation\n");
	log("  [~] - Reallocation\n");
	log("  [-] - Deallocation\n");
	log("  [I] - Information\n");
	log("  [F] - Induced random failure to test the app's error handling\n");
	log("  [D] - Debug information for debugging mmgr\n");
	log("\n");
	log("Problematic allocations can be tracked by address+owner or number.\n");
	log("\n");
}


static void log_shutdown()
{
	if(log_fp)
	{
		fclose(log_fp);
		log_fp = 0;
	}
}


static void log_this_alloc(const Alloc* a)
{
	// duplicated in write_alloc_cb(); factoring out isn't worth it
	log("%06d 0x%08p 0x%08X 0x%08p 0x%08X 0x%08X %-8s    %c       %c    %s\n",
		a->num,
		a->user_p(), a->user_size(),
		a->p, a->size,
		calc_unused(a),
		types[a->type],
		a->break_on_free ? 'Y':'N',
		a->break_on_realloc ? 'Y':'N',
		a->owner
	);
}


//////////////////////////////////////////////////////////////////////////////
// write report text files (leaks, statistics)
//////////////////////////////////////////////////////////////////////////////

static void write_alloc_cb(const Alloc* a, void* arg)
{
	FILE* f = (FILE*)arg;

	// duplicated in log_this_alloc(Alloc*); factoring out isn't worth it
	fprintf(f, "%06d 0x%08p 0x%08X 0x%08p 0x%08X 0x%08X %-8s    %c       %c    %s\n",
		a->num,
		a->user_p(), a->user_size(),
		a->p, a->size,
		calc_unused(a),
		types[a->type],
		a->break_on_free ? 'Y':'N',
		a->break_on_realloc ? 'Y':'N',
		a->owner
		);
}

static void write_all_allocs(FILE* f)
{
	fprintf(f, "Alloc.   Addr       Size       Addr       Size                        BreakOn BreakOn              \n");
	fprintf(f, "Number Reported   Reported    Actual     Actual     Unused    Method  Dealloc Realloc Allocated by \n");
	fprintf(f, "------ ---------- ---------- ---------- ---------- ---------- -------- ------- ------- --------------------------------------------------- \n");

	allocs_foreach(write_alloc_cb, f);
}


void mmgr_write_report(void)
{
	FILE* f = fopen("mem_report.txt", "w");
	if(!f)
	{
		assert2(0 && "open of memory report file failed");
		return;
	}

	// get current time as string
	// (needs to be reentrant, so don't use asctime et al)
	char time_str[100];
	time_t t = time(0);
	const struct tm* tm_ = localtime(&t);
	(void)strftime(time_str, sizeof(time_str), "%#c", tm_);

	fprintf(f, "Detailed memory report:\n");
	fprintf(f, "Built %s %s; test run ended on %s\n", __DATE__, __TIME__, time_str);
	fprintf(f, "\n");

	char num[NUM_SIZE];
	char size[99];	// more than enough

	fprintf(f, "Unused:\n");
	fprintf(f, "-------\n");
	fprintf(f, "Memory allocated but not in use: %s\n", format_size_string(size, calc_all_unused()));
	fprintf(f, "\n");

	fprintf(f, "Peaks:\n");
	fprintf(f, "------\n");
	fprintf(f, "          Allocation unit count: %10s\n", insert_commas(num, stats.peak_allocs));
	fprintf(f, "        Reported to application: %s\n", format_size_string(size, stats.peak_user_mem));
	fprintf(f, "                         Actual: %s\n", format_size_string(size, stats.peak_mem));
	fprintf(f, "       Memory tracking overhead: %s\n", format_size_string(size, stats.peak_mem - stats.peak_user_mem));
	fprintf(f, "\n");

	fprintf(f, "Totals:\n");
	fprintf(f, "-------\n");
	fprintf(f, "          Allocation unit count: %10s\n", insert_commas(num, stats.total_allocs));
	fprintf(f, "        Reported to application: %s\n", format_size_string(size, stats.total_user_mem));
	fprintf(f, "                         Actual: %s\n", format_size_string(size, stats.total_mem));
	fprintf(f, "       Memory tracking overhead: %s\n", format_size_string(size, stats.total_mem - stats.total_user_mem));
	fprintf(f, "\n");

	fprintf(f, "Current:\n");
	fprintf(f, "--------\n");
	fprintf(f, "               Allocation count: %10s\n", insert_commas(num, stats.cur_allocs));
	fprintf(f, "        Reported to application: %s\n", format_size_string(size, stats.cur_user_mem));
	fprintf(f, "                         Actual: %s\n", format_size_string(size, stats.cur_mem));
	fprintf(f, "       Memory tracking overhead: %s\n", format_size_string(size, stats.cur_mem - stats.cur_user_mem));
	fprintf(f, "\n");

	write_all_allocs(f);

	fclose(f);
}

// only generate leak report at exit if the app has called
// mmgr_write_leak_report, since it's slow.
static bool app_wants_leak_report = false;

// separate from the main report - this is updated on every deallocate
// call when exiting, because each call may be the last
// (we can't say "everything after app shutdown is a leak", because
// it's common, albeit bad practice, for NLSOs to allocate memory).
void mmgr_write_leak_report(void)
{
	app_wants_leak_report = true;

	FILE* f = fopen("mem_leaks.txt", "w");
	if(!f)
	{
		assert2(0 && "open of memory leak report file failed");
		return;
	}

	// get current time as string
	// (needs to be reentrant, so don't use asctime et al)
	char time_str[100];
	time_t t = time(0);
	const struct tm* tm_ = localtime(&t);
	(void)strftime(time_str, sizeof(time_str), "%#c", tm_);

	fprintf(f, "Memory leak report:\n");
	fprintf(f, "Built %s %s; test run ended on %s\n", __DATE__, __TIME__, time_str);
	fprintf(f, "\n");

	const size_t num_leaks = stats.cur_allocs;
	if(num_leaks)
	{
		fprintf(f, "%d memory leak%s found:\n", num_leaks, num_leaks == 1 ? "":"s");
		fprintf(f, "\n");
		write_all_allocs(f);
	}
	else
		fprintf(f, "No leaks found! Congratulations!\n");

	fclose(f);
}


//////////////////////////////////////////////////////////////////////////////
// user-callable integrity checks
//////////////////////////////////////////////////////////////////////////////

bool mmgr_is_valid_ptr(const void* p)
{
	// null pointer - won't even try ;-)
	if (p == NULL)
		return false;
	lock();
	bool found = allocs_find(p) != 0;
	unlock();
	return found;
}


static bool alloc_is_valid(const Alloc* a)
{
	if(padding_is_intact(a))
		return true;

	// this allocation has been over/underrun, i.e. modified outside the
	// allocation's memory range.
	assert2(0 && "Memory over/underrun detected by mmgr");
	log("[!] Memory over/underrun:\n");
	log_this_alloc(a);
	return false;
}

struct ValidateAllParams
{
	uint num_invalid;
	uint num_allocs;
};

static void validate_all_cb(const Alloc* a, void* arg)
{
	ValidateAllParams* p = (ValidateAllParams*)arg;
	p->num_allocs++;
	if(!alloc_is_valid(a))
		p->num_invalid++;
}

// provide separate lock-is-held version to prevent recursive locks.
// called from mmgr_alloc/realloc/free; return true if all are valid.
static bool validate_all()
{
	ValidateAllParams params = { 0, 0 };
	allocs_foreach(validate_all_cb, &params);

	// Test for hash-table correctness
	if(params.num_allocs != stats.cur_allocs)
	{
		// our internal pointer->Alloc lookup data structure is inconsistent!
		// enable MMGR_VALIDATE_ALL, trigger this condition again,
		// and check the log for the last successful operation. the problem
		// will have occurred between then and now.
		assert2(0);
		log("[!] Memory tracking hash table corrupt!\n");
	}

	if(params.num_invalid)
	{
		assert2(0);
		log("[!] %d allocations are corrupt\n", params.num_invalid);
	}

	return params.num_invalid == 0;
}

bool mmgr_are_all_valid()
{
	lock();
	debug_check_heap();
	bool all_valid = validate_all();
	unlock();
	return all_valid;
}


//////////////////////////////////////////////////////////////////////////////
// init/shutdown hook - notifies us when ctor/dtor are called
//////////////////////////////////////////////////////////////////////////////

static bool static_dtor_called = false;

static struct NonLocalStaticObject
{
	NonLocalStaticObject()
	{
		// by calling now instead of on first lock() call,
		// we ensure no threads have been spawned yet.
		lock_init();

		// note: don't init log here - current directory hasn't yet been set.
		// the log file may otherwise be split between 2 files.
	}
	~NonLocalStaticObject()
	{
		// if the app requested a leak report before now, the deallocator
		// will update its leak report on every call (since dtors are now
		// being called, each may be the last). note that there is no
		// portable way to make sure a mmgr_shutdown() would be called
		// as the very last thing before exit.
		static_dtor_called = true;

		// don't shutdown the lock - some threads may still be active.
		// do so in shutdown() - see call site.
	}
} nlso;


static void shutdown(void)
{
	alloc_shutdown();
	log_shutdown();
	lock_shutdown();
}


//////////////////////////////////////////////////////////////////////////////
// trigger breakpoint when accessing specified allocations
//////////////////////////////////////////////////////////////////////////////

static uint cur_alloc_count = 0;
static uint break_on_count = 0;


void mmgr_break_on_alloc(uint count)
{
	lock();

	break_on_count = count;

	unlock();
}


void mmgr_break_on_realloc(const void* p)
{
	lock();

	Alloc* a = allocs_find(p);
	if(!a)
	{
		assert2(0 && "setting realloc breakpoint on invalid pointer");
		return;
	}

	// setting realloc breakpoint on an allocation that
	// doesn't support realloc.
	assert2(a->type == AT_MALLOC || a->type == AT_CALLOC ||
	        a->type == AT_REALLOC);

	a->break_on_realloc = true;

	unlock();
}


void mmgr_break_on_free(const void* p)
{
	lock();

	Alloc* a = allocs_find(p);
	if(!a)
	{
		assert2(0 && "setting free breakpoint on invalid pointer");
		return;
	}

	a->break_on_free = true;

	unlock();
}


//////////////////////////////////////////////////////////////////////////////
// actual allocator, making use of all of the above :)
//////////////////////////////////////////////////////////////////////////////

void* alloc_dbg(size_t user_size, AllocType type, const char* file, int line, const char* func, uint stack_frames)
{
	void* ret = 0;

	lock();

	if(options & MMGR_TRACE)
		log("[D] ENTER: alloc_dbg\n");

	void* caller = debug_get_nth_caller(1+stack_frames);
	const char* caller_string = debug_get_symbol_string(caller, func, file, line);

	if(options & MMGR_LOG_ALL)
		log("[+] %05d %8s of size 0x%08X(%08d) by %s\n", cur_alloc_count, types[type], user_size, user_size, caller_string);

	// caller's source file didn't include "mmgr.h"
	assert2(type != AT_UNKNOWN);

	// you requested a breakpoint on this allocation number
	++cur_alloc_count;
	assert2(cur_alloc_count != break_on_count);

	// simulate random failures
#ifdef RANDOM_FAILURE
	{
		double a = rand();
		double b = RAND_MAX / 100.0 * RANDOM_FAILURE;
		if(a < b)
		{
			log("[F] Random induced failure\n");
			goto fail;
		}
	}
#endif

	const size_t size = calc_actual_size(user_size);
	void* p = malloc(size);
	if(!p)
	{
		assert2(0);
		log("[!] Allocation failed (out of memory)\n");
		goto fail;
	}

	{
		Alloc* a = alloc_new();
		if(!a)
			goto fail;
		a->p     = p;
		a->size  = size;
		a->num   = cur_alloc_count;
		a->owner = caller_string;
		a->type  = type;
		a->break_on_free = a->break_on_realloc = 0;

		allocs_add(a);
		stats_add(a);

		pattern_set(a, unusedPattern);

		// calloc() must zero the memory
		if(type == AT_CALLOC)
			memset(a->user_p(), 0, a->user_size());

		if(options & MMGR_LOG_ALL)
			log("[+] ---->             addr 0x%08p\n", a->user_p());

		ret = a->user_p();
	}
fail:
	if(options & MMGR_VALIDATE_ALL)
		(void)validate_all();

	if(options & MMGR_TRACE)
		log("[D] EXIT : alloc_dbg\n");

	unlock();
	return ret;
}


void free_dbg(const void* user_p, AllocType type, const char* file, int line, const char* func, uint stack_frames)
{
	lock();

	if(options & MMGR_TRACE)
		log("[D] ENTER: free_dbg\n");

	void* caller = debug_get_nth_caller(1+stack_frames);
	const char* caller_string = debug_get_symbol_string(caller, func, file, line);

	if(options & MMGR_LOG_ALL)
		log("[-] ----- %8s of addr 0x%08p           by %s\n", types[type], user_p, caller_string);

	// freeing a zero pointer is allowed by C and C++, and a no-op.
	if(!user_p)
		goto done;


	//
	// security checks
	//
	{
		Alloc* a = allocs_find(user_p);
		if(!a)
		{
			// you tried to free a pointer mmgr didn't allocate
			assert2(0 && "mmgr tried to free a pointer mmgr didn't allocate");
			log("[!] mmgr_free: not allocated by this memory manager\n");
			goto fail;
		}
		// .. overrun? (note: alloc_is_valid already asserts if invalid)
		alloc_is_valid(a);
		// .. the owner wasn't compiled with mmgr.h
		assert2(type != AT_UNKNOWN);
		// .. allocator / deallocator type mismatch
		assert2(
			(type == AT_DELETE       && a->type == AT_NEW      ) ||
			(type == AT_DELETE_ARRAY && a->type == AT_NEW_ARRAY) ||
			(type == AT_FREE         && a->type == AT_MALLOC   ) ||
			(type == AT_FREE         && a->type == AT_CALLOC   ) ||
			(type == AT_FREE         && a->type == AT_REALLOC  )
		);
		// .. you requested a breakpoint when freeing this allocation
		assert2(!a->break_on_free);


		// "poison" the allocation's memory, to catch use-after-free bugs.
		// the VC7 debug heap does this also (in free), so we're wasting time
		// in that case. oh well, better to be safe/consistent.
		pattern_set(a, releasedPattern);

		free(a->p);

		allocs_remove(a);
		alloc_delete(a);
		stats_remove(a);
	}

	// we're being called from destructors. each call may be the last.
	if(static_dtor_called)
	{
		// update leak report (write a new one)
		if(app_wants_leak_report)
			mmgr_write_leak_report();

		// all allocations have been freed. there's no better time to shut
		// down, so do so now. (there's no portable way to call this as
		// the very last thing before exit)
		if(stats.cur_allocs == 0)
			shutdown();
	}

done:
fail:
	if(options & MMGR_VALIDATE_ALL)
		(void)validate_all();

	if(options & MMGR_TRACE)
		log("[D] EXIT : free_dbg\n");

	unlock();
}


void* realloc_dbg(const void* user_p, size_t user_size, AllocType type, const char* file, int line, const char* func, uint stack_frames)
{
	void* ret = 0;

	assert2(type == AT_REALLOC);

	lock();

	if(options & MMGR_TRACE)
		log("[D] ENTER: realloc_dbg\n");


	//
	// security checks
	//

	if(user_p)
	{
		Alloc* a = allocs_find(user_p);
		if(!a)
		{
			// you called realloc for a pointer mmgr didn't allocate
			assert2(0);
			log("[!] realloc: wasn't previously allocated\n");
			goto fail;
		}
		// .. the owner wasn't compiled with mmgr.h
		assert2(a->type != AT_UNKNOWN);
		// .. realloc for an allocation type that doesn't support it.
		assert2(a->type == AT_MALLOC || a->type == AT_CALLOC ||
			a->type == AT_REALLOC);
		// .. you requested a breakpoint when reallocating this allocation
		// (it will continue to be triggered unless you clear a->break_on_realloc)
		assert2(!a->break_on_realloc);
	}
	// else: skip security checks; realloc(0, size) is equivalent to malloc

	unlock();	// avoid recursive lock

	if(user_size)
		ret = alloc_dbg(user_size, type, file,line,func, stack_frames+1);

	if(user_p)
		free_dbg(user_p, type, file,line,func, stack_frames+1);

	lock();

	if(options & MMGR_LOG_ALL)
	{
		log("[~] ---->             from 0x%08X(%08d)\n", user_size, user_size);
		log("[~] ---->             addr 0x%08p\n", ret);
	}

fail:
	if(options & MMGR_VALIDATE_ALL)
		(void)validate_all();

	if(options & MMGR_TRACE)
		log("[D] EXIT : realloc_dbg\n");

	unlock();
	return ret;
}


//////////////////////////////////////////////////////////////////////////////
// wrappers
//////////////////////////////////////////////////////////////////////////////

void* mmgr_malloc_dbg(size_t size, const char* file, int line, const char* func)
{
	return alloc_dbg(size, AT_MALLOC, file,line,func, 1);
}
void* mmgr_calloc_dbg(size_t num, size_t size, const char* file, int line, const char* func)
{
	return alloc_dbg(num*size, AT_CALLOC, file,line,func, 1);
}
void* mmgr_realloc_dbg(void* p, size_t size, const char* file, int line, const char* func)
{
	return realloc_dbg(p, size, AT_REALLOC, file,line,func, 1);
}
void mmgr_free_dbg(void* p, const char* file, int line, const char* func)
{
	return free_dbg(p, AT_FREE, file,line,func, 1);
}


//
// note: we can call mmgr_malloc_dbg because the macro hook has set
// file+line+func, so the stack trace info won't be used and frames-to-skip
// is irrelevant.
//

char* mmgr_strdup_dbg(const char* s, const char* file, int line, const char* func)
{
	const size_t size = strlen(s)+1;
	char* copy = (char*)mmgr_malloc_dbg(size, file,line,func);
	if(!copy)
		return 0;
	strcpy(copy, s);
	return copy;
}

wchar_t* mmgr_wcsdup_dbg(const wchar_t* s, const char* file, int line, const char* func)
{
	const size_t size = (wcslen(s) + 1) * sizeof(wchar_t);
	wchar_t* copy = (wchar_t*)mmgr_malloc_dbg(size, file,line,func);
	if(!copy)
		return 0;
	wcscpy(copy, s);
	return copy;
}


//
// wrappers for more complicated functions that allocate memory.
// instead of reimplementing them entirely, we just make sure the memory
// returned here was allocated from our functions, so the user can call
// the hooked free().
//

char* mmgr_getcwd_dbg(char* buf, size_t buf_size, const char* file, int line, const char* func)
{
	char* ret = getcwd(buf, buf_size);
	// user already had a buffer or CRT version failed - pass on return value.
	if(buf || !ret)
		return ret;
	char* copy = mmgr_strdup_dbg(ret, file,line,func);
	free(ret);
	return copy;
}


//
// note: separate versions for new/new[], and the VC debug new(file, line).
//

static void* new_common(size_t size, AllocType type,
						const char* file, int line, const char* func)
{
	const char* allocator = types[type];

	if(options & MMGR_TRACE)
		log("[D] ENTER: %s\n", allocator);

	// C++ requires size==0 to return a unique address, so allocate 1 byte.
	if(size == 0)
		size = 1;

	// loop because C++ says error handler may free up some memory.
	for(;;)
	{
		void* p = alloc_dbg(size, type, file,line,func, 2);
		if(p)
		{
			if(options & MMGR_TRACE)
				log("[D] EXIT : %s\n", allocator);

			return p;
		}

		// is a handler set?
		std::new_handler nh = std::set_new_handler(0);
		(void)std::set_new_handler(nh);
		// .. yes: call, and loop again (hoping it freed up memory)
		if(nh)
			(*nh)();
		// .. no: throw
		else
		{
			if(options & MMGR_TRACE)
				log("[D] EXIT : %s\n", allocator);

			throw std::bad_alloc();
		}
	}
}


void* operator new(size_t size)
{
	return new_common(size, AT_NEW, 0,0,0);
}

void* operator new[](size_t size)
{
	return new_common(size, AT_NEW_ARRAY, 0,0,0);
}

void* operator new(size_t size, const char* file, int line)
{
	locked_log("[?] Overloaded(file,line) global operator new called - check call site");
	return new_common(size, AT_NEW, file,line,0);
}

void* operator new[](size_t size, const char* file, int line)
{
	locked_log("[?] Overloaded(file,line) global operator new[] called - check call site");
	return new_common(size, AT_NEW_ARRAY, file,line,0);
}

void* operator new(size_t size, const char* file, int line, const char* func)
{
	return new_common(size, AT_NEW, file,line,func);
}

void* operator new[](size_t size, const char* file, int line, const char* func)
{
	return new_common(size, AT_NEW_ARRAY, file,line,func);
}


void operator delete(void* p) throw()
{
	free_dbg(p, AT_DELETE, 0,0,0, 1);
}
void operator delete[](void* p) throw()
{
	free_dbg(p, AT_DELETE_ARRAY, 0,0,0, 1);
}

//
// called by compiler after a ctor (during the counterpart overloaded global
// operator new) raises an exception. not accessible from user code.
//

void operator delete(void* p, const char* file, int line) throw()
{
	locked_log("[?] Overloaded(file,line) global operator delete called, i.e. exception raised in a ctor");
	free_dbg(p, AT_DELETE, file,line,0, 1);
}
void operator delete[](void* p, const char* file, int line) throw()
{
	locked_log("[?] Overloaded(file,line) global operator delete[] called, i.e. exception raised in a ctor");
	free_dbg(p, AT_DELETE_ARRAY, file,line,0, 1);
}

void operator delete(void* p, const char* file, int line, const char* func) throw()
{
	locked_log("[?] Overloaded(file,line,func) global operator delete called, i.e. exception raised in a ctor");
	free_dbg(p, AT_DELETE, file,line,func, 1);
}
void operator delete[](void* p, const char* file, int line, const char* func) throw()
{
	locked_log("[?] Overloaded(file,line,func) global operator delete[] called, i.e. exception raised in a ctor");
	free_dbg(p, AT_DELETE_ARRAY, file,line,func, 1);
}

#endif	// #ifdef USE_MMGR
