//
//  CSoundItem.cpp
//  SoundTester
//
//  Created by Steven Fuchs on 3/23/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#include "COggData.h"


#include <wchar.h>
#include <iostream>

COggData::COggData()
{
    mOneShot = false;
}

COggData::~COggData()
{
    alDeleteBuffers( mBuffersUsed, mBuffer );
    ov_clear(&m_vf);
}

void COggData::setFormatAndFreq( int form, ALsizei freq)
{
    mFormat = form;
    mFrequency = freq;
}

bool COggData::InitOggFile( const wchar_t* fileLoc )
{
    int buffersToStart = 100;
    
#ifdef _WIN32
    _setmode( _fileno( stdin ), _O_BINARY );
    _setmode( _fileno( stdout ), _O_BINARY );
#endif

    fprintf(stderr, "ready to open ofgg file at:%ls \r\r", fileLoc); 

    char nameH[300];
    sprintf( nameH, "%ls", fileLoc );
    
    FILE* f = fopen( nameH, "rb");
    m_current_section = 0;
    int err = ov_open_callbacks(f, &m_vf, NULL, 0, OV_CALLBACKS_DEFAULT);
    if ( err < 0) {
        fprintf(stderr,"Input does not appear to be an Ogg bitstream :%d :%d.\n", err, ferror(f) );
        return false;
    }

    mFileName       = std::wstring(fileLoc);

    mFileFinished = false;
    setFormatAndFreq( (m_vf.vi->channels == 1)? AL_FORMAT_MONO16 : AL_FORMAT_STEREO16 , (ALsizei)m_vf.vi->rate );

    alGetError(); /* clear error */
    alGenBuffers( buffersToStart, mBuffer);
    
    if(alGetError() != AL_NO_ERROR) 
    {
        printf("- Error creating initial buffer !!\n");
        return false;
    }
    else
    {
        mBuffersUsed = fetchDataIntoBuffer( buffersToStart, mBuffer);
        if ( mFileFinished ) {
            mOneShot = true;
            if ( mBuffersUsed < buffersToStart ) {
                debug_printf(L"one shot gave back %d buffers\n\n", buffersToStart - mBuffersUsed );
                alDeleteBuffers( buffersToStart - mBuffersUsed, &mBuffer[mBuffersUsed] );
            }
        }
    }
    return true;
}

#define BUFFER_SIZE 65536
ALsizei COggData::getBufferCount()
{
    return mBuffersUsed;
}

bool COggData::isFileFinished()
{
    return mFileFinished;
}

void COggData::resetFile()
{
    ov_time_seek( &m_vf, 0 );
    m_current_section = 0;
    mFileFinished = false;
}

bool COggData::isOneShot()
{
    return mOneShot;
}

int COggData::fetchDataIntoBuffer( int count, ALuint* buffers)
{
    char pcmout[70000];
    int buffersWritten = 0;
    
    for(int i = 0; ( i < count ) && !mFileFinished; i++) {
        char*   readDest = pcmout;
        long  totalRet = 0;
        while (totalRet < BUFFER_SIZE )
        {
            long ret=ov_read(&m_vf,readDest, 4096,0,2,1, &m_current_section);
            if (ret == 0) {
                mFileFinished=true;
                break;
            } else if (ret < 0) {
                /* error in the stream.  Not a problem, just reporting it in
                 case we (the app) cares.  In this case, we don't. */
            } else {
                totalRet += ret;
                readDest += ret;
            }
        }
        if ( totalRet > 0 )
        {
            buffersWritten++;
            alBufferData( buffers[i], mFormat, pcmout, (ALsizei)totalRet, (int)mFrequency);
        }
    }

    return buffersWritten;
}


ALuint COggData::getBuffer()
{
    return mBuffer[0];
}
ALuint* COggData::getBufferPtr()
{
    return mBuffer;
}





