//
//  CBufferItem.cpp
//  SoundTester
//
//  Created by Steven Fuchs on 3/23/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//
#include "CBufferItem.h"
#include "soundmanager/data/CSoundData.h"

#include <iostream>
#import <OpenAL/al.h>

CBufferItem::CBufferItem(CSoundData* sndData)
{
    resetVars();
    if ( initOpenAL() )
        attach( sndData );

    debug_printf(L"created BufferItem at: %ls\n\n", sndData->getFileName().c_str());
}


CBufferItem::~CBufferItem()
{
    stop();
    int num_processed;
    alGetSourcei( mALSource, AL_BUFFERS_PROCESSED, &num_processed);
    
    if (num_processed > 0)
    {
        ALuint al_buf[num_processed];
        alSourceUnqueueBuffers(mALSource, num_processed, al_buf);
    }
}


bool CBufferItem::idleTask()
{
    if ( mLastPlay )
    {
        int proc_state;
        alGetSourceiv( mALSource, AL_SOURCE_STATE, &proc_state);
        return ( proc_state != AL_STOPPED );
    }
    
    if ( getLooping() ) {
        int num_processed;
        alGetSourcei( mALSource, AL_BUFFERS_PROCESSED, &num_processed);
        
        for ( int i = 0; i < num_processed; i++ )
        {
            ALuint al_buf;
            alSourceUnqueueBuffers(mALSource, 1, &al_buf);
            alSourceQueueBuffers(mALSource, 1, &al_buf);
        }
    }

    return true;
}

void CBufferItem::attach( CSoundData* itemData )
{
    if ( itemData != NULL ) {
        mSoundData = itemData->incrementCount();
        alSourceQueueBuffers(mALSource, mSoundData->getBufferCount(),(const ALuint *) mSoundData->getBufferPtr());
    }
}

void CBufferItem::setLooping( bool loops )
{
    mLooping = loops;
}

