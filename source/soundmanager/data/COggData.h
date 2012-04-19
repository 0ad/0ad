//
//  CSoundItem.h
//  SoundTester
//
//  Created by Steven Fuchs on 3/23/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#ifndef SoundTester_COggData_h
#define SoundTester_COggData_h

#include <OpenAL/al.h>
#include "CSoundData.h"
#include "vorbis/vorbisfile.h"

class COggData : public CSoundData
{
    ALuint mFormat;
    long mFrequency;

public:
    COggData      ();
    virtual ~COggData      ();                                     
                                     
    virtual bool InitOggFile( const wchar_t* fileLoc );
    virtual bool isFileFinished();
    virtual bool isOneShot();

    virtual int fetchDataIntoBuffer( int count, ALuint* buffers);
    virtual void resetFile();

protected:
  	OggVorbis_File  m_vf;
    int            m_current_section;
    bool           mFileFinished;
    bool           mOneShot;
    ALuint         mBuffer[100];
    int            mBuffersUsed;

    bool addDataBuffer( char* data, long length);
    void setFormatAndFreq( int form, ALsizei freq);
    ALsizei  getBufferCount();
    ALuint getBuffer();
    ALuint* getBufferPtr();  
};



#endif
