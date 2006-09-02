/**
 * =========================================================================
 * File        : snd_mgr.cpp
 * Project     : 0 A.D.
 * Description : OpenAL sound engine. handles sound I/O, buffer
 *             : suballocation and voice management/prioritization.
 *
 * @author Jan.Wassenberg@stud.uni-karlsruhe.de
 * =========================================================================
  */

/*
 * Copyright (c) 2004-2005 Jan Wassenberg
 *
 * Redistribution and/or modification are also permitted under the
 * terms of the GNU General Public License as published by the
 * Free Software Foundation (version 2 or later, at your option).
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
  */

#include "precompiled.h"

#include <sstream>	// to extract snd_open's definition file contents
#include <string>
#include <vector>
#include <algorithm>
#include <deque>
#include <math.h>

#include "maths/MathUtil.h"

#ifdef __APPLE__
# include <OpenAL/al.h>
# include <OpenAL/alc.h>
#else
# include <AL/al.h>
# include <AL/alc.h>
#endif

// for AL_FORMAT_VORBIS_EXT decl on Linux
#if OS_LINUX
# include <AL/alexttypes.h>
#endif

// for DLL-load hack in alc_init
#if OS_WIN
# include "lib/sysdep/win/win_internal.h"
#endif

#include "lib/res/res.h"
#include "snd_mgr.h"
#include "lib/timer.h"
#include "lib/app_hooks.h"


#define OGG_HACK
#include "ogghack.h"

#if MSC_VERSION
# pragma comment(lib, "openal32.lib")
#endif

// HACK: OpenAL loads and unloads certain DLLs several times on Windows.
// that looks unnecessary and wastes 100..400 ms on startup.
// we hold a reference to prevent the actual unload. everything works ATM;
// hopefully, OpenAL doesn't rely on them actually being unloaded.
#if OS_WIN
# define WIN_LOADLIBRARY_HACK 0
#endif


// components:
// - alc*: OpenAL context
//   readies OpenAL for use; allows specifying the device.
// - al_listener*: OpenAL listener
//   owns position/orientation and master gain.
// - al_buf*: OpenAL buffer suballocator
//   for convenience; also makes sure all have been freed at exit.
// - al_src*: OpenAL source suballocator
//   avoids high source alloc cost. also enforces user-set source limit.
// - al_init*: OpenAL startup mechanism
//   allows deferred init (speeding up start time) and runtime reset.
// - snd_dev*: device enumeration
//   lists names of all available devices (for sound options screen).
// - io_buf*: buffer suballocator
//   avoids frequent large alloc/frees and therefore heap fragmentation.
// - stream*: streaming
//   passes chunks of data (read via async I/O) to snd_data on request.
// - snd_data*: sound data provider
//   holds audio data (clip or stream) and returns OpenAL buffers on request.
// - hsd_list*: list of SndData instances
//   ensures all are freed when desired (despite being cached).
// - list*: list of active sounds.
//   sorts by priority for voice management, and has each VSrc update itself.
// - vsrc*: audio source
//   owns source properties and queue, references SndData.
// - vm*: voice management
//   grants the currently most 'important' sounds a hardware voice.


static bool al_initialized = false;
	// indicates OpenAL is ready for use. checked by other components
	// when deciding if they can pass settings changes to OpenAL directly,
	// or whether they need to be saved until init.


// used by snd_dev_set to reset OpenAL after device has been changed.
static LibError al_reinit();

// used by VSrc_reload to init on demand.
static LibError snd_init();
	
// used by al_shutdown to free all VSrc and SndData objects, respectively,
// so that they release their OpenAL sources and buffers.
static LibError list_free_all();
static void hsd_list_free_all();


/**
 * check if OpenAL indicates an error has occurred. it can only report
 * 1 error at a time, so this is called after every OpenAL request.
 *
 * @param caller Name of calling function (typically passed via __func__)
 */
static void al_check(const char * caller = "(unknown)")
{
	debug_assert(al_initialized);

	ALenum err = alGetError();
	if(err == AL_NO_ERROR)
		return;

	const char * str = (const char*)alGetString(err);
	debug_printf("OpenAL error: %s; called from %s\n", str, caller);
	debug_warn("OpenAL error");
}

// convenience version that automatically passes in function name.
#define AL_CHECK al_check(__func__)


///////////////////////////////////////////////////////////////////////////////
//
// OpenAL context: readies OpenAL for use; allows specifying the device,
// in case there are problems with OpenAL's default choice.
//
///////////////////////////////////////////////////////////////////////////////

static const char * alc_dev_name = 0;
	// default (0): use OpenAL default device.
	// note: that's why this needs to be a pointer instead of an array.


/**
 * tell OpenAL to use the specified device in future.
 *
 * @param alc_new_dev_name Device name.
 * @return LibError
 *
 * name = 0 reverts to OpenAL's default choice, which will also
 * be used if this routine is never called.
 *
 * the device name is typically taken from a config file at init-time;
 * the snd_dev * enumeration routines below are used to present a list
 * of choices to the user in the options screen.
 *
 * if OpenAL hasn't yet been initialized (i.e. no sounds have been opened),
 *   this just stores the device name for use when init does occur.
 *   note: we can't check now if it's invalid (if so, init will fail).
 * otherwise, we shut OpenAL down (thereby stopping all sounds) and
 * re-initialize with the new device. that's fairly time-consuming,
 * so preferably call this routine before sounds are loaded.
 */
LibError snd_dev_set(const char * alc_new_dev_name)
{
	// requesting a specific device
	if(alc_new_dev_name)
	{
		// already using that device - done. (don't re-init)
		if(alc_dev_name && !strcmp(alc_dev_name, alc_new_dev_name))
			return INFO_OK;

		// store name (need to copy it, since alc_init is called later,
		// and it must then still be valid)
		static char buf[32];
		strcpy_s(buf, sizeof(buf), alc_new_dev_name);
		alc_dev_name = buf;
	}
	// requesting default device
	else
	{
		// already using default device - done. (don't re-init)
		if(alc_dev_name == 0)
			return INFO_OK;

		alc_dev_name = 0;
	}

	return al_reinit();
		// no-op if not initialized yet, otherwise re-init.
}


static ALCcontext * alc_ctx = 0;
static ALCdevice * alc_dev = 0;


/**
 * free the OpenAL context and device.
 */
static void alc_shutdown()
{
	if(alc_ctx)
	{
		alcMakeContextCurrent(0);
		alcDestroyContext(alc_ctx);
	}

	if(alc_dev)
		alcCloseDevice(alc_dev);
}


/**
 * Ready OpenAL for use by setting up a device and context.
 *
 * @return LibError
 */
static LibError alc_init()
{
	LibError ret = INFO_OK;

#if WIN_LOADLIBRARY_HACK
	HMODULE dlls[3];
	dlls[0] = LoadLibrary("wrap_oal.dll");
	dlls[1] = LoadLibrary("setupapi.dll");
	dlls[2] = LoadLibrary("wdmaud.drv");
#endif

	// for reasons unknown, the NV native OpenAL implementation
	// causes an "invalid handle" exception internally when loaded
	// (it's not caused by the DLL load hack above). everything works and
	// we can continue normally; we just need to catch it to prevent the
	// unhandled exception filter from reporting it.
#if OS_WIN
	__try
	{
		alc_dev = alcOpenDevice((ALubyte*)alc_dev_name);
	}
	// if invalid handle, handle it; otherwise, continue handler search.
	__except(GetExceptionCode() == EXCEPTION_INVALID_HANDLE)
	{
		// ignore
	}
#else
	alc_dev = alcOpenDevice((ALCchar*)alc_dev_name);
#endif

	if(alc_dev)
	{
		alc_ctx = alcCreateContext(alc_dev, 0);	// no attrlist needed
		if(alc_ctx)
			alcMakeContextCurrent(alc_ctx);
	}

	// check if init succeeded.
	// some OpenAL implementations don't indicate failure here correctly;
	// we need to check if the device and context pointers are actually valid.
	ALCenum err = alcGetError(alc_dev);
	if(err != ALC_NO_ERROR || !alc_dev || !alc_ctx)
	{
		debug_printf("alc_init failed. alc_dev=%p alc_ctx=%p alc_dev_name=%s err=%d\n", alc_dev, alc_ctx, alc_dev_name, err);
		ret = ERR_FAIL;
	}

	// make note of which sound device is actually being used
	// (e.g. DS3D, native, MMSYSTEM) - needed when reporting OpenAL bugs.
	const char * dev_name = (const char*)alcGetString(alc_dev, ALC_DEVICE_SPECIFIER);
	wchar_t buf[200];
	swprintf(buf, ARRAY_SIZE(buf), L"SND| alc_init: success, using %hs\n", dev_name);
	ah_log(buf);

#if WIN_LOADLIBRARY_HACK
	// release DLL references, so BoundsChecker doesn't complain at exit.
	for(int i = 0; i < ARRAY_SIZE(dlls); i++)
		if(dlls[i] != INVALID_HANDLE_VALUE)
			FreeLibrary(dlls[i]);
#endif

	return ret;
}


///////////////////////////////////////////////////////////////////////////////
//
// listener: owns position/orientation and master gain.
// if they're set before al_initialized, we pass the saved values to
// OpenAL immediately after init (instead of waiting until next update).
//
///////////////////////////////////////////////////////////////////////////////

static float al_listener_gain = 1.0;
static float al_listener_pos[3] = { 0, 0, 0 };
static float al_listener_orientation[6] = {0, 0, -1, 0, 1, 0};
	// float view_direction[3], up_vector[3]; passed directly to OpenAL



/**
 * send the current listener properties to OpenAL.
 *
 * also called from al_init.
 */
static void al_listener_latch()
{
	if(al_initialized)
	{
		alListenerf(AL_GAIN, al_listener_gain);
		alListenerfv(AL_POSITION, al_listener_pos);
		alListenerfv(AL_ORIENTATION, al_listener_orientation);
		AL_CHECK;
	}
}


/**
 * set amplitude modifier, which is effectively applied to all sounds.
 * in layman's terms, this is the global "volume".
 *
 * @param gain Modifier: must be non-negative;
 * 1 -> unattenuated, 0.5 -> -6 dB, 0 -> silence.
 * @return LibError
 */
LibError snd_set_master_gain(float gain)
{
	if(gain < 0)
		WARN_RETURN(ERR_INVALID_PARAM);

	al_listener_gain = gain;

	al_listener_latch();
		// position will get sent too.
		// this isn't called often, so we don't care.

	return INFO_OK;
}


/**
 * set position of the listener (corresponds to camera in graphics).
 * coordinates are in world space; the system doesn't matter.
 *
 * @param pos position support vector
 * @param dir view direction
 * @param up up vector
 */
static void al_listener_set_pos(const float pos[3], const float dir[3], const float up[3])
{
	int i;
	for(i = 0; i < 3; i++)
		al_listener_pos[i] = pos[i];
	for(i = 0; i < 3; i++)
		al_listener_orientation[i] = dir[i];
	for(i = 0; i < 3; i++)
		al_listener_orientation[3+i] = up[i];

	al_listener_latch();
}


/**
 * get distance between listener and point.
 * this is used to determine sound priority.
 *
 * @param point position support vector
 * @return float euclidean distance squared
 */
static float al_listener_dist_2(const float point[3])
{
	const float dx = al_listener_pos[0] - point[0];
	const float dy = al_listener_pos[1] - point[1];
	const float dz = al_listener_pos[2] - point[2];
	return dx*dx + dy*dy + dz*dz;
}


///////////////////////////////////////////////////////////////////////////////
//
// AL buffer suballocator: allocates buffers as needed (alGenBuffers is fast).
// this interface is a bit more convenient than the OpenAL routines, and we
// verify that all buffers have been freed at exit.
//
///////////////////////////////////////////////////////////////////////////////

static int al_bufs_outstanding;


/**
 * allocate a new buffer, and fill it with the specified data.
 *
 * @param data raw sound data buffer
 * @param size size of buffer in bytes
 * @param al_fmt AL_FORMAT_ * describing the sound data
 * @param al_freq sampling frequency (typically 22050 Hz)
 * @return ALuint buffer name
 */
static ALuint al_buf_alloc(ALvoid * data, ALsizei size, ALenum al_fmt, ALsizei al_freq)
{
	ALuint al_buf;
	alGenBuffers(1, &al_buf);
	alBufferData(al_buf, al_fmt, data, size, al_freq);
	AL_CHECK;

	al_bufs_outstanding++;
	return al_buf;
}


/**
 * free the buffer and its contained sound data.
 *
 * @param al_buf buffer name
 */
static void al_buf_free(ALuint al_buf)
{
	// no-op if 0 (needed in case SndData_reload fails -
	// sd->al_buf will not have been set)
	if(!al_buf)
		return;

	debug_assert(alIsBuffer(al_buf));
	alDeleteBuffers(1, &al_buf);
	AL_CHECK;

	al_bufs_outstanding--;
}


/**
 * make sure all buffers have been returned to us via al_buf_free.
 * called from al_shutdown.
 */
static void al_buf_shutdown()
{
	if(al_bufs_outstanding != 0)
		debug_warn("not all buffers freed!");
}


///////////////////////////////////////////////////////////////////////////////
//
// AL source suballocator: allocate all available sources up-front and
// pass them out as needed (alGenSources is quite slow, taking 3..5 ms per
// source returned). also responsible for enforcing user-specified limit
// on total number of sources (to reduce mixing cost on low-end systems).
//
///////////////////////////////////////////////////////////////////////////////

// regardless of sound card caps, we won't use more than this ("enough").
// necessary in case OpenAL doesn't limit #sources (e.g. if SW mixing).
static const uint AL_SRC_MAX = 64;
// stack of sources (first allocated is [0])
static ALuint al_srcs[AL_SRC_MAX];
// number of valid sources in al_srcs[] (set by al_src_init)
static uint al_src_allocated;
// number of sources currently in use
static uint al_src_used = 0;
// user-set limit on how many sources may be used
static uint al_src_cap = AL_SRC_MAX;


/**
 * grab as many sources as possible up to the limit.
 * called from al_init.
 */
static void al_src_init()
{
	// grab as many sources as possible and count how many we get.
	for(uint i = 0; i < AL_SRC_MAX; i++)
	{
		alGenSources(1, &al_srcs[i]);
		// we've reached the limit, no more are available.
		if(alGetError() != AL_NO_ERROR)
			break;
		debug_assert(alIsSource(al_srcs[i]));
		al_src_allocated++;
	}

	// limit user's cap to what we actually got.
	// (in case snd_set_max_src was called before this)
	if(al_src_cap > al_src_allocated)
		al_src_cap = al_src_allocated;

	// make sure we got the minimum guaranteed by OpenAL.
	debug_assert(al_src_allocated >= 16);
}


/**
 * release all sources on freelist (currently stack).
 * all sources must have been returned to us via al_src_free.
 * called from al_shutdown.
 */
static void al_src_shutdown()
{
	debug_assert(al_src_used == 0);
	alDeleteSources(al_src_allocated, al_srcs);
	al_src_allocated = 0;
	AL_CHECK;
}


/**
 * try to allocate a source.
 *
 * @return ALuint source name, or 0 if none available
 */
static ALuint al_src_alloc()
{
	// no more to give
	if(al_src_used >= al_src_cap)
		return 0;
	ALuint al_src = al_srcs[al_src_used++];
	return al_src;
}


/**
 * mark a source as free and available for reuse.
 *
 * @param al_src source name
 */
static void al_src_free(ALuint al_src)
{
	debug_assert(alIsSource(al_src));
	al_srcs[--al_src_used] = al_src;
	debug_assert(al_src_used < al_src_allocated);
		// don't compare against cap - it might have been
		// decreased to less than were in use.
}


/**
 * set maximum number of voices to play simultaneously,
 * to reduce mixing cost on low-end systems.
 * this limit may be ignored if e.g. there's a stricter
 * implementation- defined ceiling anyway.
 *
 * @param limit max. number of sources
 * @return LibError
 */
LibError snd_set_max_voices(uint limit)
{
	// valid if cap is legit (less than what we allocated in al_src_init),
	// or if al_src_init hasn't been called yet. note: we accept anything
	// in the second case, as al_src_init will sanity-check al_src_cap.
	if(!al_src_allocated || limit < al_src_allocated)
	{
		al_src_cap = limit;
		return INFO_OK;
	}
	// user is requesting a cap higher than what we actually allocated.
	// that's fine (not an error), but we won't set the cap, since it
	// determines how many sources may be returned.
	// there's no return value to indicate this because the cap is
	// precisely that - an upper limit only, we don't care if it can't be met.
	else
		return INFO_OK;
}


///////////////////////////////////////////////////////////////////////////////
//
// OpenAL startup mechanism: allows deferring init until sounds are actually
// played, therefore speeding up perceived game start time.
// also resets OpenAL when settings (e.g. device) are changed at runtime.
//
///////////////////////////////////////////////////////////////////////////////

/**
 * master OpenAL init; makes sure all subsystems are ready for use.
 * called from each snd_open; no harm if called more than once.
 *
 * @return LibError
 */
static LibError al_init()
{
	// only take action on first call, OR when re-initializing.
	if(al_initialized)
		return INFO_OK;

	RETURN_ERR(alc_init());

	al_initialized = true;

	// these can't fail:
	al_src_init();
	al_listener_latch();

	return INFO_OK;
}


/**
 * shut down all module subsystems.
 */
static void al_shutdown()
{
	// was never initialized - nothing to do.
	if(!al_initialized)
		return;

	// somewhat tricky: go through gyrations to free OpenAL resources.

	// .. free all active sounds so that they release their source.
	//    the SndData reference is also removed,
	//    but these remain open, since they are cached.
	list_free_all();

	// .. actually free all (still cached) SndData instances.
	hsd_list_free_all();

	// .. all sources and buffers have been returned to their suballocators.
	//    now free them all.
	al_src_shutdown();
	al_buf_shutdown();

	alc_shutdown();

	al_initialized = false;
}


/**
 * re-initialize OpenAL. currently only required for changing devices.
 *
 * @return LibError
 */
static LibError al_reinit()
{
	// not yet initialized. settings have been saved, and will be
	// applied by the component init routines called from al_init.
	if(!al_initialized)
		return INFO_OK;

	// re-init (stops all currently playing sounds)
	al_shutdown();
	return al_init();
}


///////////////////////////////////////////////////////////////////////////////
//
// device enumeration: list all devices and allow the user to choose one,
// in case the default device has problems.
//
///////////////////////////////////////////////////////////////////////////////

// set by snd_dev_prepare_enum; used by snd_dev_next.
// consists of back-to-back C strings, terminated by an extra '\0'.
// (this is taken straight from OpenAL; dox say this format may change).
static const char * devs;

/**
 * Prepare to enumerate all device names (this resets the list returned by
 * snd_dev_next).
 *
 * may be called each time the device list is needed.
 *
 * @return LibError; always successful unless the requisite device
 * enumeration extension isn't available. in the latter case, 
 * a "cannot enum device" message should be presented to the user,
 * and snd_dev_set need not be called; OpenAL will use its default device.
 */
LibError snd_dev_prepare_enum()
{
	if(alcIsExtensionPresent(0, (ALCchar*)"ALC_ENUMERATION_EXT") != AL_TRUE)
		WARN_RETURN(ERR_NO_SYS);

	devs = (const char*)alcGetString(0, ALC_DEVICE_SPECIFIER);
	return INFO_OK;
}


/**
 * Get next device name.
 *
 * do not call unless snd_dev_prepare_enum succeeded!
 * not thread-safe! (static data from snd_dev_prepare_enum is used)
 *
 * @return const char * device name, or 0 if all have been returned
 */
const char * snd_dev_next()
{
	if(!*devs)
		return 0;
	const char * dev = devs;
	devs += strlen(dev)+1;
	return dev;
}


///////////////////////////////////////////////////////////////////////////////
//
// stream: passes chunks of data (read via async I/O) to snd_data on request.
//
///////////////////////////////////////////////////////////////////////////////

// one stream apiece for music and voiceover (narration during tutorial).
// allowing more is possible, but would be inefficent due to seek overhead.
// set this limit to catch questionable usage (e.g. streaming normal sounds).
static const int MAX_STREAMS = 2;

// maximum IOs queued per stream.
static const int MAX_IOS = 4;

static const size_t STREAM_BUF_SIZE = 32*KiB;


//
// I/O buffer suballocator: avoids frequent large alloc/frees
// when streaming, and therefore heap fragmentation.
//
///////////////////////////////////////////////////////////////////////////////

static const int TOTAL_IOS = MAX_STREAMS * MAX_IOS;
static const size_t TOTAL_BUF_SIZE = TOTAL_IOS * STREAM_BUF_SIZE;

// one large allocation for all buffers
static void * io_bufs;
	
// list of free buffers. start of buffer holds pointer to next in list.
static void * io_buf_freelist;


/**
 * Free an IO buffer.
 *
 * @param void * IO buffer
 */
static void io_buf_free(void * p)
{
	debug_assert(io_bufs <= p && p <= (char*)io_bufs+TOTAL_BUF_SIZE);
	*(void**)p = io_buf_freelist;
	io_buf_freelist = p;
}


/**
 * Allocate a memory pool for all IO buffers.
 * Called from first io_buf_alloc.
 */
static void io_buf_init()
{
	// allocate 1 big aligned block for all buffers.
	io_bufs = mem_alloc(TOTAL_BUF_SIZE, 4*KiB);
	// .. failed; io_buf_alloc calls will return 0
	if(!io_bufs)
		return;

	// build freelist.
	char * p = (char*)io_bufs;
	for(int i = 0; i < TOTAL_IOS; i++)
	{
		io_buf_free(p);
		p += STREAM_BUF_SIZE;
	}
}


/**
 * Allocate a fixed-size IO buffer.
 *
 * @return void * buffer, or 0 (and warning) if not enough memory.
 */
static void * io_buf_alloc()
{
	ONCE(io_buf_init());

	void * buf = io_buf_freelist;
	// note: we have to bail now; can't update io_buf_freelist.
	if(!buf)
	{
		// no buffer allocated
		if(!io_bufs)
			WARN_ERR(ERR_NO_MEM);
		// too many streams - can't happen (tm) because
		// stream_open enforces MAX_STREAMS.
		else
			WARN_ERR(ERR_LIMIT);

		return 0;
	}

	io_buf_freelist = *(void**)io_buf_freelist;
	return buf;
}


/**
 * Free memory pool holding all IO buffers.
 * no-op if io_buf_alloc was never called.
 * called by snd_shutdown.
 */
static void io_buf_shutdown()
{
	mem_free(io_bufs);
}


//
// stream: owns queue and buffers; uses VFS async I/O
//
///////////////////////////////////////////////////////////////////////////////

// rationale: no need for a centralized queue - we have a suballocator,
// so reallocs aren't a problem; central scheduling isn't necessary,
// because we'll only have <= 2 streams active at a time.

/**
 * Information for streaming sounds from file
 * (i.e. loading pieces of it in the background)
 */
struct Stream
{
	Handle hf;
	Handle ios[MAX_IOS];
	uint active_ios;
	/// set by stream_buf_get, used by stream_buf_discard to free buf.
	void * last_buf;
		
};

/**
 * Begin reading the next segment (asynchronously).
 * Called from SndData_reload and snd_data_buf_get.
 *
 * @param Stream*
 * @return LibError
 */
static LibError stream_issue(Stream * s)
{
	if(s->active_ios >= MAX_IOS)
		return INFO_OK;

	void * buf = io_buf_alloc();
	if(!buf)
		WARN_RETURN(ERR_NO_MEM);

	Handle h = vfs_io_issue(s->hf, STREAM_BUF_SIZE, buf);
	RETURN_ERR(h);
	s->ios[s->active_ios++] = h;
	return INFO_OK;
}


/**
 * Access the data most recently streamed in.
 *
 * @param Stream*
 * @param data pointer to buffer
 * @param size [bytes]
 * @return LibError; if the first pending IO hasn't completed,
 * ERR_AGAIN (not an error).
 */
static LibError stream_buf_get(Stream * s, void*& data, size_t& size)
{
	if(s->active_ios == 0)
		WARN_RETURN(ERR_EOF);
	Handle hio = s->ios[0];

	// has it finished? if not, bail.
	int is_complete = vfs_io_has_completed(hio);
	RETURN_ERR(is_complete);
	if(is_complete == 0)
		return ERR_AGAIN;	// NOWARN

	// get its buffer.
	// no delay, since vfs_io_has_completed == 1
	RETURN_ERR(vfs_io_wait(hio, data, size));

	// (next stream_buf_discard will free this buffer)
	s->last_buf = data;
	return INFO_OK;
}


/**
 * Free the buffer that was last returned by stream_buf_get,
 * and remove its IO slot from our queue.
 *
 * Must be called exactly once after every successful stream_buf_get;
 * call before calling any other stream_ * functions!
 *
 * @param Stream*
 * @return LibError
 */
static LibError stream_buf_discard(Stream * s)
{
	Handle hio = s->ios[0];

	LibError ret = vfs_io_discard(hio);

	// we implement the required 'circular queue' as a stack;
	// have to shift all items after this one down.
	s->active_ios--;
	for(uint i = 0; i < s->active_ios; i++)
		s->ios[i] = s->ios[i+1];

	io_buf_free(s->last_buf);	// can't fail
	s->last_buf = 0;

	return ret;
}


static uint active_streams;


/**
 * open a stream and begin reading from disk.
 *
 * @param Stream*
 * @param fn VFS filename.
 * @return LibError
 */
static LibError stream_open(Stream * s, const char * fn)
{
	// bail because we wouldn't have enough IO buffers for all
	if(active_streams >= MAX_STREAMS)
		WARN_RETURN(ERR_LIMIT);
	active_streams++;

	s->hf = vfs_open(fn);
	RETURN_ERR(s->hf);

	for(int i = 0; i < MAX_IOS; i++)
		RETURN_ERR(stream_issue(s));

	return INFO_OK;
}


/**
 * close a stream, which may currently be active.
 *
 * @param Stream*
 * @return LibError - the first error that occurred while waiting for
 * IOs / closing file.
 */
static LibError stream_close(Stream * s)
{
	LibError ret = INFO_OK;
	LibError err;

	// for each pending IO:
	for(uint i = 0; i < s->active_ios; i++)
	{
		// .. wait until complete,
		void * data; size_t size;	// unused
		do
			err = stream_buf_get(s, data, size);
		while(err == ERR_AGAIN);
		if(err < 0 && ret == 0)
			ret = err;

		// .. and discard.
		err = stream_buf_discard(s);
		if(err < 0 && ret == 0)
			ret = err;
	}

	err = vfs_close(s->hf);
	if(err < 0 && ret == 0)
		ret = err;

	active_streams--;

	return ret;
}


/**
 * Make sure the given Stream is valid/self-consistent.
 *
 * @param const Stream*
 * @return LibError
 */
static LibError stream_validate(const Stream * s)
{
	if(s->active_ios > MAX_IOS)
		WARN_RETURN(ERR_1);
	// <last_buf> has no invariant we could check.
	return INFO_OK;
}


///////////////////////////////////////////////////////////////////////////////
//
// sound data provider: holds audio data (clip or stream) and returns
// OpenAL buffers on request.
//
///////////////////////////////////////////////////////////////////////////////

// rationale for separate VSrc (instance) and SndData resources:
// - need to be able to fade out and cancel loops.
//   => VSrc isn't fire and forget; need to access sounds at runtime.
// - allowing access via direct pointer is unsafe
//   => need Handle-based access
// - don't want to reload sound data on every play()
//   => need either a separate caching mechanism or one central data resource.
// - want to support reloading (for consistency if not necessity)
//   => can't hack via h_find / setting fn_key to 0; need a separate instance.

// rationale for integrating IO logic into SndData:
// if the IO layer were completely separate, we'd need a callback to pass
// the buffer to OpenAL, or mark processed buffers as "discardable" for
// next time. both are ugly; we instead integrate IO into the SndData code.
// IOs are passed to OpenAL and then discarded immediately.

/**
 * Holder for sound data - either a clip, or stream.
 */
struct SndData
{
	// stream
	Stream s;
	ALenum al_fmt;
	ALsizei al_freq;

	// clip
	ALuint al_buf;

	uint is_stream : 1;
	uint is_valid : 1;

#ifdef OGG_HACK
// pointer to Ogg instance
void * o;
#endif
};

H_TYPE_DEFINE(SndData);

static void SndData_init(SndData * sd, va_list args)
{
	// olsner: pass as int instead of bool for GCC compat.
	sd->is_stream = va_arg(args, int) != 0;
}

static void SndData_dtor(SndData * sd)
{
	// in case reload failed and the following haven't been initialized yet
	// (freeing them would fail in that case, so bail)
	if(!sd->is_valid)
		return;

	if(sd->is_stream)
		stream_close(&sd->s);
	else
		al_buf_free(sd->al_buf);

#ifdef OGG_HACK
if(sd->o) ogg_release(sd->o);
#endif
}

static void hsd_list_add(Handle hsd);

static LibError SndData_reload(SndData * sd, const char * fn, Handle hsd)
{
	hsd_list_add(hsd);

	//
	// detect sound format by checking file extension
	//

	enum FileType
	{
		FT_OGG
	}
	file_type;

	const char * ext = path_extension(fn);
	// .. OGG (data will be passed directly to OpenAL)
	if(!stricmp(ext, "ogg"))
	{
#ifdef OGG_HACK
#else
		// first use of OGG: check if OpenAL extension is available.
		// note: this is required! OpenAL does its init here.
		static int ogg_supported = -1;
		if(ogg_supported == -1)
			ogg_supported = alIsExtensionPresent((ALubyte*)"AL_EXT_vorbis")? 1 : 0;
		if(!ogg_supported)
			WARN_RETURN(ERR_NO_SYS);

		sd->al_fmt  = AL_FORMAT_VORBIS_EXT;
		sd->al_freq = 0;
#endif

		file_type = FT_OGG;
	}
	// .. unknown extension
	else
		WARN_RETURN(ERR_UNKNOWN_FORMAT);

	// note: WAV is no longer supported. writing our own loader is infeasible
	// due to a seriously watered down spec with many incompatible variants.
	// pulling in an external library (e.g. freealut) is deemed not worth the
	// effort - OGG should be better in all cases.

	if(sd->is_stream)
	{
		// refuse to stream anything that cannot be passed directly to OpenAL -
		// we'd have to extract the audio data ourselves (not worth it).
		if(file_type != FT_OGG)
			WARN_RETURN(ERR_NOT_SUPPORTED);

		RETURN_ERR(stream_open(&sd->s, fn));
		sd->is_valid = 1;
		return INFO_OK;
	}

	// else: clip

	FileIOBuf file;
	size_t file_size;
	RETURN_ERR(vfs_load(fn, file, file_size));

	ALvoid * al_data = (ALvoid*)file;
	ALsizei al_size = (ALsizei)file_size;

#ifdef OGG_HACK
std::vector<u8> data;
data.reserve(500000);
if(file_type == FT_OGG)
{
 sd->o = ogg_create();
 ogg_give_raw(sd->o, (void*)file, file_size);
 ogg_open(sd->o, sd->al_fmt, sd->al_freq);
 size_t datasize=0;
 size_t bytes_read;
 do
 {
  const size_t bufsize = 32*KiB;
  char buf[bufsize];
  bytes_read = ogg_read(sd->o, buf, bufsize);
  data.insert(data.end(), &buf[0], &buf[bytes_read]);
  datasize += bytes_read;
 }
 while(bytes_read > 0);
 al_data = &data[0];
 al_size = (ALsizei)datasize;
}
else
{
 sd->o = NULL;
}
#endif

	sd->al_buf = al_buf_alloc(al_data, al_size, sd->al_fmt, sd->al_freq);
	sd->is_valid = 1;

	(void)file_buf_free(file);

	return INFO_OK;
}

static LibError SndData_validate(const SndData * sd)
{
	if(sd->al_fmt == 0)
		WARN_RETURN(ERR_11);
	if((uint)sd->al_freq > 100000)	// suspicious
		WARN_RETURN(ERR_12);
	if(sd->al_buf == 0)
		WARN_RETURN(ERR_13);

	if(sd->is_stream)
		RETURN_ERR(stream_validate(&sd->s));

	return INFO_OK;
}

static LibError SndData_to_string(const SndData * sd, char * buf)
{
	const char * type = sd->is_stream? "stream" : "clip";
	snprintf(buf, H_STRING_LEN, "%s; al_buf=%d", type, sd->al_buf);
	return INFO_OK;
}


/**
 * open and return a handle to a sound file's data.
 *
 * @param fn VFS filename
 * @param is_stream (default false) indicates whether this file should be
 * streamed in (opening is faster, it won't be kept in memory, but
 * only one instance can be open at a time; makes sense for large music files)
 * or loaded immediately.
 * @return Handle or LibError on failure
 */
static Handle snd_data_load(const char * fn, bool is_stream)
{
	// don't allow reloading stream objects
	// (both references would read from the same file handle).
	const uint flags = is_stream? RES_UNIQUE : 0;

	return h_alloc(H_SndData, fn, flags, (int)is_stream);
		// (int) rationale: see SndData_init
}



/**
 * Free the sound.
 *
 * @param hsd Handle to SndData; set to 0 afterwards.
 * @return LibError
 */
static LibError snd_data_free(Handle& hsd)
{
	return h_free(hsd, H_SndData);
}

//
// SndData instance list: ensures all allocated since last al_shutdown
// are freed when desired (they are cached => extra work needed).
//
///////////////////////////////////////////////////////////////////////////////

// rationale: all SndData objects (actually, their OpenAL buffers) must be
// freed during al_shutdown, to prevent leaks. we can't rely on list*
// to free all VSrc, and thereby their associated SndData objects -
// completed sounds are no longer in the list.
//
// nor can we use the h_mgr_shutdown automatic leaked resource cleanup:
// we need to be able to al_shutdown at runtime
// (when resetting OpenAL, after e.g. device change).
//
// h_mgr support is required to forcibly close SndData objects,
// since they are cached (kept open). it requires that this come after
// H_TYPE_DEFINE(SndData), since h_force_free needs the handle type.
//
// we never need to delete single entries: hsd_list_free_all
// (called by al_shutdown) frees each entry and clears the entire list.

typedef std::vector<Handle> Handles;
static Handles hsd_list;

/**
 * Add hsd to the list.
 * called from SndData_reload; will later be removed via hsd_list_free_all.
 * @param hsd Handle to SndData
 */
static void hsd_list_add(Handle hsd)
{
	hsd_list.push_back(hsd);
}


/**
 * Free all sounds on list.
 * called by al_shutdown (at exit, or when reinitializing OpenAL).
 */
static void hsd_list_free_all()
{
	for(Handles::iterator it = hsd_list.begin(); it != hsd_list.end(); ++it)
	{
		Handle& hsd = *it;

		(void)h_force_free(hsd, H_SndData);
		// ignore errors - if hsd was a stream, and its associated source
		// was active when al_shutdown was called, it will already have been
		// freed (list_free_all would free the source; it then releases
		// its SndData reference, which closes the instance because it's
		// RES_UNIQUE).
	}

	// leave its memory intact, so we don't have to reallocate it later
	// if we are now reinitializing OpenAL (not exiting).
	hsd_list.resize(0);
}


///////////////////////////////////////////////////////////////////////////////

/**
 * Get the sound's AL buffer (typically to play it)
 *
 * @param hsd Handle to SndData.
 * @param al_buf buffer name.
 * @return LibError, most commonly:
 * INFO_OK    = buffer has been returned; more are expected to be available.
 * ERR_EOF   = buffer has been returned but is the last one
 *             (end of file reached)
 * ERR_AGAIN = no buffer returned yet; still streaming in ATM.
 *             call back later.
 */
static LibError snd_data_buf_get(Handle hsd, ALuint& al_buf)
{
	LibError err = INFO_OK;

	// in case H_DEREF fails
	al_buf = 0;

	H_DEREF(hsd, SndData, sd);

	// clip: just return buffer (which was created in snd_data_load)
	if(!sd->is_stream)
	{
		al_buf = sd->al_buf;
		return ERR_EOF;	// NOWARN
	}

	// stream:

	// .. check if IO finished.
	void * data;
	size_t size;
	err = stream_buf_get(&sd->s, data, size);
	if(err == ERR_AGAIN)
		return ERR_AGAIN;	// NOWARN
	RETURN_ERR(err);

	// .. yes: pass to OpenAL and discard IO buffer.
	al_buf = al_buf_alloc(data, (ALsizei)size, sd->al_fmt, sd->al_freq);
	stream_buf_discard(&sd->s);

	// .. try to issue the next IO.
	// if EOF reached, indicate al_buf is the last that will be returned.
	err = stream_issue(&sd->s);
	if(err == ERR_EOF)
		return ERR_EOF;	// NOWARN
	RETURN_ERR(err);

	// al_buf valid and next IO issued successfully.
	return INFO_OK;
}


/**
 * Indicate the sound's buffer is no longer needed.
 *
 * @param hsd Handle to SndData.
 * @param al_buf buffer name
 * @return LibError
 */
static LibError snd_data_buf_free(Handle hsd, ALuint al_buf)
{
	H_DEREF(hsd, SndData, sd);

	// clip: no-op (caller will later release hsd reference;
	// when hsd actually unloads, sd->al_buf will be freed).
	if(!sd->is_stream)
		return INFO_OK;

	// stream: we had allocated an additional buffer, so free it now.
	al_buf_free(al_buf);
	return INFO_OK;
}


//-----------------------------------------------------------------------------
// fading
//-----------------------------------------------------------------------------

/**
 * control block for a fade operation.
 */
struct FadeInfo
{
	double start_time;
	FadeType type;
	float length;
	float initial_val;
	float final_val;
};

static float fade_factor_linear(float t)
{
	return t;
}

static float fade_factor_exponential(float t)
{
	// t**3
	return t*t*t;
}

static float fade_factor_s_curve(float t)
{
	// cosine curve
	float y = cos(t*PI + PI);
	// map [-1,1] to [0,1]
	return (y + 1.0f) / 2.0f;
}


/**
 * fade() return value; indicates if the fade operation is complete.
 */
enum FadeRet
{
	FADE_NO_CHANGE,
	FADE_CHANGED,
	FADE_TO_0_FINISHED
};

/**
 * Carry out the requested fade operation.
 *
 * This is called for each active VSrc; if they have no fade operation
 * active, nothing happens. Note: as an optimization, we could make a
 * list of VSrc with fade active and only call this for those;
 * not yet necessary, though.
 *
 * @param fi Describes the fade operation
 * @param cur_time typically returned via get_time()
 * @param out_val Output gain value, i.e. the current result of the fade.
 * @return FadeRet
 */
static FadeRet fade(FadeInfo& fi, double cur_time, float& out_val)
{
	// no fade in progress - abort immediately. this check is necessary to
	// avoid division-by-zero below.
	if(fi.type == FT_NONE)
		return FADE_NO_CHANGE;

	// end reached - if fi.length is 0, but the fade is "in progress", do the
	// processing here, and skip the dangerous division
	if(fi.type == FT_ABORT || (cur_time >= fi.start_time + fi.length))
	{
		// make sure exact value is hit
		out_val = fi.final_val;

		// special case: we were fading out; caller will free the sound.
		if(fi.final_val == 0.0f)
			return FADE_TO_0_FINISHED;

		// wipe out all values amd mark as no longer actively fading
		memset(&fi, 0, sizeof(fi));
		fi.type = FT_NONE;
		
		return FADE_CHANGED;
	}

	// how far into the fade are we? [0,1]
	const float t = (cur_time - fi.start_time) / fi.length;

	float factor;
	switch(fi.type)
	{
	case FT_LINEAR:
		factor = fade_factor_linear(t);
		break;
	case FT_EXPONENTIAL:
		factor = fade_factor_exponential(t);
		break;
	case FT_S_CURVE:
		factor = fade_factor_s_curve(t);
		break;

	// initialize with anything at all, just so that the calculation
	// below runs through; we reset out_val after that.
	case FT_ABORT:
		factor = 0.0f;
		break;

	NODEFAULT;
	}

	out_val = fi.initial_val + factor*(fi.final_val - fi.initial_val);

	return FADE_CHANGED;
}


/**
 * Is the fade operation currently active?
 *
 * @param FadeInfo
 * @return bool
 */
static bool fade_is_active(FadeInfo& fi)
{
	return (fi.type != FT_NONE);
}


///////////////////////////////////////////////////////////////////////////////
//
// virtual sound source: a sound the user wants played.
// owns source properties, buffer queue, and references SndData.
//
///////////////////////////////////////////////////////////////////////////////

// rationale: combine Src and VSrc - best interface, due to needing hsd,
// buffer queue (# processed) in update

enum VSrcFlags
{
	// SndData has reported EOF. will close down after last buffer completes.
	VS_EOF       = 1,

	// tell SndData to open the sound as a stream.
	// require caller to pass this explicitly, so they're aware
	// of the limitation that there can only be 1 instance of those.
	VS_IS_STREAM = 2,

	// this VSrc was added via list_add and needs to be removed with
	// list_remove in VSrc_dtor.
	// not set if load fails somehow (avoids list_remove "not found" error).
	VS_IN_LIST   = 4,

	VS_ALL_FLAGS = VS_EOF|VS_IS_STREAM|VS_IN_LIST
};

/**
 * control block for a virtual source, which represents a sound that the
 * application wants played. it may or may not be played, depending on
 * priority and whether an actual OpenAL source is available.
 */
struct VSrc
{
	/// handle to this VSrc, so that it can close itself.
	Handle hvs;

	/// associated sound data
	Handle hsd;

	// AL source properties (set via snd_set*)
	ALfloat pos[3];
	ALfloat gain;	/// [0,1]
	ALfloat pitch;	/// (0,1]
	ALboolean loop;
	ALboolean relative;

	/// controls vsrc_update behavior (VSrcFlags)
	uint flags;

	ALuint al_src;

	// priority for voice management
	float static_pri;	/// as given by snd_play
	float cur_pri;		/// holds newly calculated value

	FadeInfo fade;
};

H_TYPE_DEFINE(VSrc);

static void VSrc_init(VSrc * vs, va_list args)
{
	vs->flags = va_arg(args, uint);
	vs->fade.type = FT_NONE;
}

static void list_remove(VSrc * vs);
static LibError vsrc_reclaim(VSrc * vs);

static void VSrc_dtor(VSrc * vs)
{
	// only remove if added (not the case if load failed)
	if(vs->flags & VS_IN_LIST)
	{
		list_remove(vs);
		vs->flags &= ~VS_IN_LIST;
	}

	// these are safe, even if reload (partially) failed:
	vsrc_reclaim(vs);
	(void)snd_data_free(vs->hsd);
}

static LibError VSrc_reload(VSrc * vs, const char * fn, Handle hvs)
{
	// cannot wait till play(), need to init here:
	// must load OpenAL so that snd_data_load can check for OGG extension.
	LibError err = snd_init();
	// .. don't complain if sound is disabled; fail silently.
	if(err == ERR_AGAIN)
		return err;
	// .. catch genuine errors during init.
	RETURN_ERR(err);

	//
	// if extension is "txt", fn is a definition file containing the
	// sound file name and its gain; otherwise, read directly from fn
	// and assume default gain (1.0).
	//

	const char * snd_fn;		// actual sound file name
	std::string snd_fn_s;
	// extracted from stringstream;
	// declare here so that it doesn't go out of scope below.

	const char * ext = path_extension(fn);
	if(!stricmp(ext, "txt"))
	{
		FileIOBuf buf; size_t size;
		RETURN_ERR(vfs_load(fn, buf, size));
		std::istringstream def(std::string((char*)buf, (int)size));
		(void)file_buf_free(buf);

		float gain_percent;
		def >> snd_fn_s;
		def >> gain_percent;

		snd_fn = snd_fn_s.c_str();
		vs->gain = gain_percent / 100.0f;
	}
	else
	{
		snd_fn = fn;
		vs->gain = 1.0f;
	}

	// note: vs->gain can legitimately be > 1.0 - don't clamp.

	vs->pitch = 1.0f;

	vs->hvs = hvs;
	// needed so we can snd_free ourselves when done playing.

	bool is_stream = (vs->flags & VS_IS_STREAM) != 0;
	vs->hsd = snd_data_load(snd_fn, is_stream);
	RETURN_ERR(vs->hsd);

	return INFO_OK;
}

static LibError VSrc_validate(const VSrc * vs)
{
	// al_src can legitimately be 0 (if vs is low-pri)
	if(vs->flags & ~VS_ALL_FLAGS)
		WARN_RETURN(ERR_1);
	// no limitations on <pos>
	if(!(0.0f <= vs->gain && vs->gain <= 1.0f))
		WARN_RETURN(ERR_2);
	if(!(0.0f < vs->pitch && vs->pitch <= 1.0f))
		WARN_RETURN(ERR_3);
	if(*(u8*)&vs->loop > 1 || *(u8*)&vs->relative > 1)
		WARN_RETURN(ERR_4);
	// <static_pri> and <cur_pri> have no invariant we could check.
	return INFO_OK;
}

static LibError VSrc_to_string(const VSrc * vs, char * buf)
{
	snprintf(buf, H_STRING_LEN, "al_src = %d", vs->al_src);
	return INFO_OK;
}


/**
 * open and return a handle to a sound instance.
 *
 * @param snd_fn VFS filename. if a text file (extension ".txt"),
 * it is assumed to be a definition file containing the
 * sound file name and its gain (0.0 .. 1.0).
 * otherwise, it is taken to be the sound file name and
 * gain is set to the default of 1.0 (no attenuation).
 * @param is_stream (default false) indicates whether this file should be
 * streamed in (opening is faster, it won't be kept in memory, but
 * only one instance can be open at a time; makes sense for large music files)
 * or loaded immediately.
 * @return Handle or LibError on failure
 */
Handle snd_open(const char * snd_fn, bool is_stream)
{
	uint flags = 0;
	if(is_stream)
		flags |= VS_IS_STREAM;
	// note: RES_UNIQUE forces each instance to get a new resource
	// (which is of course what we want).
	return h_alloc(H_VSrc, snd_fn, RES_UNIQUE, flags);
}


/**
 * Free the sound; if it was playing, it will be stopped.
 * Note: sounds are closed automatically when done playing;
 * this is provided for completeness only.
 *
 * @param hvs Handle to VSrc. will be set to 0 afterwards.
 * @return LibError
 */
LibError snd_free(Handle& hvs)
{
	return h_free(hvs, H_VSrc);
}


//
// list of active sounds. used by voice management component,
// and to have each VSrc update itself (queue new buffers).
//
///////////////////////////////////////////////////////////////////////////////

// VSrc fields are used -> must come after struct VSrc

// sorted in descending order of current priority
// (we sometimes remove low pri items, which requires moving down
// everything that comes after them, so we want those to come last).
//
// don't use list, to avoid lots of allocs (expect thousands of VSrcs).
typedef std::deque<VSrc*> VSrcs;
typedef VSrcs::iterator It;
static VSrcs vsrcs;

// don't need to sort now - caller will list_sort() during update.
static void list_add(VSrc * vs)
{
	vsrcs.push_back(vs);
}


/**
 * call back for each VSrc entry in the list.
 *
 * @param cb Callback function
 * @param skip number of entries to skip (default 0)
 * @param end_idx if not the default value of 0, stop before that entry.
 */
static void list_foreach(void (*cb)(VSrc*), uint skip = 0, uint end_idx = 0)
{
	It begin = vsrcs.begin() + skip;
	It end = vsrcs.end();
	if(end_idx)
		end = vsrcs.begin()+end_idx;

	// can't use std::for_each: some entries may have been deleted
	// (i.e. set to 0) since last update.
	for(It it = begin; it != end; ++it)
	{
		VSrc * vs = *it;
		if(vs)
			cb(vs);
	}
}


static bool is_greater(const VSrc * vs1, const VSrc * vs2)
{
	return vs1->cur_pri > vs2->cur_pri;
}

/// sort list by decreasing 'priority' (most important first)
static void list_sort()
{
	std::sort(vsrcs.begin(), vsrcs.end(), is_greater);
}


/**
 * scan list and remove the given VSrc (by setting it to 0; list will be
 * pruned later (see rationale below).
 * O(N)!
 *
 * @param vs VSrc to remove.
 */
static void list_remove(VSrc * vs)
{
	for(size_t i = 0; i < vsrcs.size(); i++)
	{
		if(vsrcs[i] == vs)
		{
			// found it; several ways we could remove:
			// - shift everything else down (slow) -> no
			// - fill the hole with e.g. the last element
			//   (vsrcs would no longer be sorted by priority) -> no
			// - replace with 0 (will require prune_removed and
			//   more work in foreach) -> best alternative
			vsrcs[i] = 0;
			return;
		}
	}

	debug_warn("VSrc not found");
}


static bool is_null(VSrc * vs) { return (vs == 0); }

/**
 * remove entries that were set to 0 by list_remove, so that
 * code below can grant the first al_src_cap entries a soure.
 */
static void list_prune_removed()
{
	It new_end = remove_if(vsrcs.begin(), vsrcs.end(), is_null);
	vsrcs.erase(new_end, vsrcs.end());
}


static void vsrc_free(VSrc * vs) { snd_free(vs->hvs); }

static LibError list_free_all()
{
	list_foreach(vsrc_free);
	return INFO_OK;
}


///////////////////////////////////////////////////////////////////////////////

/**
 * Send the VSrc properties to OpenAL (when we actually have a source).
 * called by snd_set * and vsrc_grant.
 *
 * @param VSrc*
 */
static void vsrc_latch(VSrc * vs)
{
	if(!vs->al_src)
		return;

#ifndef NDEBUG
	// paranoid value checking; helps determine which parameter is
	// the problem when the below AL_CHECK fails.
	debug_assert(!isnan(vs->pos[0]) && !isnan(vs->pos[1]) && !isnan(vs->pos[2]));
	debug_assert(vs->relative == AL_TRUE || vs->relative == AL_FALSE);
	debug_assert(!isnan(vs->gain));
	debug_assert(!isnan(vs->pitch) && vs->pitch > 0.0f);
	debug_assert(vs->loop == AL_TRUE || vs->loop == AL_FALSE);
#endif

	alSourcefv(vs->al_src, AL_POSITION,        vs->pos);
	alSourcei (vs->al_src, AL_SOURCE_RELATIVE, vs->relative);
	alSourcef (vs->al_src, AL_GAIN,            vs->gain);
	alSourcef (vs->al_src, AL_PITCH,           vs->pitch);
	alSourcei (vs->al_src, AL_LOOPING,         vs->loop);

	AL_CHECK;
}


/**
 * Dequeue any of the VSrc's sound buffers that are finished playing.
 *
 * @param VSrc*
 * @return int number of entries that were removed.
 */
static int vsrc_deque_finished_bufs(VSrc * vs)
{
	int num_processed;
	alGetSourcei(vs->al_src, AL_BUFFERS_PROCESSED, &num_processed);

	for(int i = 0; i < num_processed; i++)
	{
		ALuint al_buf;
		alSourceUnqueueBuffers(vs->al_src, 1, &al_buf);
		snd_data_buf_free(vs->hsd, al_buf);
	}

	AL_CHECK;
	return num_processed;
}


// HACK: fade() requires the current time. we don't want to query that
// anew in every vsrc_update (slow and may mess up crossfades), and passing
// as a parameter isn't possible due to the list_foreach interface.
// therefore, static variable (set from snd_update).
static double snd_update_time;

/**
 * Update the VSrc - perform fade (if active), queue/unqueue buffers.
 * Called once a frame.
 *
 * @param VSrc*
 * @return LibError
 */
static LibError vsrc_update(VSrc * vs)
{
	if(!vs->al_src)
		return INFO_OK;

	FadeRet ret = fade(vs->fade, snd_update_time, vs->gain);
	// auto-free after fadeout.
	if(ret == FADE_TO_0_FINISHED)
	{
		vsrc_free(vs);
		return INFO_OK;	// don't continue - <vs> has been freed.
	}
	// fade in progress; latch current gain value.
	else if(ret == FADE_CHANGED)
		vsrc_latch(vs);

	int num_queued;
	alGetSourcei(vs->al_src, AL_BUFFERS_QUEUED, &num_queued);
	al_check("vsrc_update alGetSourcei");

	int num_processed = vsrc_deque_finished_bufs(vs);

	if(vs->flags & VS_EOF)
	{
		// no more buffers left, and EOF reached - done playing.
		if(num_queued == 0)
		{
			snd_free(vs->hvs);
			return INFO_OK;
		}
	}
	// can still read from SndData
	else
	{
		// decide how many buffers we are going to request
		// (start off with MAX_IOS; after that, replace finished buffers).
		int to_fill = MAX_IOS;
		if(num_queued > 0)
			to_fill = num_processed;

		// request and queue each buffer.
		int ret;
		do
		{
			ALuint al_buf;
			ret = snd_data_buf_get(vs->hsd, al_buf);
			// these 2 are legit (see above); otherwise, bail.
			if(ret != ERR_AGAIN && ret != ERR_EOF)
				RETURN_ERR(ret);

			alSourceQueueBuffers(vs->al_src, 1, &al_buf);
			al_check("vsrc_update alSourceQueueBuffers");
		}
		while(to_fill-- && ret == INFO_OK);

		// SndData has reported that no further buffers are available.
		if(ret == ERR_EOF)
			vs->flags |= VS_EOF;
	}

	return INFO_OK;
}


/**
 * Try to give the VSrc an AL source so that it can (re)start playing.
 * called by snd_play and voice management.
 *
 * @param VSrc*
 * @return LibError (ERR_FAIL if no AL source is available)
 */
static LibError vsrc_grant(VSrc * vs)
{
	// already playing - bail
	if(vs->al_src)
		return INFO_OK;

	// try to alloc source. if none are available, bail -
	// we get called in that hope that one is available by snd_play.
	vs->al_src = al_src_alloc();
	if(!vs->al_src)
		return ERR_FAIL;	// NOWARN

	// OpenAL docs don't specify default values, so initialize everything
	// ourselves to be sure. note: alSourcefv param is not const.
	float zero3[3] = { 0.0f, 0.0f, 0.0f };
	alSourcefv(vs->al_src, AL_VELOCITY,        zero3);
	alSourcefv(vs->al_src, AL_DIRECTION,       zero3);
	alSourcef (vs->al_src, AL_ROLLOFF_FACTOR,  0.0f);
	alSourcei (vs->al_src, AL_SOURCE_RELATIVE, AL_TRUE);
	AL_CHECK;

	// set remaining (user-specifiable) properties.
	vsrc_latch(vs);

	// queue up some buffers (enough to start playing, at least).
	vsrc_update(vs);

	alSourcePlay(vs->al_src);
	AL_CHECK;
	return INFO_OK;
}


/**
 * stop playback, and reclaim the OpenAL source.
 * called when closing the VSrc, or when voice management decides
 * this VSrc must yield to others of higher priority.
 *
 * @param VSrc*
 * @return LibError
 */
static LibError vsrc_reclaim(VSrc * vs)
{
	// don't own a source - bail.
	if(!vs->al_src)
		return ERR_FAIL;	// NOWARN

	alSourceStop(vs->al_src);
	AL_CHECK;
		// (note: all queued buffers are now considered 'processed')

	// deque and free remaining buffers (if sound was closed abruptly).
	vsrc_deque_finished_bufs(vs);

	al_src_free(vs->al_src);
	return INFO_OK;
}


///////////////////////////////////////////////////////////////////////////////

/**
 * Request the sound be played.
 *
 * Once done playing, the sound is automatically closed (allows
 * fire-and-forget play code).
 * if no hardware voice is available, this sound may not be played at all,
 * or in the case of looped sounds, start later.
 *
 * @param hvs Handle to VSrc
 * @param static_pri (min 0 .. max 1, default 0) indicates which sounds are
 * considered more important; this is attenuated by distance to the
 * listener (see snd_update).
 * @return LibError
 */
LibError snd_play(Handle hvs, float static_pri)
{
	H_DEREF(hvs, VSrc, vs);

	// note: vs->hsd is valid, otherwise snd_open would have failed
	// and returned an invalid handle (caught above).

	vs->static_pri = static_pri;
	list_add(vs);
	vs->flags |= VS_IN_LIST;

	// optimization (don't want to do full update here - too slow)
	// either we get a source and playing begins immediately,
	// or it'll be taken care of on next update.
	vsrc_grant(vs);
	return INFO_OK;
}


/**
 * Change 3d position of the sound source.
 *
 * May be called at any time; fails with invalid handle return if
 * the sound has already been closed (e.g. it never played).
 *
 * @param hvs Handle to VSrc
 * @param x,y,z coordinates (interpretation: see below)
 * @param relative if true, (x,y,z) is treated as relative to the listener;
 * otherwise, it is the position in world coordinates (default).
 * @return LibError
 */
LibError snd_set_pos(Handle hvs, float x, float y, float z, bool relative)
{
	H_DEREF(hvs, VSrc, vs);

	vs->pos[0] = x; vs->pos[1] = y; vs->pos[2] = z;
	vs->relative = relative;

	vsrc_latch(vs);
	return INFO_OK;
}


/**
 * Change gain (amplitude modifier) of the sound source.
 *
 * should not be called during a fade (see note in implementation);
 * fails with invalid handle return if the sound has already been
 * closed (e.g. it never played).
 *
 * @param hvs Handle to VSrc
 * @param gain modifier; must be non-negative;
 * 1 -> unattenuated, 0.5 -> -6 dB, 0 -> silence.
 * @return LibError
 */
LibError snd_set_gain(Handle hvs, float gain)
{
	H_DEREF(hvs, VSrc, vs);

	if(!(0.0f <= gain && gain <= 1.0f))
		WARN_RETURN(ERR_INVALID_PARAM);

	// if fading, gain changes would be overridden during the next
	// snd_update. attempting this indicates a logic error. we abort to
	// avoid undesired jumps in gain that might surprise (and deafen) user.
	if(fade_is_active(vs->fade))
		WARN_RETURN(ERR_LOGIC);

	vs->gain = gain;

	vsrc_latch(vs);
	return INFO_OK;
}


/**
 * Change pitch shift of the sound source.
 *
 * may be called at any time; fails with invalid handle return if
 * the sound has already been closed (e.g. it never played).
 *
 * @param hvs Handle to VSrc
 * @param pitch shift: 1.0 means no change; each reduction by 50% equals a
 * pitch shift of -12 semitones (one octave). zero is invalid.
 * @return LibError
 */
LibError snd_set_pitch(Handle hvs, float pitch)
{
	H_DEREF(hvs, VSrc, vs);

	if(!(0.0f < pitch && pitch <= 1.0f))
		WARN_RETURN(ERR_INVALID_PARAM);

	vs->pitch = pitch;

	vsrc_latch(vs);
	return INFO_OK;
}


/**
 * Enable/disable looping on the sound source.
 * used to implement variable-length sounds (e.g. while building).
 *
 * may be called at any time; fails with invalid handle return if
 * the sound has already been closed (e.g. it never played).
 *
 * notes:
 * - looping sounds are not discarded if they cannot be played for lack of
 *   a hardware voice at the moment play was requested.
 * - once looping is again disabled and the sound has reached its end,
 *   the sound instance is freed automatically (as if never looped).
 *
 * @param hvs Handle to VSrc
 * @param bool loop
 * @return LibError
 */
LibError snd_set_loop(Handle hvs, bool loop)
{
	H_DEREF(hvs, VSrc, vs);

	vs->loop = loop;

	vsrc_latch(vs);
	return INFO_OK;
}


/**
 * Fade the sound source in or out over time.
 * Its gain starts at <initial_gain> immediately and is moved toward
 * <final_gain> over <length> seconds.
 *
 * may be called at any time; fails with invalid handle return if
 * the sound has already been closed (e.g. it never played).
 *
 * note that this function doesn't busy-wait until the fade is complete;
 * any number of fades may be active at a time (allows cross-fading).
 * each snd_update calculates a new gain value for all pending fades.
 * it is safe to start another fade on the same sound source while
 * one is currently in progress; the old one is dropped.
 *
 * @param hvs Handle to VSrc
 * @param initial_gain gain. if < 0 (an otherwise illegal value), the sound's
 * current gain is used as the start value (useful for fading out).
 * @param final_gain gain. if 0, the sound is freed when the fade completes or
 * is aborted, thus allowing fire-and-forget fadeouts. no cases are
 * foreseen where this is undesirable, and it is easier to implement
 * than an extra set-free-after-fade-flag function.
 * @param length duration of fade [s]
 * @param type determines the fade curve: linear, exponential or S-curve.
 * for guidance on which to use, see
 * http://www.transom.org/tools/editing_mixing/200309.stupidfadetricks.html
 * you can also pass FT_ABORT to stop fading (if in progress) and
 * set gain to the final_gain parameter passed here.
 * @return LibError
 */
LibError snd_fade(Handle hvs, float initial_gain, float final_gain,
	float length, FadeType type)
{
	H_DEREF(hvs, VSrc, vs);

	if(type != FT_LINEAR && type != FT_EXPONENTIAL && type != FT_S_CURVE &&
	   type != FT_ABORT)
		WARN_RETURN(ERR_INVALID_PARAM);

	// special case - set initial value to current gain (see above).
	if(initial_gain < 0.0f)
		initial_gain = vs->gain;

	const double cur_time = get_time();

	FadeInfo& fi = vs->fade;
	fi.type        = type;
	fi.start_time  = cur_time;
	fi.initial_val = initial_gain;
	fi.final_val   = final_gain;
	fi.length      = length;

	(void)fade(fi, cur_time, vs->gain);
	vsrc_latch(vs);

	return INFO_OK;
}


///////////////////////////////////////////////////////////////////////////////
//
// voice management: grants the currently most 'important' sounds
// a hardware voice.
//
///////////////////////////////////////////////////////////////////////////////


/// length of vector squared (avoids costly sqrt)
static float magnitude_2(const float v[3])
{
	return v[0]*v[0] + v[1]*v[1] + v[2]*v[2];
}


/**
 * determine new priority of the VSrc based on distance to listener and
 * static priority.
 * called via list_foreach.
 *
 * @param VSrc*
 */
static void calc_cur_pri(VSrc * vs)
{
	const float MAX_DIST_2 = 1000.0f;
	const float falloff = 10.0f;

	float d_2;	// euclidean distance to listener (squared)
	if(vs->relative)
		d_2 = magnitude_2(vs->pos);
	else
		d_2 = al_listener_dist_2(vs->pos);

	// scale priority down exponentially
	float e = d_2 / MAX_DIST_2;	// 0.0f (close) .. 1.0f (far)

	// assume farther away than OpenAL cutoff - no sound contribution
	float cur_pri = -1.0f;
	if(e < 1.0f)
		cur_pri = vs->static_pri / pow(falloff, e);
	vs->cur_pri = cur_pri;
}


/**
 * convenience function that strips all unimportant VSrc of their AL source.
 * called via list_foreach; also immediately frees discarded clips.
 */
static void reclaim(VSrc * vs)
{
	vsrc_reclaim(vs);

	if(!vs->loop)
		snd_free(vs->hvs);
}

/// thunk that allows calling via list_foreach (return types differ)
static void grant(VSrc * vs)
{
	vsrc_grant(vs);
}


/**
 * update voice management, i.e. recalculate priority and assign AL sources.
 * no-op if OpenAL not yet initialized.
 *
 * @return LibError
 */
static LibError vm_update()
{
	list_prune_removed();

	// update current priorities (a function of static priority and distance).
	list_foreach(calc_cur_pri);

	// sort by descending current priority.
	list_sort();

	// partition list; the first al_src_cap will be granted a source
	// (if they don't have one already), after reclaiming all sources from
	// the remainder of the VSrc list entries.
	uint first_unimportant = MIN((uint)vsrcs.size(), al_src_cap); 
	list_foreach(reclaim, first_unimportant, 0);
	list_foreach(grant, 0, first_unimportant);

	return INFO_OK;
}


///////////////////////////////////////////////////////////////////////////////

/**
 * perform housekeeping (e.g. streaming); call once a frame.
 *
 * @param pos position support vector. if NULL, all parameters are
 * ignored and listener position unchanged; this is useful in case the
 * world isn't initialized yet.
 * @param dir view direction
 * @param up up vector
 * @return LibError
 */
LibError snd_update(const float * pos, const float * dir, const float * up)
{
	// there's no sense in updating anything if we weren't initialized
	// yet (most notably, if sound is disabled). we check for this to
	// avoid confusing the code below. the caller should complain if
	// this fails, so report success here (everything will work once
	// sound is re-enabled).
	if(!al_initialized)
		return INFO_OK;

	if(pos)
		al_listener_set_pos(pos, dir, up);

	vm_update();

	// for each source: add / remove buffers; carry out fading.
	snd_update_time = get_time();	// see decl
	list_foreach((void (*)(VSrc*))vsrc_update);

	return INFO_OK;
}


// prevent OpenAL from being initialized when snd_init is called.
static bool snd_disabled = false;

/**
 * extra layer on top of al_init that allows 'disabling' sound.
 * called from each snd_open.
 *
 * @return LibError from al_init, or ERR_AGAIN if sound disabled
 */
static inline LibError snd_init()
{
	// (note: each VSrc_reload and therefore snd_open will fail)
	if(snd_disabled)
		return ERR_AGAIN;	// NOWARN

	return al_init();
}


/**
 * (temporarily) disable all sound output. because it causes future snd_open
 * calls to immediately abort before they demand-initialize OpenAL,
 * startup is sped up considerably (500..1000ms). therefore, this must be
 * called before the first snd_open to have any effect; otherwise, the
 * cat will already be out of the bag and we debug_warn of it.
 *
 * rationale: this is a quick'n dirty way of speeding up startup during
 * development without having to change the game's sound code.
 *
 * can later be called to reactivate sound; all settings ever changed
 * will be applied and subsequent sound load / play requests will work.
 *
 * @param bool disabled
 * @return LibError
 */
LibError snd_disable(bool disabled)
{
	snd_disabled = disabled;

	if(snd_disabled)
	{
		if(al_initialized)
			debug_warn("already initialized => disable is pointless");
		return INFO_OK;
	}
	else
		return snd_init();
			// note: won't return ERR_AGAIN, since snd_disabled == false
}


/**
 * free all resources and shut down the sound system.
 * call before h_mgr_shutdown.
 */
void snd_shutdown()
{
	io_buf_shutdown();

	al_shutdown();
		// calls list_free_all
}
