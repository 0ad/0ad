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








static const char* alc_dev_name = 0;
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
	const char* str = 0;
	switch(alGetError())
	{
	case AL_NO_ERROR:
		return;
#define ERR(name) case name: str = #name; break;
		ERR(AL_INVALID_NAME);
		ERR(AL_INVALID_ENUM);
		ERR(AL_INVALID_VALUE);
		ERR(AL_INVALID_OPERATION);
		ERR(AL_OUT_OF_MEMORY);
	}

	debug_out("openal error: %s", str);
	debug_warn("OpenAL error");
}



// convenience function (this is called from 2 places)
// buffer allocation overhead is very low -> no suballocator needed
static ALuint al_create_buffer(ALvoid* data, ALsizei size, ALenum al_fmt, ALsizei al_freq)
{
	ALuint al_buf;
	alGenBuffers(1, &al_buf);
	alBufferData(al_buf, al_fmt, data, size, al_freq);
	al_check();
	return al_buf;
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
debug_out("init %p\n", al_srcs[i]);
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
}


static ALuint al_src_alloc()
{
	// no more to give
	if(al_src_used >= al_src_cap)
		return 0;
	ALuint al_src = al_srcs[al_src_used++];
	assert(alIsSource(al_src));
	return al_src;
}


static void al_src_free(ALuint al_src)
{
	al_srcs[--al_src_used] = al_src;
	assert(0 <= al_src_used && al_src_used < al_src_cap);
}


int snd_set_max_src(int cap)
{
	// non-positive - bogus.
	if(cap <= 0)
	{
		debug_warn("snd_set_max_src: cap <= 0");
		return -1;
	}

	// cap must be at least as much as we're currently using.
	if(cap < al_src_used)
		cap = al_src_used;

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


static bool al_initialized = false;


// called from each sound_open, and from snd_dev_set
static int al_init()
{
	// only take action on first call, OR after snd_dev_set calls us again
	if(al_initialized)
		return 0;
	al_initialized = true;

	ONCE(atexit(alc_shutdown));
		// we might init OpenAL several times, but must register
		// the atexit function only once!

	CHECK_ERR(alc_init());
	al_src_init();	// can't fail

	return 0;
}


static void al_shutdown()
{
	al_initialized = false;

	alc_shutdown();
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
// OpenAL extension isn't available). on failure, a "cannot change device"
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


// tell OpenAL to use the specified device (0 for default) in future.
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
// IO for streams
//
///////////////////////////////////////////////////////////////////////////////

// one stream apiece for music and voiceover (narration during tutorial).
// allowing more is possible, but would be inefficent due to seek overhead.
// set this limit to catch questionable usage (e.g. streaming normal sounds).
static const int MAX_STREAMS = 2;

static const int MIN_BUFFERS = 2;
static const int MAX_IOS = 4;


static const int TOTAL_IOS = MAX_STREAMS * MAX_IOS;

static const size_t RAW_BUF_SIZE = 32*KB;
static void* raw_buf;

struct VSrc;

struct IO
{
	Handle hio;
	void* raw_buf;

	// 
	void* vs;

	// so we can issue the next
};

// advantage over per-stream queue: we don't have to realloc a buffer
// for every IO issue.

static IO ios[TOTAL_IOS];
static uint next_issue;
static uint next_complete;
static uint available_ios = TOTAL_IOS;

static int io_init_ios()
{
	const size_t total_size = RAW_BUF_SIZE * TOTAL_IOS;
	raw_buf = mem_alloc(total_size, 4096);
	if(!raw_buf)
		return ERR_NO_MEM;

	char* p = (char*)raw_buf;
	for(int i = 0; i < TOTAL_IOS; i++)
	{
		ios[i].raw_buf = p;
		p += RAW_BUF_SIZE;
	}
}


static int io_free_ios()
{
	// !wait for all IOs to terminate; do not issue new ones!

	mem_free(raw_buf);
	memset(ios, 0, sizeof(ios));
}


static int io_issue(VSrc* vs, Handle hf)
{
	if(!available_ios)
		return 0;

	IO* io = &ios[next_issue];

	//	slot->hio = vfs_stream(hf, RAW_BUF_SIZE, slot->raw_buf);
	CHECK_ERR(io->hio);

	next_issue = (next_issue + 1) % TOTAL_IOS;
	available_ios--;

	return 0;
}


/*
static int sound_add_buffer(VSrc* vs, void* buf, size_t size);
static int sound_update(VSrc* vs);
*/

// error return value checked by snd_update
static int io_check_complete()
{
	for(;;)
	{
		// ring buffer is empty; nothing to wait on
		if(next_complete == next_issue)
			break;

		IO* slot = &ios[next_complete];

		// check if first IO is finished; if not, bail.
		int is_complete = vfs_io_complete(slot->hio);
		CHECK_ERR(is_complete);
		if(is_complete == 0)
			break;
		next_complete = (next_complete+1) % TOTAL_IOS;

		void* buf;
		size_t size;
		CHECK_ERR(vfs_wait_io(slot->hio, buf, size));
		// returns immediately

		// rationale: could issue right after we determine an IO is complete -
		// it might take a while for OpenAL to copy the buffer, and we could
		// start transferring in that time. however, there will be enough IOs
		// in-flight to cover long intervals between calls, so this isn't a
		// problem. it would also require more slots, since we wouldn't have
		// discarded the IO slot yet. finally, we want to limit issues to the
		// actual rate of data consumption - the IO shouldn't run ahead.
		// therefore, issue from sound_update for each finished buffer.

		//		CHECK_ERR(sound_add_buffer(slot->s, buf, size));

		CHECK_ERR(vfs_discard_io(slot->hio));
	}

	return 0;
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
	int queued_buffers;
};

H_TYPE_DEFINE(SndData);


static void SndData_init(SndData* sd, va_list args)
{
	sd->stream = va_arg(args, bool);
}

static void SndData_dtor(SndData* sd)
{
	alDeleteBuffers(1, &sd->al_buf);
	al_check();

	vfs_close(sd->hf);
}


static int SndData_reload(SndData* sd, const char* fn, Handle)
{
	int ret = -1;

	if(sd->stream)
	{
		sd->hf = vfs_open(fn);
		CHECK_ERR(sd->hf);

//		for(int i = 0; i < MAX_IOS; i++)
//			CHECK_ERR(io_issue(s, vs->hf));
	}
	else
	{
	}



	// freed in bailout ladder
	void* file = 0;

	{

	size_t file_size;
	CHECK_ERR(vfs_load(fn, file, file_size));


	//
	// detect sound format by checking file extension
	//

	ALvoid* al_data;
	ALsizei al_size;

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
		{
			ret = -1;
			goto fail;
		}

		al_data = file;
		al_size = (ALsizei)file_size;
		sd->al_fmt  = AL_FORMAT_VORBIS_EXT;
		sd->al_freq = 0;
	}
	// .. WAV
	else if(ext && !stricmp(ext, ".wav"))
	{
		ALbyte* memory = (ALbyte*)file;
		ALboolean al_loop;	// unused
		alutLoadWAVMemory(memory, &sd->al_fmt, &al_data, &al_size, &sd->al_freq, &al_loop);
	}
	// .. unknown extension
	else
	{
		ret = -1;
		goto fail;
	}

	sd->al_buf = al_create_buffer(al_data, al_size, sd->al_fmt, sd->al_freq);

	ret = 0;
fail:
	mem_free(file);

	}

	return ret;
}


// open and return a handle to a sound file's data
static Handle snd_data_load(const char* const fn, const bool stream)
{
	//
//
	//
//
	//		TODO: unique when stream - no reload
//
	//
//
	//
uint flags = 0;
	return h_alloc(H_SndData, fn, flags, stream);
}


// close the xxx <y> and set y to 0.
static int snd_data_free(Handle& hsd)
{
	return h_free(hsd, H_SndData);
}


// snd_data_get_buf return value
enum BufRet
{
	BUF_OK = 0,
	BUF_EOF = 1,
	// otherwise, a negative error code.
};

static int snd_data_get_buf(Handle hsd, ALuint& al_buf)
{
	H_DEREF(hsd, SndData, sd);

	// clip: just return buffer (which was created in snd_data_load)
	if(sd->al_buf)
	{
		al_buf = sd->al_buf;
		return BUF_OK;
	}

	return -1;
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
	// - data layer can't know if src is asking for a clip's buffer the first
	//   time, or whether "EOF" (clip done). src must not ask for second
	//   buffer if stream = true
	//
	// alternatives:
	// - pass source info down to snd_data. increased coupling, bad interface
	// - if src gets same buffer returned as last call by snd_data_get_buf,
	//   assume clip EOF. not watertight! buffer may change during OpenAL
	//   reinit (e.g. if changing provider)
	bool stream;

	ALfloat pos[3];
	ALfloat gain;
	ALboolean loop;
	ALboolean relative;

	float static_pri;
	float cur_pri;
};

H_TYPE_DEFINE(VSrc);






static int vsrc_grant_src(VSrc* vs)
{
	// already playing - bail
	if(vs->al_src)
		return 0;

	// try to alloc source
	vs->al_src = al_src_alloc();
	// called from 2 places: sound_play can't know if a source is available,
	// so this isn't an error
	if(!vs->al_src)
		return -1;

debug_out("got  %p\n", vs->al_src);

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

	ALuint al_buf;
	int ret = snd_data_get_buf(vs->hsd, al_buf);
	if(ret == BUF_OK)
		alSourceQueueBuffers(vs->al_src, 1, &al_buf);
	else
		debug_warn("snd_data_get_buf failed");
	al_check();

	alSourcePlay(vs->al_src);
	al_check();
	return 0;
}


static int vsrc_reclaim_src(VSrc* vs)
{
	// not playing - bail
	if(!vs->al_src)
		return 0;

	alSourceStop(vs->al_src);

	// free all buffers
	if(vs->stream)
	{
		int num_bufs;
		alGetSourcei(vs->al_src, AL_BUFFERS_PROCESSED, &num_bufs);
		// all are considered processed, as the source has been stopped
		for(int i = 0; i < num_bufs;  i++)
		{
			ALuint al_buf;
			alSourceUnqueueBuffers(vs->al_src, 1, &al_buf);
			alDeleteBuffers(1, &al_buf);
		}
	}

	al_check();

debug_out("free %p\n", vs->al_src);
	al_src_free(vs->al_src);
	return 0;
}


static void vsrc_update(VSrc* vs)
{
	ALint num_finished_buffers;
	alGetSourcei(vs->al_src, AL_BUFFERS_PROCESSED, &num_finished_buffers);
	al_check();



	if(!vs->stream)
	{
		if(num_finished_buffers == 1)
			snd_free(vs->hvs);
	}
/*

	if(vs->flags & SF_STREAMING)
		for(int i = 0; i < num_finished_buffers; i++)
		{
			ALuint al_buf;
			alSourceUnqueueBuffers(vs->al_src, 1, &al_buf);
			alDeleteBuffers(1, &al_buf);
			al_check();

			CHECK_ERR(io_issue(s, vs->hf));
		}
*/
}





static void list_remove(VSrc*);


static void VSrc_init(VSrc* vs, va_list args)
{
	vs->stream = va_arg(args, bool);
}


static void VSrc_dtor(VSrc* vs)
{
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

	// store so we can shut ourselves down via sound_free when done playing
	vs->hvs = hvs;

	vs->hsd = snd_data_load(snd_data_fn.c_str(), vs->stream);

	return 0;
}


// open and return a handle to the sound <fn>.
// stream: default false
Handle snd_open(const char* const fn, const bool stream)
{
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
	std::for_each(vsources.begin(), vsources.end(), cb);
}


// O(N)!
static void list_remove(VSrc* vs)
{
	for(size_t i = 0; i < vsources.size(); i++)
		if(vsources[i] == vs)
		{
			vsources[i] = 0;
			return;
		}

	debug_warn("list_remove: VSrc not found");
}


static int list_update()
{
	// prune NULL-entries, so code below doesn't have to check if non-NULL
	// (these were removed, but we didn't shuffle everything down to save time)
	It src = vsources.begin(), dst = vsources.begin();
	size_t valid_entries = vsources.size();
	for(;;)
	{
		if(src == vsources.end())
			break;
		if(*src == 0)
		{
			++src;
			valid_entries--;
			continue;
		}
		*dst = *src;
		++dst;
		++src;
	}
	vsources.resize(valid_entries);

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
			vsrc_reclaim_src(vs);

		if(!vs->loop)
			snd_free(vs->hvs);
	}

	// grant each of the most important vsources a source
	for(it = vsources.begin(); it != first_unimportant; ++it)
	{
		VSrc* vs = *it;
		if(!vs->al_src)
			vsrc_grant_src(vs);
	}


	std::for_each(vsources.begin(), vsources.end(), vsrc_update);

	return 0;
}


int snd_play(Handle hs)
{
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





int snd_update(float lx, float ly, float lz)
{
	float* p = listener_pos;
	p[0] = lx; p[1] = ly; p[2] = lz;

	list_update();

	CHECK_ERR(io_check_complete());

	return 0;
}


int snd_shutdown()
{
//	io_shutdown();
	return 0;
}
