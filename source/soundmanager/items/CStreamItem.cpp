//
//  CStreamItem.cpp
//  SoundTester
//
//  Created by Steven Fuchs on 3/23/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//
#include "CStreamItem.h"
#include "soundmanager/data/COggData.h"

#include <iostream>
#import <OpenAL/al.h>

CStreamItem::CStreamItem(CSoundData* sndData)
{
    resetVars();
    if ( initOpenAL() )
        attach( sndData );

    debug_printf(L"created StreamItem at: %ls\n\n", sndData->getFileName().c_str());
}

CStreamItem::~CStreamItem()
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

bool CStreamItem::idleTask()
{
    int proc_state;
    alGetSourceiv( mALSource, AL_SOURCE_STATE, &proc_state);
    
    if ( proc_state == AL_STOPPED ) {
        if ( mLastPlay )
            return ( proc_state != AL_STOPPED );
    }
    else {
        COggData* tmp = (COggData*)mSoundData;
        
        if ( ! tmp->isFileFinished() ) {
            int num_processed;
            alGetSourcei( mALSource, AL_BUFFERS_PROCESSED, &num_processed);
            
            if (num_processed > 0)
            {
                ALuint al_buf[num_processed];
                alSourceUnqueueBuffers(mALSource, num_processed, al_buf);
                int didWrite = tmp->fetchDataIntoBuffer( num_processed, al_buf);
                alSourceQueueBuffers( mALSource, didWrite, al_buf);
            }
        }
        else if ( getLooping() )
        {
            tmp->resetFile();
        }
    }
    return true;
}

void CStreamItem::attach( CSoundData* itemData )
{
    if ( itemData != NULL ) {
        mSoundData = itemData->incrementCount();
        alSourceQueueBuffers(mALSource, mSoundData->getBufferCount(), (const ALuint *)mSoundData->getBufferPtr());
    }
}

void CStreamItem::setLooping( bool loops )
{
    mLooping = loops;
}

