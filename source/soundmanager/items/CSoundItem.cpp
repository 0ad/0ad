//
//  CSoundItem.cpp
//  SoundTester
//
//  Created by Steven Fuchs on 3/23/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//
#include "CSoundItem.h"
#include "soundmanager/data/CSoundData.h"

#include <iostream>
#import <OpenAL/al.h>


CSoundItem::CSoundItem()
{
    resetVars();
}

CSoundItem::CSoundItem(CSoundData* sndData)
{
    resetVars();
    if ( initOpenAL() )
        attach( sndData );

    debug_printf(L"created SoundItem at: %ls\n\n", sndData->getFileName().c_str());
}

CSoundItem::~CSoundItem()
{
    ALuint al_buf;
    
    stop();
    alSourceUnqueueBuffers(mALSource, 1, &al_buf);
}

bool CSoundItem::idleTask()
{
    handleFade();

    if ( mLastPlay )
    {
        int proc_state;
        alGetSourceiv( mALSource, AL_SOURCE_STATE, &proc_state);
        return ( proc_state != AL_STOPPED );
    }
    return true;
}

void CSoundItem::attach( CSoundData* itemData )
{
    if ( itemData != NULL ) {
        mSoundData = itemData->incrementCount();
        alSourcei( mALSource, AL_BUFFER, mSoundData->getBuffer() );
    }
}
