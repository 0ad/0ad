#include "precompiled.h"

#include "CMusicPlayer.h"
#include <CLogger.h>
#include <sstream>

#ifdef _MSC_VER
# ifdef NDEBUG
#  pragma comment(lib, "ogg.lib")
#  pragma comment(lib, "vorbis.lib")
#  pragma comment(lib, "vorbisfile.lib")
# else
#  pragma comment(lib, "ogg_d.lib")
#  pragma comment(lib, "vorbis_d.lib")
#  pragma comment(lib, "vorbisfile_d.lib")
# endif
#endif


#include "res/res.h"

size_t VorbisRead(void *ptr, size_t byteSize, size_t sizeToRead, void *datasource)
{
	size_t spaceToEOF;			
	size_t actualSizeToRead;	
	SOggFile* vorbisData;			

	vorbisData = (SOggFile*)datasource;

	spaceToEOF = vorbisData->dataSize - vorbisData->dataRead;
	if ((sizeToRead*byteSize) < spaceToEOF)
		actualSizeToRead = (sizeToRead*byteSize);
	else
		actualSizeToRead = spaceToEOF;	
	
	if (actualSizeToRead)
	{
		memcpy(ptr, (char*)vorbisData->dataPtr + vorbisData->dataRead, actualSizeToRead);
		vorbisData->dataRead += (actualSizeToRead);
	}

	return actualSizeToRead;
}

int VorbisSeek(void *datasource, ogg_int64_t offset, int fromWhere)
{
	size_t spaceToEOF;	
	ogg_int64_t actualOffset;
	SOggFile* vorbisData;
	
	vorbisData = (SOggFile*)datasource;

	switch (fromWhere)
	{
	case SEEK_SET: 
		
		if (vorbisData->dataSize >= offset)
			actualOffset = offset;
		else
			actualOffset = vorbisData->dataSize;
		
		vorbisData->dataRead = (int)actualOffset;
		break;
	case SEEK_CUR: // Seek from where we are
		spaceToEOF = vorbisData->dataSize - vorbisData->dataRead;
		if (offset < spaceToEOF)
			actualOffset = (offset);
		else
			actualOffset = spaceToEOF;	
		vorbisData->dataRead += (size_t)actualOffset;
		break;
	case SEEK_END: 
		vorbisData->dataRead = vorbisData->dataSize+1;
		break;
	default:
		printf("*** ERROR *** Unknown seek command in VorbisSeek\n");
		break;
	};

	return 0;
}

int VorbisClose(void *datasource) 
{
	return 1;
}

long VorbisTell(void *datasource)
{
	SOggFile* vorbisData;
	vorbisData = (SOggFile*)datasource;

	return (long)vorbisData->dataRead;
}



//Class implementation
CMusicPlayer::CMusicPlayer(void)
{
	info = NULL;
	source = 0;
	format = 0;

	buffers[0] = buffers[1] = 0;
}

CMusicPlayer::~CMusicPlayer(void)
{
	release();
}

void CMusicPlayer::open(char *filename)
{
	oal_Init();

	// If a new file is opened while another is already in memory,
	// close the old one first.
	if (is_open)
		release();

	int ret = 0;
	format = 0;
	info = NULL;

	void* p;
	size_t sizeOfFile;
	if(vfs_load(filename, p, sizeOfFile) <= 0)
	{
		LOG(ERROR, "CMusicPlayer::open(): vfs_load for %s failed!\n", filename);
		return;
	}
	else
		LOG(NORMAL, "CMusicPlayer::open(): file %s loaded successfully\n", filename);

	memFile.dataPtr = (char*)p;
	memFile.dataRead = 0;
	memFile.dataSize = sizeOfFile;	
	

	//set up callbacks
	vorbisCallbacks.read_func = VorbisRead;
	vorbisCallbacks.close_func = VorbisClose;
	vorbisCallbacks.seek_func = VorbisSeek;
	vorbisCallbacks.tell_func = VorbisTell;

	//start file to from memory
	if (ov_open_callbacks(&memFile, &oggStream, NULL, 0, vorbisCallbacks) != 0)
	{
		LOG(ERROR, "Could not decode ogg into memory\n");
		return;
	}
	
	info = ov_info(&oggStream, -1);
	
	if(info->channels == 1)
	{
		//hopefully not mono
		format = AL_FORMAT_MONO16;
	}
	else
	{
		format = AL_FORMAT_STEREO16;
	}

	alGenBuffers(2,buffers);
	//printf("Gen buffers calling check()\n");
	check();
	//printf("Gen sources calling check()\n");
	alGenSources(1,&source);
	check();
	//printf("check returned from sources\n");

	alSource3f(source,AL_POSITION,0.0,0.0,0.0);
	check();
	//printf("returned from POSITION\n");
	alSource3f(source,AL_VELOCITY,0.0,0.0,0.0);
	check();
	//printf("returned from VELOCITY\n");
	alSource3f(source,AL_DIRECTION,0.0,0.0,0.0);
	check();
	//printf("returned from DIRECTION\n");
	alSourcef(source,AL_ROLLOFF_FACTOR,0.0);
	check();
	//printf("returned from ROLLOFF\n");
	alSourcei(source,AL_SOURCE_RELATIVE,AL_TRUE);
	check();
	//printf("returned from S RELATIVE\n");

	is_open = true;
}

void CMusicPlayer::release()
{
	if(!is_open)
		return;
	is_open = false;

	alSourceStop(source);
	empty();
	alDeleteSources(1,&source);
	check();
	source = 0;

	alDeleteBuffers(1,buffers);
	check();
	buffers[0] = buffers[1] = 0;

	ov_clear(&oggStream);

	mem_free(memFile.dataPtr);
}


bool CMusicPlayer::play()
{
	check();
	//printf("returned from check at start of isPlaying()\n");
	//check if already playing
	if(isPlaying())
		return true;

	if(!is_open)
		debug_warn("play() called before open()");

	//printf("calling stream on first buffer\n");
	//load data into 1st buffer
	if(!stream(buffers[0]))
		return false;

	//printf("calling stream on second buffer\n");
	//load data into 2nd buffer
	if(!stream(buffers[1]))
		return false;

	//bind the 2 buffers to the source
	alSourceQueueBuffers(source,2,buffers);
	check();
	//printf("check returned from Queuebuffers\n");
	alSourcePlay(source);
	check();
	//printf("check returned from SourcePlay\n");

	return true;
}


bool CMusicPlayer::isPlaying()
{
	// guard against OpenAL using source when it's not initialized
	if(!is_open)
		return false;

	ALenum state = 0;
	alGetSourcei(source,AL_SOURCE_STATE,&state);
	check();
	//printf("returning from isPlaying.\n");
	return (state == AL_PLAYING);
}


bool CMusicPlayer::update()
{
	if(!isPlaying())
		return false;

	//check which buffers have already been played
	int processed;
	alGetSourcei(source,AL_BUFFERS_PROCESSED, &processed);
	//printf("checking() buffers processed\n");
	check();
	//printf("check returned\n");

	bool active = true;

	while(processed-- && processed >= 0)
	{
		ALuint buffer;
		
		//remove buffer from queue
		alSourceUnqueueBuffers(source,1,&buffer);
		check();

		//fill buffer with new data if false is returned the there is no more data
		active = stream(buffer);

		//attach buffer to end of queue
		alSourceQueueBuffers(source,1,&buffer);
		check();
	}
	
	return active;
}


void CMusicPlayer::check()
{
	int error = alGetError();

	if(error != AL_NO_ERROR)
	{
		std::string str = errorString(error);
		LOG(ERROR, "OpenAL error: %s\n", str.c_str());
	}
}


void CMusicPlayer::empty()
{
	int queued;
    alGetSourcei(source,AL_BUFFERS_QUEUED,&queued);

	while(queued-- > 0)
	{
		ALuint buffer;

		alSourceUnqueueBuffers(source,1,&buffer);
		check();
	}
}


bool CMusicPlayer::stream(ALuint buffer)
{
	char data[AUDIO_BUFFER_SIZE];
	int size = 0;
	int section, ret;

	while(size < AUDIO_BUFFER_SIZE)
	{
		ret = ov_read(&oggStream, data + size, AUDIO_BUFFER_SIZE - size,0,2,1,&section);

		// success
		if(ret > 0)
			size += ret;
		// error
		else if(ret < 0)
		{
			LOG(ERROR, "Error reading from ogg file: %s\n", errorString(ret).c_str());
			return false;
		}
		// EOF
		else
			break;
	}

	// nothing was read
	if(size == 0)
		return false;

	alBufferData(buffer,format,data,size,info->rate);
	//printf("calling check in stream()\n");
	check();
	//printf("check returned\n");

	return true;
}


std::string CMusicPlayer::errorString(int errorcode)
{
	switch(errorcode)
    {
        case OV_EREAD:
			return std::string("Read from media.");
        case OV_ENOTVORBIS:
			return std::string("Not Vorbis data.");
        case OV_EVERSION:
			return std::string("Vorbis version mismatch.");
        case OV_EBADHEADER:
			return std::string("Invalid Vorbis header.");
        case OV_EFAULT:
			return std::string("Internal logic fault (bug or heap/stack corruption.");
		case OV_EINVAL:
			return std::string("Invalid argument passed to Vorbis function");
		case AL_INVALID_NAME:
			return "AL_INVALID_NAME";
		case AL_INVALID_ENUM:
			return "AL_INVALID_ENUM";
		case AL_INVALID_VALUE:
			return "AL_INVALID_VALUE";
		case AL_INVALID_OPERATION:
			return "AL_INVALID_OPERATION";
		case AL_OUT_OF_MEMORY:
			return "AL_OUT_OF_MEMORY";
        default:
			std::stringstream str;
			str << "Unknown Ogg error (code "<< errorcode << ")";
			return str.str();
    }
}
