#include "precompiled.h"

#include "res/res.h"
#include "res/snd.h"

#include <sstream>	// to extract snd_open's definition file contents

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

#define OGG_HACK
#include "ogghack.h"


#ifdef _MSC_VER
#pragma comment(lib, "openal32.lib")
#pragma comment(lib, "alut.lib")
#endif






static bool al_initialized = false;
	// indicates OpenAL is ready for use. checked by other components
	// when deciding if they can pass settings changes to OpenAL directly,
	// or whether they need to be saved until init.

static int al_reinit();
static int list_free_all();
static void hsd_list_free_all();



// rationale for "buffer" handle and separate handle for each sound instance:
// - need access to sound instances to fade them out / loop for an unknown period
// - access via handle for safety
// - don't want to reload sound data every time => need one central instance
//   that owns the data
// - want to support normal reload mechanism (for consistency if not necessity)
// - could hack something via h_find / if so, create new handle with fn_key = 0,
//   but that would break reloading and is dodgy. we will create a new handle
//   type instead.



static void al_check(const char* caller = "(unknown)")
{
	assert(al_initialized);

	ALenum err = alGetError();
	if(err == AL_NO_ERROR)
		return;

	const char* str = (const char*)alGetString(err);
	debug_out("openal error: %s; called from %s", str, caller);
	debug_warn("OpenAL error");
}







///////////////////////////////////////////////////////////////////////////////
//
// OpenAL context: readies OpenAL for use; allows specifying the device,
// in case there are problems with OpenAL's default choice.
//
///////////////////////////////////////////////////////////////////////////////

static const char* alc_dev_name = 0;
	// default: use OpenAL default device.


// tell OpenAL to use the specified device in future.
// name = 0 reverts to OpenAL's default choice, which will also
// be used if this routine is never called.
//
// the device name is typically taken from a config file at init-time;
// the snd_dev* enumeration routines below are used to present a list
// of choices to the user in the options screen.
//
// if OpenAL hasn't yet been initialized (i.e. no sounds have been opened),
//   this just stores the device name for use when init does occur.
//   note: we can't check now if it's invalid (if so, init will fail).
// otherwise, we shut OpenAL down (thereby stopping all sounds) and
// re-initialize with the new device. that's fairly time-consuming,
// so preferably call this routine before sounds are loaded.
//
// return 0 on success, or the status returned by OpenAL re-init.
int snd_dev_set(const char* alc_new_dev_name)
{
	// requesting a specific device
	if(alc_new_dev_name)
	{
		// already using that device - done. (don't re-init)
		if(alc_dev_name && !strcmp(alc_dev_name, alc_new_dev_name))
			return 0;

		// store name (need to copy it, since alc_init is called later,
		// and it must then still be valid)
		static char buf[32];
		strncpy(buf, alc_new_dev_name, 32-1);
		alc_dev_name = buf;
	}
	// requesting default device
	else
	{
		// already using default device - done. (don't re-init)
		if(alc_dev_name == 0)
			return 0;

		alc_dev_name = 0;
	}

	return al_reinit();
		// no-op if not initialized yet, otherwise re-init
}


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
	// we hold a reference to prevent the actual unload,
	// thus speeding up startup by 100..400 ms. everything works ATM;
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

	// check if init succeeded.
	// some OpenAL implementations don't indicate failure here correctly;
	// we need to check if the device and context pointers are actually valid.
	ALCenum err = alcGetError(alc_dev);
	if(err != ALC_NO_ERROR || !alc_dev || !alc_ctx)
	{
		debug_out("alc_init failed. alc_dev=%p alc_ctx=%p alc_dev_name=%s err=%d\n", alc_dev, alc_ctx, alc_dev_name, err);
		ret = -1;
	}

	// release DLL references, so BoundsChecker doesn't complain at exit.
#ifdef _WIN32
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
static float al_listener_pos[3];
static float al_listener_orientation[6];
	// float view_direction[3], up_vector[3]; passed directly to OpenAL


// also called from al_init.
static void al_listener_latch()
{
	if(al_initialized)
	{
		alListenerf(AL_GAIN, al_listener_gain);
		alListenerfv(AL_POSITION, al_listener_pos);
		alListenerfv(AL_ORIENTATION, al_listener_orientation);
		al_check("al_listener_latch");
	}
}


int snd_set_master_gain(float gain)
{
	if(gain < 0)
	{
		debug_warn("snd_set_master_gain: gain < 0");
		return ERR_INVALID_PARAM;
	}

	al_listener_gain = gain;

	al_listener_latch();
		// position will get sent too.
		// this isn't called often, so we don't care.

	return 0;
}


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


// return euclidean distance squared between listener and point.
// used to determine sound priority.
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

// allocate a new buffer, and fill it with the specified data.
static ALuint al_buf_alloc(ALvoid* data, ALsizei size, ALenum al_fmt, ALsizei al_freq)
{
	ALuint al_buf;
	alGenBuffers(1, &al_buf);
	alBufferData(al_buf, al_fmt, data, size, al_freq);
	al_check("al_buf_alloc");

	al_bufs_outstanding++;
	return al_buf;
}


static void al_buf_free(ALuint al_buf)
{
	assert(alIsBuffer(al_buf));
	alDeleteBuffers(1, &al_buf);
	al_check("al_buf_free");

	al_bufs_outstanding--;
}


// make sure all buffers have been returned to us via al_buf_free.
// called from al_shutdown.
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

static const int AL_SRC_MAX = 64;
	// regardless of sound card caps, we won't use more than this ("enough").
	// necessary in case OpenAL doesn't limit #sources (e.g. if SW mixing).
static ALuint al_srcs[AL_SRC_MAX];
	// stack of sources (first allocated is #0)
static int al_src_allocated;
	// number of valid sources in al_srcs[] (set by al_src_init)
static int al_src_used = 0;
	// number of sources currently in use
static int al_src_cap = AL_SRC_MAX;
	// user-set limit on how many sources may be used


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
	al_src_allocated = 0;
	al_check("al_src_shutdown");
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
	// or al_src_init hasn't been called yet. note: we accept anything
	// in the second case, as al_src_init will sanity-check al_src_cap.
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
// OpenAL startup mechanism: allows deferring init until sounds are actually
// played, therefore speeding up perceived game start time.
// also resets OpenAL when settings (e.g. device) are changed at runtime.
//
///////////////////////////////////////////////////////////////////////////////

// called from each snd_open; no harm if called more than once.
static int al_init()
{
	// only take action on first call, OR when re-initializing.
	if(al_initialized)
		return 0;
	al_initialized = true;

	CHECK_ERR(alc_init());

	// these can't fail:
	al_src_init();
	al_listener_latch();

	return 0;
}


static void al_shutdown()
{
	// was never initialized - nothing to do.
	if(!al_initialized)
		return;

	// free all active sounds
	list_free_all();
	// actually free (they're cached) all SndData instances ever allocated.
	hsd_list_free_all();

	al_src_shutdown();
	al_buf_shutdown();
	alc_shutdown();

	al_initialized = false;
}


// called from snd_dev_set (no other settings that require re-init ATM).
static int al_reinit()
{
	// not yet initialized. settings have been saved, and will be
	// applied by the component init routines called from al_init.
	if(!al_initialized)
		return 0;

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

static const size_t STREAM_BUF_SIZE = 32*KB;


//
// I/O buffer suballocator for streamed sounds, to avoid
// frequent alloc/frees and therefore heap fragmentation.
//
///////////////////////////////////////////////////////////////////////////////

static const int TOTAL_IOS = MAX_STREAMS * MAX_IOS;
static const size_t TOTAL_BUF_SIZE = TOTAL_IOS * STREAM_BUF_SIZE;

static void* io_bufs;
	// one large allocation for all buffers

static void* io_buf_freelist;
	// list of free buffers. start of buffer holds pointer to next in list.


static void io_buf_free(void* p)
{
	assert(io_bufs <= p && p <= (char*)io_bufs+TOTAL_BUF_SIZE);
	*(void**)p = io_buf_freelist;
	io_buf_freelist = p;
}


// called from first io_buf_alloc.
static void io_buf_init()
{
	// allocate 1 big aligned block for all buffers
	io_bufs = mem_alloc(TOTAL_BUF_SIZE, 4*KB);
	// .. failed; io_buf_alloc calls will return 0
	if(!io_bufs)
		return;

	// build freelist
	char* p = (char*)io_bufs;
	for(int i = 0; i < TOTAL_IOS; i++)
	{
		io_buf_free(p);
		p += STREAM_BUF_SIZE;
	}
}


static void* io_buf_alloc()
{
	ONCE(io_buf_init());

	void* buf = io_buf_freelist;
	// note: we have to bail now; can't update io_buf_freelist.
	if(!buf)
	{
		if(!io_bufs)
			debug_warn("io_buf_alloc: not enough memory to allocate buffer pool");
		else
			debug_warn("io_buf_alloc: max #streams exceeded");
		return 0;
	}

	io_buf_freelist = *(void**)io_buf_freelist;
	return buf;
}


// no-op if io_buf_alloc was never called.
// called by snd_shutdown.
static void io_buf_shutdown()
{
	mem_free(io_bufs);
}


//
// stream layer on top of VFS async I/O: owns queue and buffers
//
///////////////////////////////////////////////////////////////////////////////

// rationale: no need for a centralized queue - we have a suballocator,
// so reallocs aren't a problem; central scheduling isn't necessary,
// because we'll only have <= 2 streams active at a time.

struct Stream
{
	Handle hf;
	Handle ios[MAX_IOS];
	uint active_ios;
	void* last_buf;
		// set by stream_buf_get, used by stream_buf_discard to free buf.
};

// called from SndData_reload and snd_data_buf_get.
static int stream_issue(Stream* s)
{
	if(s->active_ios >= MAX_IOS)
		return 0;

	void* buf = io_buf_alloc();
	if(!buf)
		return ERR_NO_MEM;

	Handle h = vfs_start_io(s->hf, STREAM_BUF_SIZE, buf);
	CHECK_ERR(h);
	s->ios[s->active_ios++] = h;
	return 0;
}


static int stream_buf_get(Stream* s, void*& data, size_t& size)
{
	if(s->active_ios == 0)
		return ERR_EOF;
	Handle hio = s->ios[0];

	// has it finished? if not, bail
	int is_complete = vfs_io_complete(hio);
	CHECK_ERR(is_complete);
	if(is_complete == 0)
		return ERR_AGAIN;

	// get its buffer
	CHECK_ERR(vfs_wait_io(hio, data, size));
		// no delay, since vfs_io_complete == 1

	s->last_buf = data;
		// next stream_buf_discard will free this buffer
	return 0;
}


// must be called once after every stream_buf_get;
// call before calling any other stream_* functions!
static int stream_buf_discard(Stream* s)
{
	Handle hio = s->ios[0];

	int ret = vfs_discard_io(hio);

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

static int stream_open(Stream* s, const char* fn)
{
	if(active_streams >= MAX_STREAMS)
	{
		debug_warn("stream_open: MAX_STREAMS exceeded - why?");
		return -1;
			// fail, because we wouldn't have enough IO buffers for all
	}
	active_streams++;

	s->hf = vfs_open(fn);
	CHECK_ERR(s->hf);

	for(int i = 0; i < MAX_IOS; i++)
		CHECK_ERR(stream_issue(s));
	
	return 0;
}


// returns the first error that occurred while waiting for IOs / closing file.
static int stream_close(Stream* s)
{
	int ret = 0;
	int err;

	// for each pending IO:
	for(uint i = 0; i < s->active_ios; i++)
	{
		// .. wait until complete,
		void* data; size_t size;	// unused
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


///////////////////////////////////////////////////////////////////////////////
//
// sound data provider
//
///////////////////////////////////////////////////////////////////////////////

// rationale: could make a case for a separate IO layer, but there's a
// problem: IOs need to be discarded after their data has been processed.
// if the IO layer is separate, we'd either need a callback from
// io_complete, passing the buffer to OpenAL, or mark IO slots as
// "discardable", so that they are freed the next io_issue.
// both are ugly; we instead integrate IO into the sound data code.
// IOs are passed to OpenAL and the discarded immediately.

struct SndData
{
	// stream
	Stream s;
	ALenum al_fmt;
	ALsizei al_freq;

	// clip
	ALuint al_buf;

	bool is_stream;

#ifdef OGG_HACK
// pointer to Ogg instance
void* o;
#endif
};

H_TYPE_DEFINE(SndData);


// open and return a handle to a sound file's data.
static Handle snd_data_load(const char* const fn, const bool stream)
{
	// don't allow reloading stream objects
	// (both references would read from the same file handle).
	const uint flags = stream? RES_UNIQUE : 0;

	return h_alloc(H_SndData, fn, flags, stream);
}


// close the sound file data <hsd> and set hsd to 0.
static int snd_data_free(Handle& hsd)
{
	return h_free(hsd, H_SndData);
}


///////////////////////////////////////////////////////////////////////////////

// rationale: must free before OpenAL shuts down. no longer know of buffers
// after sound completes - already removed from list.
// can't rely on h_mgr_shutdown cleaning up all leaked handles (i.e.
// snd_shutdown after h_mgr_shutdown) - same problem when resetting
// OpenAL at runtime.

typedef std::vector<Handle> Handles;
static Handles hsd_list;

static void hsd_list_add(Handle hsd)
{
	hsd_list.push_back(hsd);
}

// rationale: no need to remove single entries - they are all wiped out
// during al_shutdown; afterwards, the list is cleared.


// called by al_shutdown (at exit, or when reinitializing OpenAL).
static void hsd_list_free_all()
{
	for(Handles::iterator it = hsd_list.begin(); it != hsd_list.end(); ++it)
	{
		Handle& hsd = *it;
		int err1 = h_allow_free(hsd, H_SndData);
		// already freed via list_free_all
		if(err1 == ERR_INVALID_HANDLE)
			continue;

		int err2 = snd_data_free(hsd);
		if(err1 < 0 || err2 < 0)
			debug_warn("sd_list_free_all: error while freeing a handle");
	}

	// leave its memory intact, so we don't have to reallocate it later
	// if we are now reinitializing OpenAL (not exiting).
	hsd_list.resize(0);
}


///////////////////////////////////////////////////////////////////////////////


static void SndData_init(SndData* sd, va_list args)
{
	sd->is_stream = va_arg(args, bool);
}

static void SndData_dtor(SndData* sd)
{
	if(sd->is_stream)
		stream_close(&sd->s);
	else
		al_buf_free(sd->al_buf);
}


static int SndData_reload(SndData* sd, const char* fn, Handle hsd)
{
	hsd_list_add(hsd);

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
#ifdef OGG_HACK
#else
		// first use of OGG: check if OpenAL extension is available.
		// note: this is required! OpenAL does its init here.
		static int ogg_supported = -1;
		if(ogg_supported == -1)
			ogg_supported = alIsExtensionPresent((ALubyte*)"AL_EXT_vorbis")? 1 : 0;
		if(!ogg_supported)
			return -1;

		sd->al_fmt  = AL_FORMAT_VORBIS_EXT;
		sd->al_freq = 0;
#endif

		file_type = FT_OGG;
	}
	// .. WAV
	else if(ext && !stricmp(ext, ".wav"))
		file_type = FT_WAV;
	// .. unknown extension
	else
		return -1;


	if(sd->is_stream)
	{
		// refuse to stream anything that cannot be passed directly to OpenAL -
		// we'd have to extract the audio data ourselves (not worth it).
		if(file_type != FT_OGG)
			return -1;

		return stream_open(&sd->s, fn);
	}

	// else: clip

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

#ifdef OGG_HACK
std::vector<u8> data;
if(file_type == FT_OGG)
{
sd->o = ogg_create();
ogg_give_raw(sd->o, file, file_size);
ogg_open(sd->o, sd->al_fmt, sd->al_freq);
size_t datasize=0;
size_t bytes_read;
do
{
const size_t bufsize = 32*KB;
char buf[bufsize];
bytes_read = ogg_read(sd->o, buf, bufsize);
data.resize(data.size() + bytes_read);
for(size_t i = 0; i < bytes_read; i++) data[datasize+i] = buf[i];
datasize += bytes_read;
}
while(bytes_read > 0);
al_data = &data[0];
al_size = (ALsizei)datasize;
}
#endif

	sd->al_buf = al_buf_alloc(al_data, al_size, sd->al_fmt, sd->al_freq);

	mem_free(file);

	return 0;
}


// (need to convert ERR_EOF and ERR_AGAIN to legitimate return values -
// for the caller, those aren't errors.)
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

static int snd_data_buf_get(Handle hsd, ALuint& al_buf)
{
	int err = 0;

	// in case H_DEREF fails
	al_buf = 0;

	H_DEREF(hsd, SndData, sd);

	// clip: just return buffer (which was created in snd_data_load)
	if(!sd->is_stream)
	{
		al_buf = sd->al_buf;
		return BUF_EOF;
	}

	// stream:

	// .. check if IO finished.
	void* data;
	size_t size;
	err = stream_buf_get(&sd->s, data, size);
	if(err == ERR_AGAIN)
		return BUF_AGAIN;
	CHECK_ERR(err);

	// .. yes: pass to OpenAL and discard IO buffer.
	al_buf = al_buf_alloc(data, (ALsizei)size, sd->al_fmt, sd->al_freq);
	stream_buf_discard(&sd->s);

	// .. try to issue the next IO.
	// if EOF reached, indicate al_buf is the last that will be returned.
	err = stream_issue(&sd->s);
	if(err == ERR_EOF)
		return BUF_EOF;
	CHECK_ERR(err);

	// al_buf valid and next IO issued successfully.
	return BUF_OK;
}


static int snd_data_buf_free(Handle hsd, ALuint al_buf)
{
	H_DEREF(hsd, SndData, sd);

	// clip: no-op (caller will later release hsd reference;
	// when hsd actually unloads, sd->al_buf will be freed).
	if(!sd->is_stream)
		return 0;

	// stream: we had allocated an additional buffer, so free it now.
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



#include "timer.h"



static void vsrc_latch(VSrc* vs)
{
	if(!vs->al_src)
		return;

	alSourcefv(vs->al_src, AL_POSITION,        vs->pos);
	alSourcei (vs->al_src, AL_SOURCE_RELATIVE, vs->relative);
	alSourcef (vs->al_src, AL_GAIN,            vs->gain);
	alSourcei (vs->al_src, AL_LOOPING,         vs->loop);
	al_check("vsrc_latch");
}


static int vsrc_update(VSrc* vs)
{
	// remove all finished buffers
	int num_processed;
	alGetSourcei(vs->al_src, AL_BUFFERS_PROCESSED, &num_processed);
	for(int i = 0; i < num_processed;  i++)
	{
		ALuint al_buf;
		alSourceUnqueueBuffers(vs->al_src, 1, &al_buf);
debug_out("removing processed buf=%p vs=%p src=%p   hvs=%I64x\n", al_buf, vs, vs->al_src, vs->hvs);
		CHECK_ERR(snd_data_buf_free(vs->hsd, al_buf));
	}

if(vs->closing)
return 0;

	// no more buffers left, and EOF reached
	ALint num_queued;
	alGetSourcei(vs->al_src, AL_BUFFERS_QUEUED, &num_queued);
	al_check("vsrc_update alGetSourcei");

	if(num_queued == 0 && vs->eof)
	{
debug_out("%g: reached end, closing vs=%p src=%p    hvs=%I64x\n", get_time(), vs, vs->al_src, vs->hvs);
		snd_free(vs->hvs);
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
			ret = snd_data_buf_get(vs->hsd, al_buf);
			CHECK_ERR(ret);

debug_out("%g: got buf: buf=%p vs=%p src=%p   hvs=%I64x\n", get_time(), al_buf, vs, vs->al_src, vs->hvs);

			alSourceQueueBuffers(vs->al_src, 1, &al_buf);
			al_check("vsrc_update SourceQueueBuffers");
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
	al_check("vsrc_grant_src Source*");

	// we only now got a source, so latch previous settings
	vsrc_latch(vs);	// can't fail

	CHECK_ERR(vsrc_update(vs));

debug_out("play vs=%p src=%p    hvs=%I64x\n", vs, vs->al_src, vs->hvs);
	alSourcePlay(vs->al_src);
	al_check("vsrc_grant_src SourcePlay");
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
	al_check("vsrc_reclaim_src SourceStop");

vs->closing = true;
	CHECK_ERR(vsrc_update(vs));
	// (note: all buffers are now considered 'processed', since src is stopped)

	al_src_free(vs->al_src);
	return 0;
}





static void list_add(VSrc*);
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

#if 0
	void* def_file;
	size_t def_size;
	CHECK_ERR(vfs_load(def_fn, def_file, def_size));
	std::istringstream def(std::string((char*)def_file, (int)def_size));
	mem_free(def_file);

	std::string snd_data_fn;
	float gain_percent;
	def >> snd_data_fn;
	def >> gain_percent;
#else
float gain_percent = 100.0;
std::string snd_data_fn = def_fn;
#endif

	vs->gain = gain_percent / 100.0f;
		// can legitimately be > 1.0 - don't clamp.

	vs->hvs = hvs;
		// needed so we can snd_free ourselves when done playing.

	const char* data_fn = snd_data_fn.c_str();
	vs->hsd = snd_data_load(data_fn, vs->stream);

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




// relative: default false.
int snd_set_pos(Handle hvs, float x, float y, float z, bool relative)
{
	H_DEREF(hvs, VSrc, vs);

	vs->pos[0] = x; vs->pos[1] = y; vs->pos[2] = z;
	vs->relative = relative;

	vsrc_latch(vs);
	return 0;
}


int snd_set_gain(Handle hvs, float gain)
{
	H_DEREF(hvs, VSrc, vs);

	vs->gain = gain;

	vsrc_latch(vs);
	return 0;
}


int snd_set_loop(Handle hvs, bool loop)
{
	H_DEREF(hvs, VSrc, vs);

	vs->loop = loop;

	vsrc_latch(vs);
	return 0;
}








static float magnitude_2(const float v[3])
{
	return v[0]*v[0] + v[1]*v[1] + v[2]*v[2];
}


static float calc_pri(const float pos[3], ALboolean relative, float static_pri)
{
	const float MAX_DIST_2 = 1000.0f;
	const float falloff = 10.0f;

	float d_2;	// euclidean distance to listener (squared)
	if(relative)
		d_2 = magnitude_2(pos);
	else
		d_2 = al_listener_dist_2(pos);

	// scale priority down exponentially
	float e = d_2 / MAX_DIST_2;	// 0.0f (close) .. 1.0f (far)

	// farther away than OpenAL cutoff - no sound contribution
	if(e > 1.0f)
		return -1;
	return static_pri / pow(falloff, e);
}







///////////////////////////////////////////////////////////////////////////////
//
// list of sounds
//
///////////////////////////////////////////////////////////////////////////////


static bool vsrc_greater(const VSrc* const s1, const VSrc* const s2)
{
	return s1->cur_pri > s2->cur_pri;
}

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
	return vs == 0;
}


static void vsrc_free(VSrc* vs)
{
	snd_free(vs->hvs);
}

static void vsrc_calc_cur_pri(VSrc* vs)
{
	vs->cur_pri = calc_pri(vs->pos, vs->relative, vs->static_pri);
}


// no-op if OpenAL not yet initialized.
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
			snd_free(vs->hvs);
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


static int list_free_all()
{
	list_foreach(vsrc_free);
	return 0;
}



///////////////////////////////////////////////////////////////////////////////


int snd_update(const float* pos, const float* dir, const float* up)
{
	if(pos || dir || up)
		al_listener_set_pos(pos, dir, up);

	list_update();

	return 0;
}


void snd_shutdown()
{
debug_out("SND_SHUTDOWN\n");
	io_buf_shutdown();

	al_shutdown();
		// calls list_free_all
}
