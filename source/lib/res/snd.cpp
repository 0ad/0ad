#include "precompiled.h"

#include "res/res.h"
#include "res/snd.h"

#ifdef __APPLE__
# include <OpenAL/alut.h>
#else
# include <AL/al.h>
# include <AL/alut.h>
#endif

#ifdef _MSC_VER
#pragma comment(lib, "openal32.lib")
#pragma comment(lib, "alut.lib")
#endif



// called as late as possible, i.e. the first time sound/music is played
// (either from module init there, or from the play routine itself).
// this delays library load, leading to faster perceived app startup.
// registers an atexit routine for cleanup.
// no harm if called more than once.
int oal_Init()
{
	ONCE({
		alutInit(0, 0);
		atexit(alutExit);
	});

	return 0;
}


///////////////////////////////////////////////////////////////////////////////


// rationale for "buffer" handle and separate handle for each sound instance:
// - need access to sound instances to fade them out / loop for an unknown period
// - access via handle for safety
// - don't want to reload sound data every time => need one central instance
//   that owns the data
// - want to support normal reload mechanism (for consistentcy if not necessity)
// - could hack something via h_find / if so, create new handle with fn_key = 0,
//   but that would break reloading and is dodgy. we will create a new handle
//   type instead.

enum SoundFlags
{
	SF_PLAY_PENDING = 1,
	SF_STREAMING = 2
};

struct Sound
{
	uint flags;
	ALuint al_source;

	// list of all sounds
	Sound* next;

	// handle to this Sound, so that it can close itself
	Handle hs;

	// if stream:
	Handle hf;
	ALenum al_format;
	ALsizei al_sample_rate;
	int queued_buffers;

	// if clip:
	Handle hb;
};


static void check()
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
}


static bool ogg_supported;

static const size_t CLIP_MAX_SIZE = 512*KB;

static ALuint al_create_buffer(void* data, size_t size, ALenum al_format, ALsizei al_sample_rate)
{
	ALuint al_buffer;
	alGenBuffers(1, &al_buffer);
	alBufferData(al_buffer, al_format, data, (ALsizei)size, al_sample_rate);
	check();
	return al_buffer;
}


static void snd_init();

///////////////////////////////////////////////////////////////////////////////
//
// audio file format interpretation
//
///////////////////////////////////////////////////////////////////////////////

static const u32 ID_RIFF = FOURCC('R', 'I', 'F', 'F');

struct RiffHeader
{
	u32 id;
	u32 remaining_bytes;
};
cassert(sizeof(RiffHeader) == 8);


struct RiffFileHeader
{
	RiffHeader hdr;
	u32 file_id;
};
cassert(sizeof(RiffFileHeader) == 12);

static int riff_valid(Handle hf, u32 file_id)
{
	RiffFileHeader file_hdr;
	void* dst = &file_hdr;	// for vfs_io
	ssize_t bytes_read = vfs_io(hf, 12, &dst);
	if(bytes_read != 12)
		return -1;

	// check "RIFF" FourCC
	RiffHeader* hdr = &file_hdr.hdr;
	if(hdr->id != ID_RIFF)
		return -1;

	// paranoia: check if file size is as reported
	// (no harm if not).
	ssize_t file_size = vfs_size(hf);
	if(file_size != hdr->remaining_bytes+8)
		debug_warn("note: riff_valid: file size mismatch");

	// check file type FourCC
	if(file_hdr.file_id != file_id)
		return -1;

	return 0;
}


// (optional) output parameter zeroed on failure.
static int riff_find_chunk(Handle hf, u32 id, size_t* chunk_size = 0)
{
	if(chunk_size)
		*chunk_size = 0;

	for(;;)
	{
		// read chunk header
		RiffHeader hdr;
		void* dst = &hdr;	// for vfs_io
		ssize_t bytes_read = vfs_io(hf, 8, &dst);
		CHECK_ERR(bytes_read);
		if(bytes_read != 8)
			return -1;

		// .. it was what we're looking for; done.
		if(hdr.id == id)
		{
			if(chunk_size)
				*chunk_size = hdr.remaining_bytes;
			return 0;
		}

		// read up to start of next chunk
		// (don't bother merging with above read)
		bytes_read = vfs_io(hf, hdr.remaining_bytes, 0);
		CHECK_ERR(bytes_read);
		if(bytes_read != hdr.remaining_bytes)
			return -1;
	}
}



static const u32 ID_WAVE = FOURCC('W', 'A', 'V', 'E');
static const u32 ID_fmt  = FOURCC('f', 'm', 't', ' ');
static const u32 ID_data = FOURCC('d', 'a', 't', 'a');

#pragma pack(push, 1)

struct WavFormatChunk
{
	i16 wFormatTag;
	u16 wChannels;
	u32 dwSamplesPerSec;
	u32 dwAvgBytesPerSec;
	u16 wBlockAlign;
	u16 wBitsPerSample;

	// additional fields may follow, if wFormatTag != 1
};
cassert(sizeof(WavFormatChunk) == 16);

#pragma pack(pop)


// output parameters zeroed on failure.
static int wav_detect_format(Handle hf, ALenum* al_format, ALsizei* al_sample_rate)
{
	*al_format = 0;
	*al_sample_rate = 0;

	int ret = -1;

	WavFormatChunk* fmt = 0;

	{
		size_t reported_fmt_size;
		CHECK_ERR(riff_find_chunk(hf, ID_fmt, &reported_fmt_size));
		const size_t fmt_size = sizeof(WavFormatChunk);
		// we need to read up to and including the wBitsPerSample member.
		if(reported_fmt_size < fmt_size)
			return -1;

		fmt = (WavFormatChunk*)malloc(fmt_size);
		void* dst = fmt;	// for vfs_io
		ssize_t bytes_read = vfs_io(hf, fmt_size, &dst);
		if(bytes_read != fmt_size)
		{
			ret = (int)bytes_read;
			goto fail;
		}

		//
		// determine format
		//

		const bool mono = (fmt->wChannels == 1);
		const int bps = (int)fmt->wBitsPerSample;

		// barring extensions that add WAV formats, OpenAL
		// only supports uncompressed 8- or 16-bit PCM data.
		if(fmt->wFormatTag != 1)
			return -1;
		if(bps != 8 && bps != 16)
			return -1;

		if(mono)
		{
			if(bps == 8)
				*al_format = AL_FORMAT_MONO8;
			else
				*al_format = AL_FORMAT_MONO16;
		}
		else
		{
			if(bps == 8)
				*al_format = AL_FORMAT_STEREO8;
			else
				*al_format = AL_FORMAT_STEREO16;
		}

		*al_sample_rate = fmt->dwSamplesPerSec;


		// seek to data chunk
		ret = riff_find_chunk(hf, ID_data);

	}

fail:
	free(fmt);
	return ret;
}



///////////////////////////////////////////////////////////////////////////////
//
// AL buffer for clips
//
///////////////////////////////////////////////////////////////////////////////


struct ALBuffer
{
	ALuint al_buffer;
};

H_TYPE_DEFINE(ALBuffer);


static void ALBuffer_init(ALBuffer*, va_list)
{
}

static void ALBuffer_dtor(ALBuffer* b)
{
	alDeleteBuffers(1, &b->al_buffer);
	check();
}


static int ALBuffer_reload(ALBuffer* b, const char* fn, Handle)
{
	int ret = -1;

	// freed in bailout ladder
	Handle hf = 0;
	void* file = 0;

	{

	hf = vfs_open(fn);
	CHECK_ERR(hf);


	//
	// detect sound format (by checking file extension)
	//

	ALenum al_format;
	ALsizei al_sample_rate;

	const char* ext = strrchr(fn, '.');
	// .. OGG (data will be passed directly to OpenAL)
	if(ext && !stricmp(ext, ".ogg"))
	{
		if(!ogg_supported)
		{
			ret = -1;
			goto fail;
		}

		al_format = AL_FORMAT_VORBIS_EXT;
		al_sample_rate = 1;
	}
	// .. WAV (read format from header and seek to audio data)
	else if(ext && !stricmp(ext, ".wav"))
	{
		int err = wav_detect_format(hf, &al_format, &al_sample_rate);
		if(err < 0)
		{
			ret = err;
			goto fail;
		}
	}
	// .. unknown extension
	else
	{
		ret = -1;
		goto fail;
	}


	//
	// allocate buffer for audio data
	//

	// note: freed after openal latches its contents in al_create_buffer.
	// we don't know how much has already been read by wav_detect_format;
	// allocate enough for the entire file.
	ssize_t file_size = vfs_size(hf);
	if(file_size < 0)
	{
		ret = file_size;
		goto fail;
	}
	void* file = mem_alloc(file_size, 65536);	// freed soon after
	if(!file)
	{
		ret = ERR_NO_MEM;
		goto fail;
	}
memset(file, 0xe2, file_size);


	// read from file. note: don't use vfs_load - detect_audio_fmt
	// has seeked to the actual audio data in the file.
	void* dst = file;	// for vfs_io
	ssize_t bytes_read = vfs_io(hf, file_size, &dst);
	if(bytes_read < 0)
	{
		ret = bytes_read;
		goto fail;
	}

	b->al_buffer = al_create_buffer(file, bytes_read, al_format, al_sample_rate);

	ret = 0;
fail:
	mem_free(file);
	vfs_close(hf);

	}

	return ret;
}


// open and return a handle to the sound clip <fn>.
static Handle albuffer_load(const char* const fn)
{
	return h_alloc(H_ALBuffer, fn);
}


// close the buffer <hb> and set hb to 0.
static int albuffer_free(Handle& hb)
{
	return h_free(hb, H_ALBuffer);
}


static ALuint albuffer_get(Handle hb)
{
	H_DEREF(hb, ALBuffer, b);
	return b->al_buffer;
}





///////////////////////////////////////////////////////////////////////////////
// list of sounds
///////////////////////////////////////////////////////////////////////////////

// currently active sounds - needed to update them, i.e. remove old buffers
// and enqueue just finished async buffers. this can't happen from
// io_check_complete alone - see dox there.

// don't use std::list - expect many sounds => don't alloc for each
// implementation: slist; add to front for convenience
static Sound* head;

static void sounds_add(Sound* s)
{
	// never allow adding 0 - would leak the entire list
	if(!s)
	{
		debug_warn("sounds_add: adding 0 not allowed");
		return;
	}
	s->next = head;
	head = s;
}


static void sounds_remove(Sound* target)
{
	Sound** pprev = &head;
	for(Sound* s = head; s != 0; s = s->next)
	{
		if(s == target)
		{
			*pprev = target->next;
			return;
		}
		pprev = &s->next;
	}

	debug_warn("sounds_remove: not in list");
}


static int sounds_foreach(int (*cb)(Sound*))
{
	for(Sound* s = head; s != 0; s = s->next)
		CHECK_ERR(cb(s));
	return 0;
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

struct IO
{
	Handle hio;
	void* raw_buf;

	// 
	Sound* s;

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


static int io_issue(Sound* s, Handle hf)
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



static int sound_add_buffer(Sound* s, void* buf, size_t size);
static int sound_update(Sound* s);


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

		CHECK_ERR(sound_add_buffer(slot->s, buf, size));

		CHECK_ERR(vfs_discard_io(slot->hio));
	}

	return 0;
}




///////////////////////////////////////////////////////////////////////////////
//
// sound instance
//
///////////////////////////////////////////////////////////////////////////////


H_TYPE_DEFINE(Sound);


static void Sound_init(Sound*, va_list)
{
}

static void Sound_dtor(Sound* s)
{
	sounds_remove(s);

	alDeleteSources(1, &s->al_source);
	check();

	// move to sounddatasource
	vfs_close(s->hf);
	albuffer_free(s->hb);
}


static int Sound_reload(Sound* s, const char* fn, Handle hs)
{
	// always add; if this fails, dtor is called, which removes from list
	sounds_add(s);

	s->hs = hs;

	alGenSources(1, &s->al_source);
	check();

	// OpenAL docs don't specify default values, so initialize everything
	// ourselves to be sure. note: alSourcefv param is not const.
	float zero3[3] = { 0.0f, 0.0f, 0.0f };
	alSourcefv(s->al_source, AL_POSITION, zero3);
	alSourcefv(s->al_source, AL_VELOCITY, zero3);
	alSourcefv(s->al_source, AL_DIRECTION, zero3);
	alSourcef(s->al_source, AL_ROLLOFF_FACTOR, 0.0f);
	alSourcei(s->al_source, AL_SOURCE_RELATIVE, AL_TRUE);
	check();








	// decide if it'll be streamed or loaded into memory
	struct stat stat_buf;
	CHECK_ERR(vfs_stat(fn, &stat_buf));
	off_t file_size = stat_buf.st_size;
	// big enough to warrant streaming
//	if(file_size > CLIP_MAX_SIZE)
//		s->flags = SF_STREAMING;

	// TODO: let caller decide as well



	if(s->flags & SF_STREAMING)
	{
		s->hf = vfs_open(fn);
		CHECK_ERR(s->hf);

//		CHECK_ERR(detect_audio_fmt(s->hf, &s->al_format, &s->al_sample_rate));

		for(int i = 0; i < MAX_IOS; i++)
			CHECK_ERR(io_issue(s, s->hf));
	}
	else
	{
		s->hb = albuffer_load(fn);
		CHECK_ERR(s->hb);

		ALuint al_buffer = albuffer_get(s->hb);
		alSourceQueueBuffers(s->al_source, 1, &al_buffer);
	}

	return 0;
}


// open and return a handle to the sound <fn>.
Handle sound_open(const char* const fn)
{
	snd_init();
	return h_alloc(H_Sound, fn, RES_NO_CACHE);
}


// close the sound <hs> and set hs to 0.
int sound_free(Handle& hs)
{
	return h_free(hs, H_Sound);
}



static int sound_add_buffer(Sound* s, void* buf, size_t size)
{
	ALuint al_buffer = al_create_buffer(buf, size, s->al_format, s->al_sample_rate);
	alSourceQueueBuffers(s->al_source, 1, &al_buffer);
	check();

	if(s->flags & SF_PLAY_PENDING && s->queued_buffers > MIN_BUFFERS)
	{
		alSourcePlay(s->al_format);
		s->flags &= ~SF_PLAY_PENDING;
	}

	if(!(s->flags & SF_STREAMING))
	{
	}

	return 0;
}


static int sound_update(Sound* s)
{
	ALint num_finished_buffers;
	alGetSourcei(s->al_source, AL_BUFFERS_PROCESSED, &num_finished_buffers);
	check();

	if(!(s->flags & SF_STREAMING) && num_finished_buffers == 1)
	{
		ALuint al_buffer;
		alSourceUnqueueBuffers(s->al_source, 1, &al_buffer);
		check();

		alSourceStop(s->al_source);
		check();

		sound_free(s->hs);
	}

	if(s->flags & SF_STREAMING)
		for(int i = 0; i < num_finished_buffers; i++)
		{
			ALuint al_buffer;
			alSourceUnqueueBuffers(s->al_source, 1, &al_buffer);
			alDeleteBuffers(1, &al_buffer);
			check();

			CHECK_ERR(io_issue(s, s->hf));
		}

	return 0;
}


int sound_play(Handle hs)
{
	H_DEREF(hs, Sound, s);
	alSourcePlay(s->al_source);
	check();
	return 0;
}


///////////////////////////////////////////////////////////////////////////////
//
// sound engine
//
///////////////////////////////////////////////////////////////////////////////

void snd_init()
{
	static bool initialized = false;
	if(initialized)
		return;
	initialized = true;

	oal_Init();

	if(!alIsExtensionPresent((ALubyte*)"AL_EXT_vorbis"))
		debug_warn("no OpenAL ogg extension");
	else
		ogg_supported = true;
}


int snd_update()
{
	CHECK_ERR(io_check_complete());

	CHECK_ERR(sounds_foreach(sound_update));

	return 0;
}

