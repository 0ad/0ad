#include "precompiled.h"

#include "res/res.h"
#include "res/snd.h"

#include <sstream>

#ifdef __APPLE__
# include <OpenAL/alut.h>
#else
# include <AL/al.h>
# include <AL/alc.h>
# include <AL/alut.h>
#endif

// Linux OpenAL puts the Ogg Vorbis extension enums in alexttypes.h
#ifdef OS_LINUX
# include <AL/alexttypes.h>
#endif

// for DLL-load hack in alc_init
#ifdef _WIN32
#include "sysdep/win/win_internal.h"
#endif



#ifdef _MSC_VER
#pragma comment(lib, "openal32.lib")
#pragma comment(lib, "alut.lib")
#endif




#include "timer.h"

static bool al_initialized = false;

static const char* alc_dev_name = "MMSYSTEM";

//static const char* alc_dev_name = 0;
	// default: use OpenAL default device.



static float listener_pos[3];


// rationale for "buffer" handle and separate handle for each sound instance:
// - need access to sound instances to fade them out / loop for an unknown period
// - access via handle for safety
// - don't want to reload sound data every time => need one central instance
//   that owns the data
// - want to support normal reload mechanism (for consistency if not necessity)
// - could hack something via h_find / if so, create new handle with fn_key = 0,
//   but that would break reloading and is dodgy. we will create a new handle
//   type instead.



static void al_check()
{
	assert(al_initialized);

	ALenum err = alGetError();
	if(err == AL_NO_ERROR)
		return;

	const char* str = (const char*)alGetString(err);
	debug_out("openal error: %s", str);
	debug_warn("OpenAL error");
}


//
// buf 'suballocator': can just return buffers directly,
// since allocation overhead is very low. however, this is useful for checking
// if all buffers have been freed, and a bit more convenient for the caller.
//

static int al_bufs_outstanding;

// convenience function (this is called from 2 places)
// buffer allocation overhead is very low -> no suballocator needed
static ALuint al_buf_alloc(ALvoid* data, ALsizei size, ALenum al_fmt, ALsizei al_freq)
{
debug_out("buf alloc\n");
	ALuint al_buf;
	alGenBuffers(1, &al_buf);
	alBufferData(al_buf, al_fmt, data, size, al_freq);
	al_check();
	al_bufs_outstanding++;
	return al_buf;
}


static void al_buf_free(ALuint al_buf)
{
debug_out("buf free\n");
	assert(alIsBuffer(al_buf));
//	alDeleteBuffers(1, &al_buf);
	al_check();
	al_bufs_outstanding--;
}


static void al_buf_shutdown()
{
	assert(al_bufs_outstanding == 0);
}



///////////////////////////////////////////////////////////////////////////////
//
// AL source suballocator: allocate all available sources up-front and
// pass them out as needed (alGenSources is quite slow, taking 3..5 ms per
// source returned). also responsible for enforcing user-specified limit
// on total number of sources (to reduce mixing cost on low-end systems).
//
///////////////////////////////////////////////////////////////////////////////

// stack
static const int AL_SRC_MAX = 64;
	// regardless of sound card caps, we won't use more than this
	// (64 is just overkill).
static ALuint al_srcs[AL_SRC_MAX];
static int al_src_used = 0;
static int al_src_cap = AL_SRC_MAX;
	// user-set limit on how many sources may be used
static int al_src_allocated;
	// how many valid sources in al_srcs we got


// called from al_init.
static void al_src_init()
{
	// grab as many sources as possible and count how many we get.
	for(int i = 0; i < AL_SRC_MAX; i++)
	{
		alGenSources(1, &al_srcs[i]);
		// we've reached the limit, no more are available.
		if(alGetError() != AL_NO_ERROR)
			break;
		assert(alIsSource(al_srcs[i]));
		al_src_allocated++;
debug_out("list src %p\n", al_srcs[i]);
	}

	// limit user's cap to what we actually got.
	if(al_src_cap > al_src_allocated)
		al_src_cap = al_src_allocated;

	// make sure we got the minimum guaranteed by OpenAL.
	assert(al_src_allocated >= 16);
}


// release all sources on freelist (currently stack).
// all sources must have been returned to us via al_src_free.
// called from al_shutdown.
static void al_src_shutdown()
{
	assert(al_src_used == 0);
	alDeleteSources(al_src_allocated, al_srcs);
	al_check();
}


static ALuint al_src_alloc()
{
	// no more to give
	if(al_src_used >= al_src_cap)
		return 0;
	ALuint al_src = al_srcs[al_src_used++];
	return al_src;
}


static void al_src_free(ALuint al_src)
{
	assert(alIsSource(al_src));
	al_srcs[--al_src_used] = al_src;
	assert(0 <= al_src_used && al_src_used < al_src_allocated);
		// don't compare against cap - it might have been
		// decreased to less than were in use.
}


int snd_set_max_src(int cap)
{
	// non-positive - bogus.
	if(cap <= 0)
	{
		debug_warn("snd_set_max_src: cap <= 0");
		return -1;
	}

	// either cap is legit (less than what we allocated in al_src_init),
	// or al_src_init wasn't called yet. note: we accept anything in the
	// second case, as al_src_init will sanity-check al_src_cap.
	if(!al_src_allocated || cap < al_src_allocated)
	{
		al_src_cap = cap;
		return 0;
	}
	// user is requesting a cap higher than what we actually allocated.
	// that's fine (not an error), but we won't set the cap, since it
	// determines how many sources may be returned.
	else
		return -1;
}


///////////////////////////////////////////////////////////////////////////////
//
// OpenAL init / shutdown
//
///////////////////////////////////////////////////////////////////////////////

// called as late as possible, i.e. the first time sound/music is played
// (either from module init there, or from the play routine itself).
// this delays library load, leading to faster perceived app startup.
// registers an atexit routine for cleanup.
// no harm if called more than once.

static ALCcontext* alc_ctx;
static ALCdevice* alc_dev;


static void alc_shutdown()
{
	alcMakeContextCurrent(0);
	alcDestroyContext(alc_ctx);
	alcCloseDevice(alc_dev);
}


static int alc_init()
{
	int ret = 0;

	// HACK: OpenAL loads and unloads these DLLs several times on Windows.
	// we hold a reference to prevent the actual unload, thus speeding up
	// sound startup by 100..400 ms. everything works ATM;
	// hopefully, OpenAL doesn't rely on them actually being unloaded.
#ifdef _WIN32
	HMODULE dlls[3];
	dlls[0] = LoadLibrary("wrap_oal.dll");
	dlls[1] = LoadLibrary("setupapi.dll");
	dlls[2] = LoadLibrary("wdmaud.drv");
#endif

	alc_dev = alcOpenDevice((ALubyte*)alc_dev_name);
	if(alc_dev)
	{
		alc_ctx = alcCreateContext(alc_dev, 0);	// no attrlist needed
		if(alc_ctx)
			alcMakeContextCurrent(alc_ctx);
	}

	ALCenum err = alcGetError(alc_dev);
	if(err != ALC_NO_ERROR || !alc_dev || !alc_ctx)
	{
		debug_out("alc_init failed. alc_dev=%p alc_ctx=%p err=%d", alc_dev, alc_ctx, err);
		ret = -1;
	}

	// release DLL references, so BoundsChecker doesn't complain at exit
#ifdef _WIN32
	for(int i = 0; i < ARRAY_SIZE(dlls); i++)
		if(dlls[i] != INVALID_HANDLE_VALUE)
			FreeLibrary(dlls[i]);
#endif

	return ret;
}





// called from each sound_open, and from snd_dev_set
static int al_init()
{
	// only take action on first call, OR after snd_dev_set calls us again
	if(al_initialized)
		return 0;
	al_initialized = true;

	CHECK_ERR(alc_init());
	al_src_init();	// can't fail

	return 0;
}


static void al_shutdown()
{
	al_src_shutdown();
	al_buf_shutdown();
	alc_shutdown();

	al_initialized = false;
}


// if OpenAL hasn't been initialized yet, we only remember the device
// name, which will be set when alc_init is later called; otherwise,
// OpenAL is reinitialized to use the desired device.
// (this is to speed up the common case of retrieving a device name from
// config files and setting it; OpenAL doesn't have to be loaded until
// sounds are actually played).
// return 0 to indicate success, or the status returned while initializing
// OpenAL.
static int al_reinit()
{
	if(!al_initialized)
		return 0;

	// was already using another device; now re-init
	// (stops all currently playing sounds)
	return al_init();
}


///////////////////////////////////////////////////////////////////////////////
//
// device enumeration: list all devices and allow the user to choose one,
// in case the default device has problems.
//
///////////////////////////////////////////////////////////////////////////////

static const char* devs;
	// set by snd_dev_prepare_enum; used by snd_dev_next.
	// consists of back-to-back C strings, terminated by an extra '\0'.
	// (this is taken straight from OpenAL; dox say this format may change).

// prepare to enumerate all device names (this resets the list returned by
// snd_dev_next). return 0 on success, otherwise -1 (only if the requisite
// OpenAL extension isn't available). on failure, a "cannot enum device"
// message should be presented to the user, and snd_dev_set need not be
// called; OpenAL will use its default device.
// may be called each time the device list is needed.
int snd_dev_prepare_enum()
{
	if(alcIsExtensionPresent(0, (ALubyte*)"ALC_ENUMERATION_EXT") != AL_TRUE)
		return -1;

	devs = (const char*)alcGetString(0, ALC_DEVICE_SPECIFIER);
	return 0;
}


// return the next device name, or 0 if all have been returned.
// do not call unless snd_dev_prepare_enum succeeded!
// not thread-safe! (static data from snd_dev_prepare_enum is used)
const char* snd_dev_next()
{
	if(!*devs)
		return 0;
	const char* dev = devs;
	devs += strlen(dev)+1;
	return dev;
}


// tell OpenAL to use the specified device (0 to revert to default) in future.
//
// if OpenAL hasn't been initialized yet, we only remember the device
// name, which will be set when snd_init is later called; otherwise,
// OpenAL is reinitialized to use the desired device (thus stopping all
// active sounds). we go to this trouble to speed up perceived load times:
// OpenAL doesn't need to be loaded until sounds are actually played.
//
// return 0 on success, or the status returned while re-initializing OpenAL.
int snd_dev_set(const char* new_alc_dev_name)
{
	// requesting a specific device
	if(new_alc_dev_name)
	{
		// already using that device - done
		if(alc_dev_name && !strcmp(alc_dev_name, new_alc_dev_name))
			return 0;

		// store name (need to copy it, since we snd_init later,
		// and it must then still be valid)
		static char buf[32];
		strncpy(buf, new_alc_dev_name, 32-1);
		alc_dev_name = buf;
	}
	// requesting default device
	else
	{
		// already using default device - done
		if(alc_dev_name == 0)
			return 0;

		alc_dev_name = 0;
	}

	return al_reinit();
}


///////////////////////////////////////////////////////////////////////////////
//
// streams
//
///////////////////////////////////////////////////////////////////////////////

// rationale: could make a case for a separate IO layer, but there's a
// problem: IOs need to be discarded after their data has been processed.
// if the IO layer is separate, we'd either need a callback from
// io_complete, passing the buffer to OpenAL, or mark IO slots as
// "discardable", so that they are freed the next io_issue.
// both are ugly; we instead integrate IO into the sound data code.
// IOs are passed to OpenAL and the discarded immediately.
//
// having one IO-queue per sound data object is no problem:
// we suballocate buffers, and don't need centralized scheduling
// (there will be <= 2 streams active at once).


// one stream apiece for music and voiceover (narration during tutorial).
// allowing more is possible, but would be inefficent due to seek overhead.
// set this limit to catch questionable usage (e.g. streaming normal sounds).
static const int MAX_STREAMS = 2;

static const int MAX_IOS = 4;

static const int TOTAL_IOS = MAX_STREAMS * MAX_IOS;

static const size_t RAW_BUF_SIZE = 32*KB;


// suballocator, so that we aren't constantly allocating/freeing large buffers.
//
// note: snd_shutdown is called after h_mgr_shutdown,
// so all memory blocks will already have been freed.
// we don't want our alloc to show up as a leak,
// so we use malloc and align ourselves.

static void* io_bufs_raw;
	// raw, unaligned mem returned by malloc

static void* io_buf_freelist;

static void io_buf_init()
{
	const size_t align = 4*KB;
	io_bufs_raw = malloc(TOTAL_IOS*RAW_BUF_SIZE + align-1);
	void* bufs = (void*)round_up((uintptr_t)io_bufs_raw, align);
}


static void* io_buf_alloc()
{
	ONCE(io_buf_init());

	void* buf = io_buf_freelist;
	// note: we have to bail; can't update io_buf_freelist
	if(!buf)
	{
		debug_warn("io_buf_alloc: no memory or #streams exceeded");
		return 0;
	}

	io_buf_freelist = *(void**)io_buf_freelist;
	return buf;
}


static void io_buf_free(void* p)
{
	*(void**)p = io_buf_freelist;
	io_buf_freelist = p;
}


// no-op if io_buf_alloc was never called
static void io_buf_shutdown()
{
	free(io_bufs_raw);
}


///////////////////////////////////////////////////////////////////////////////
//
// sound data provider
//
///////////////////////////////////////////////////////////////////////////////

struct SndData
{
	bool stream;

	// clip
	ALuint al_buf;

	// stream
	Handle hf;
	ALenum al_fmt;
	ALsizei al_freq;
	Handle ios[MAX_IOS];
	int active_ios;
};

H_TYPE_DEFINE(SndData);


static int stream_issue(SndData* sd, Handle hf)
{
	if(sd->active_ios >= MAX_IOS)
		return 0;

return 0;
/*
	Handle h = vfs_stream(hf, RAW_BUF_SIZE, io->raw_buf);
	CHECK_ERR(h);
	sd->ios[sd->active_ios++] = h;
	return 0;
*/
}




static void SndData_init(SndData* sd, va_list args)
{
	sd->stream = va_arg(args, bool);
}

static void SndData_dtor(SndData* sd)
{
debug_out("snd_data dtor\n");
	if(sd->stream)
	{
		vfs_close(sd->hf);
	}
	else
	{
		al_buf_free(sd->al_buf);
	}
}


static int SndData_reload(SndData* sd, const char* fn, Handle)
{
	//
	// detect sound format by checking file extension
	//

	enum FileType
	{
		FT_WAV,
		FT_OGG
	}
	file_type;

	const char* ext = strrchr(fn, '.');
	// .. OGG (data will be passed directly to OpenAL)
	if(ext && !stricmp(ext, ".ogg"))
	{
		// first use of OGG: check if OpenAL extension is available.
		// note: this is required! OpenAL does its init here.
		static int ogg_supported = -1;
		if(ogg_supported == -1)
			ogg_supported = alIsExtensionPresent((ALubyte*)"AL_EXT_vorbis")? 1 : 0;
		if(!ogg_supported)
			return -1;

		sd->al_fmt  = AL_FORMAT_VORBIS_EXT;
		sd->al_freq = 0;

		file_type = FT_OGG;
	}
	// .. WAV
	else if(ext && !stricmp(ext, ".wav"))
		file_type = FT_WAV;
	// .. unknown extension
	else
		return -1;


	if(sd->stream)
	{
		// refuse to stream anything that cannot be passed directly to OpenAL -
		// we'd have to extract the audio data ourselves (not worth it).
		if(file_type != FT_OGG)
			return -1;

		sd->hf = vfs_open(fn);
		CHECK_ERR(sd->hf);

		for(int i = 0; i < MAX_IOS; i++)
			CHECK_ERR(stream_issue(sd, sd->hf));
	}
	// clip
	else
	{
		void* file;
		size_t file_size;
		CHECK_ERR(vfs_load(fn, file, file_size));

		ALvoid* al_data = file;
		ALsizei al_size = (ALsizei)file_size;

		if(file_type == FT_WAV)
		{
			ALbyte* memory = (ALbyte*)file;
			ALboolean al_loop;	// unused
			alutLoadWAVMemory(memory, &sd->al_fmt, &al_data, &al_size, &sd->al_freq, &al_loop);
		}

		sd->al_buf = al_buf_alloc(al_data, al_size, sd->al_fmt, sd->al_freq);

		mem_free(file);
	}

	return 0;
}


// open and return a handle to a sound file's data
static Handle snd_data_load(const char* const fn, const bool stream)
{
	// make sure we don't reload a stream object
	uint flags = 0;
	if(stream)
		flags = RES_UNIQUE;

	return h_alloc(H_SndData, fn, flags, stream);
}


// close the sound file data <hsd> and set hsd to 0.
static int snd_data_free(Handle& hsd)
{
	return h_free(hsd, H_SndData);
}


enum BufRet
{
	// buffer has been returned; barring errors, more will be available.
	BUF_OK = 0,

	// this was the last buffer we will return (end of file reached).
	BUF_EOF = 1,

	// no buffer returned - still streaming in ATM. call again later.
	BUF_AGAIN = 2,

	// anything else: negative error code
};

static int snd_data_get_buf(Handle hsd, ALuint& al_buf)
{
	// in case H_DEREF fails
	al_buf = 0;

	H_DEREF(hsd, SndData, sd);

	// clip: just return buffer (which was created in snd_data_load)
	if(sd->al_buf)
	{
		al_buf = sd->al_buf;
		return BUF_EOF;
	}

	//
	// stream: check if IO finished, issue next, return the completed buffer
	//

	assert(sd->active_ios);

	// has it finished? if not, bail
	Handle hio = sd->ios[0];
	int is_complete = vfs_io_complete(hio);
	CHECK_ERR(is_complete);
	if(is_complete == 0)
		return BUF_AGAIN;

	// get its buffer
	void* data;
	size_t size;
	CHECK_ERR(vfs_wait_io(hio, data, size));
		// returns immediately, since vfs_io_complete == 1

	al_buf = al_buf_alloc(data, (ALsizei)size, sd->al_fmt, sd->al_freq);

	// free IO slot
	int err = vfs_discard_io(hio);
	if(err < 0)
	{
		al_buf_free(al_buf);
		return err;
	}

	// we implement the required 'circular queue' as a stack;
	// have to shift all items after this one down.
	sd->active_ios--;
	for(int i = 0; i < sd->active_ios; i++)
		sd->ios[i] = sd->ios[i+1];

	// try to issue the next IO. if EOF reached, indicate al_buf is the last.
	err = stream_issue(sd, sd->hf);
	if(err == ERR_EOF)
		return BUF_EOF;

	// al_buf valid and next IO issued successfully.
	return BUF_OK;
}


static int snd_data_free_buf(Handle hsd, ALuint al_buf)
{
	H_DEREF(hsd, SndData, sd);

	// clip: no-op (caller will later release hsd reference;
	// it won't actually be freed until exit, because it's cached).
	if(!sd->stream)
		return 0;

	al_buf_free(al_buf);
	return 0;
}


///////////////////////////////////////////////////////////////////////////////
//
// sound instance
//
///////////////////////////////////////////////////////////////////////////////

struct VSrc
{
	ALuint al_src;

	// handle to this VSrc, so that it can close itself
	Handle hvs;

	// associated sound data
	Handle hsd;

	// - can't have 2 active instances of a streamed sound, so make sure
	//   caller is aware of the limitation by requiring them to set this.
	bool stream;

	bool eof;
	bool closing;

	ALfloat pos[3];
	ALfloat gain;
	ALboolean loop;
	ALboolean relative;

	float static_pri;
	float cur_pri;
};

H_TYPE_DEFINE(VSrc);






static int vsrc_update(VSrc* vs)
{
ALint _num_queued;
alGetSourcei(vs->al_src, AL_BUFFERS_QUEUED, &_num_queued);


	// remove all finished buffers
	int num_processed;
	alGetSourcei(vs->al_src, AL_BUFFERS_PROCESSED, &num_processed);
debug_out("%g: vs=%p  hvs=%I64x   q=%d p=%d\n", get_time(), vs, vs->hvs, _num_queued, num_processed);
	for(int i = 0; i < num_processed;  i++)
	{
		ALuint al_buf;
		alSourceUnqueueBuffers(vs->al_src, 1, &al_buf);
debug_out("removing processed buf=%p vs=%p src=%p   hvs=%I64x\n", al_buf, vs, vs->al_src, vs->hvs);
		CHECK_ERR(snd_data_free_buf(vs->hsd, al_buf));
	}

if(vs->closing)
return 0;

	// no more buffers left, and EOF reached
	ALint num_queued;
	alGetSourcei(vs->al_src, AL_BUFFERS_QUEUED, &num_queued);
	al_check();

	if(num_queued == 0 && vs->eof)
	{
Handle tmp = vs->hvs;
debug_out("%g: reached end, closing vs=%p src=%p    hvs=%I64x\n", get_time(), vs, vs->al_src, vs->hvs);
		snd_free(/*vs->hvs*/tmp);
		return 0;
	}

	if(!vs->eof)
	{
		int to_fill = 4;
		if(num_queued > 0)
			to_fill = num_processed;

		int ret;
		do
		{
			ALuint al_buf;
			ret = snd_data_get_buf(vs->hsd, al_buf);
			CHECK_ERR(ret);

debug_out("%g: got buf: buf=%p vs=%p src=%p   hvs=%I64x\n", get_time(), al_buf, vs, vs->al_src, vs->hvs);

			alSourceQueueBuffers(vs->al_src, 1, &al_buf);
			al_check();
		}
		while(to_fill-- && ret == BUF_OK);

		if(ret == BUF_EOF)
		{
			vs->eof = true;
debug_out("EOF reported for vs=%p src=%p   hvs=%I64x\n", vs, vs->al_src, vs->hvs);
		}
	}

	return 0;
}


static int vsrc_grant_src(VSrc* vs)
{
debug_out("grant vs=%p src=%p\n", vs, vs->al_src);

	// already playing - bail
	if(vs->al_src)
		return 0;

	// try to alloc source
	vs->al_src = al_src_alloc();
	// called from 2 places: sound_play can't know if a source is available,
	// so this isn't an error
	if(!vs->al_src)
	{
debug_out("grant couldn't alloc src!\n");
		return -1;
	}

	// OpenAL docs don't specify default values, so initialize everything
	// ourselves to be sure. note: alSourcefv param is not const.
	float zero3[3] = { 0.0f, 0.0f, 0.0f };
	alSourcefv(vs->al_src, AL_VELOCITY, zero3);
	alSourcefv(vs->al_src, AL_DIRECTION, zero3);
	alSourcef(vs->al_src, AL_ROLLOFF_FACTOR, 0.0f);
	alSourcei(vs->al_src, AL_SOURCE_RELATIVE, AL_TRUE);
	al_check();

	// we only now got a source, so latch previous settings
	// don't use snd_set*; this way is easiest
	alSourcef(vs->al_src, AL_GAIN, vs->gain);
	alSourcefv(vs->al_src, AL_POSITION, vs->pos);
	alSourcei(vs->al_src, AL_LOOPING, vs->loop);

	CHECK_ERR(vsrc_update(vs));

debug_out("play vs=%p src=%p    hvs=%I64x\n", vs, vs->al_src, vs->hvs);
	alSourcePlay(vs->al_src);
	al_check();
	return 0;
}


static int vsrc_reclaim_src(VSrc* vs)
{
debug_out("reclaim vs=%p src=%p\n", vs, vs->al_src);

	// not playing - bail
	if(!vs->al_src)
		return 0;

debug_out("stop\n");
	alSourceStop(vs->al_src);
	al_check();

vs->closing = true;
	CHECK_ERR(vsrc_update(vs));
	// (note: all buffers are now considered 'processed', since src is stopped)

	al_src_free(vs->al_src);
	return 0;
}






static void list_remove(VSrc*);


static void VSrc_init(VSrc* vs, va_list args)
{
	vs->stream = va_arg(args, bool);
}


static void VSrc_dtor(VSrc* vs)
{
debug_out("vsrc_dtor vs=%p\n    hvs=%I64x\n", vs, vs->hvs);
	list_remove(vs);
	vsrc_reclaim_src(vs);
	snd_data_free(vs->hsd);
}


static int VSrc_reload(VSrc* vs, const char* def_fn, Handle hvs)
{
	// cannot wait till play(), need to init here:
	// must load OpenAL so that snd_data_load can check for OGG extension.
	al_init();

/*
	void* def_file;
	size_t def_size;
	CHECK_ERR(vfs_load(def_fn, def_file, def_size));
	std::istringstream def(std::string((char*)def_file, (int)def_size));
	mem_free(def_file);

	std::string snd_data_fn;
	float gain_percent;
	def >> snd_data_fn;
	def >> gain_percent;*/

float gain_percent = 100.0;
std::string snd_data_fn = def_fn;

	vs->gain = gain_percent / 100.0f;

	// store so we can shut ourselves down via snd_free when done playing
	vs->hvs = hvs;

	vs->hsd = snd_data_load(snd_data_fn.c_str(), vs->stream);

	return 0;
}


// open and return a handle to the sound <fn>.
// stream: default false
Handle snd_open(const char* const fn, const bool stream)
{
static int seq;
if(++seq == 2)
seq = 2;

	return h_alloc(H_VSrc, fn, RES_UNIQUE, stream);
}


// close the sound <hs> and set hs to 0.
int snd_free(Handle& hvs)
{
	return h_free(hvs, H_VSrc);
}


int snd_set_pos(Handle hvs, float x, float y, float z, bool relative /* = false */)
{
	H_DEREF(hvs, VSrc, vs);

	vs->pos[0] = x; vs->pos[1] = y; vs->pos[2] = z;
	vs->relative = relative;

	if(vs->al_src)
	{
		alSourcefv(vs->al_src, AL_POSITION, vs->pos);
		alSourcei(vs->al_src, AL_SOURCE_RELATIVE, vs->relative);
		al_check();
	}

	return 0;
}


int snd_set_gain(Handle hvs, float gain)
{
	H_DEREF(hvs, VSrc, vs);

	vs->gain = gain;

	if(vs->al_src)
	{
		alSourcef(vs->al_src, AL_GAIN, vs->gain);
		al_check();
	}

	return 0;
}


int snd_set_loop(Handle hvs, bool loop)
{
	H_DEREF(hvs, VSrc, vs);

	vs->loop = loop;

	if(vs->al_src)
	{
		alSourcei(vs->al_src, AL_LOOPING, vs->loop);
		al_check();
	}

	return 0;
}








static float norm(const float* v)
{
	return v[0]*v[0] + v[1]*v[1] + v[2]*v[2];
}

static float dist3d_sqr(const float* v1, const float* v2)
{
	const float dx = v1[0] - v2[0];
	const float dy = v1[1] - v2[1];
	const float dz = v1[2] - v2[2];
	return dx*dx + dy*dy + dz*dz;
}


const float MAX_DIST2 = 1000.0f;

static void vsrc_calc_cur_pri(VSrc* vs)
{
	float d2;	// euclidean distance to listener (squared)
	if(vs->relative)
		d2 = norm(vs->pos);
	else
		d2 = dist3d_sqr(vs->pos, listener_pos);

	// farther away than OpenAL cutoff - no sound contribution
	if(d2 > MAX_DIST2)
	{
		vs->cur_pri = -1.0f;
		return;

		// TODO: make sure these never play, even if no sounds active
	}

	// scale priority down exponentially
	float e = d2 / MAX_DIST2;	// 0.0f (close) .. 1.0f (far)
	const float falloff = 10.0f;
	vs->cur_pri = vs->static_pri * pow(falloff, e);
}


static bool vsrc_greater(const VSrc* const s1, const VSrc* const s2)
{
	return s1->cur_pri > s2->cur_pri;
}





///////////////////////////////////////////////////////////////////////////////
//
// list of sounds
//
///////////////////////////////////////////////////////////////////////////////

// currently active sounds - needed to update them, i.e. remove old buffers
// and enqueue just finished async buffers. this can't happen from
// io_check_complete alone - see dox there.


// sorted in ascending order of current priority
// (we remove low pri items, and have to move down everything after them,
// so they should come last)
typedef std::vector<VSrc*> VSources;
typedef VSources::iterator It;
static VSources vsources;

// don't need to sort - that's done during full update
static void list_add(VSrc* vs)
{
	vsources.push_back(vs);
}

static void list_foreach(void(*cb)(VSrc*))
{
	// can't use std::for_each: some entries may have been deleted
	// (i.e. set to 0) since last update.
	for(It it = vsources.begin(); it != vsources.end(); ++it)
	{
		VSrc* vs = *it;
		if(vs)
			cb(vs);
	}
}


// O(N)!
//
// TODO: replace with last list entry, resize -1
//

static void list_remove(VSrc* vs)
{
debug_out("list_remove vs=%p\n", vs);

	for(size_t i = 0; i < vsources.size(); i++)
		if(vsources[i] == vs)
		{
			vsources[i] = 0;
			return;
		}

//	debug_warn("list_remove: VSrc not found");
debug_out("NOT FOUND!\n");
}

static bool vsrc_is_null(VSrc* vs)
{
	return vs != 0;
}


static int list_update()
{
	// prune NULL-entries, so code below doesn't have to check if non-NULL
	// (these were removed, but we didn't shuffle everything down to save time)
	It new_end = remove_if(vsources.begin(), vsources.end(), vsrc_is_null);
	vsources.erase(new_end, vsources.end());

	// update current priorities (a function of static priority and distance)
	std::for_each(vsources.begin(), vsources.end(), vsrc_calc_cur_pri);

	// sort by descending current priority
	std::sort(vsources.begin(), vsources.end(), vsrc_greater);

	It it;
	It first_unimportant = vsources.begin() + min((int)vsources.size(), al_src_cap); 

	// reclaim source from the less important vsources
	for(it = first_unimportant; it != vsources.end(); ++it)
	{
		VSrc* vs = *it;
		if(vs->al_src)
		{
debug_out("reclaiming from low-pri\n");
			vsrc_reclaim_src(vs);
		}

		if(!vs->loop)
		{
debug_out("kicking out low-pri\n");
Handle tmp = vs->hvs;
			snd_free(/*vs->hvs*/tmp);
		}
	}

	// grant each of the most important vsources a source
	for(it = vsources.begin(); it != first_unimportant; ++it)
	{
		VSrc* vs = *it;
		if(!vs->al_src)
		{
debug_out("now granting high-pri a new src\n");
			vsrc_grant_src(vs);
		}
	}


	std::for_each(vsources.begin(), vsources.end(), vsrc_update);

	return 0;
}


int snd_play(Handle hs)
{
debug_out("snd_play\n");
	H_DEREF(hs, VSrc, vs);

	list_add(vs);

	// optimization (don't want to do full update here - too slow)
	// either we get a source and playing begins immediately, or it'll be
	// taken care of on next update
	vsrc_grant_src(vs);
	return 0;
}





///////////////////////////////////////////////////////////////////////////////
//
// sound engine
//
///////////////////////////////////////////////////////////////////////////////



static void vsrc_free(VSrc* vs)
{
Handle tmp = vs->hvs;
	snd_free(/*vs->hvs*/tmp);
}

int snd_update(float listener_orientation[16])
{
	int i;

	float* pos = &listener_orientation[12];
	float* in  = &listener_orientation[8];
	float* up  = &listener_orientation[4];

	for(i = 0; i < 3; i++)
		listener_pos[i] = pos[i];

	float al_orientation[6];
	for(i = 0; i < 3; i++)
		al_orientation[i] = in[i];
	for(i = 0; i < 3; i++)
		al_orientation[3+i] = up[i];


	alListenerfv(AL_POSITION, pos);
	alListenerfv(AL_ORIENTATION, al_orientation);

	list_update();

//	CHECK_ERR(io_check_complete());

	return 0;
}


void snd_shutdown()
{
debug_out("snd_shutdown\n");
	list_foreach(vsrc_free);

	io_buf_shutdown();

	al_shutdown();
}
