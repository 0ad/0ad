#include "precompiled.h"

#include "CMusicPlayer.h"

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
		vorbisData->dataRead += actualOffset;
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

	return vorbisData->dataRead;
}



//Class implementation
CMusicPlayer::CMusicPlayer(void)
{
	info = NULL;
	source = NULL;
	format = NULL;

	for(int i = 0; i < 2; i++)
	{
		buffers[i] = NULL;
	}
}

CMusicPlayer::~CMusicPlayer(void)
{
	release();
}

void CMusicPlayer::open(char *filename)
{
	int ret = 0;
	format = NULL;
	info = NULL;

	FILE*   tempOggFile;
	int		sizeOfFile; 
	char	tempChar;
	int		tempArray;

    if(!(tempOggFile = fopen(filename, "rb")))
	{	
		printf("Unable to open Ogg file\n");
		exit(1);
	}
	sizeOfFile = 0;
	while (!feof(tempOggFile))
	{
		tempChar = getc(tempOggFile);
		sizeOfFile++;
	}

	memFile.dataPtr = new char[sizeOfFile];
	rewind(tempOggFile);
	tempArray = 0;
	while (!feof(tempOggFile))
	{
		memFile.dataPtr[tempArray] = getc(tempOggFile);
		tempArray++;
	}

	fclose(tempOggFile);

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
		printf("Could not decode ogg into memory\n");
		exit(1);
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
	
}

void CMusicPlayer::release()
{
	alSourceStop(source);
	empty();
	alDeleteSources(1,&source);
	check();
	alDeleteBuffers(1,buffers);
	check();
	ov_clear(&oggStream);
	delete[] memFile.dataPtr;
	memFile.dataPtr = NULL;

}


bool CMusicPlayer::play()
{
	check();
	//printf("returned from check at start of isPlaying()\n");
	//check if already playing
	if(isPlaying())
		return true;

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
	ALenum state = NULL;

	alGetSourcei(source,AL_SOURCE_STATE,&state);
	check();
	//printf("returning from isPlaying.\n");
	return (state == AL_PLAYING);
}


bool CMusicPlayer::update()
{
	int processed;
	bool active = true;

	//check which buffers have already been played
	alGetSourcei(source,AL_BUFFERS_PROCESSED, &processed);
	//printf("checking() buffers processed\n");
	check();
	//printf("check returned\n");
	
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
		std::cout << "OpenAL error " << error << std::endl;
		exit(1);
	}

}

void CMusicPlayer::empty()
{
	int queued;
    alGetSourcei(source,AL_BUFFERS_QUEUED,&queued);

	while(queued--)
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

		if(ret > 0)
		{
			size += ret;
		}
		else
		{
			if(ret < 0)
			{
				printf("Error reading from ogg file\n");
				exit(1);
			}
			else
			{
				break;
			}
		}
	}

	if(size == 0)
	{
		return false;
	}

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
        default:
			return std::string("Unknown Ogg error.");
    }
}