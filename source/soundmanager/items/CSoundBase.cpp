//
//  CSoundBase.cpp
//  SoundTester
//
//  Created by Steven Fuchs on 3/23/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//
#include "CSoundBase.h"
#include "soundmanager/CSoundManager.h"
#include "soundmanager/data/CSoundData.h"

#include <iostream>
#include <OpenAL/al.h>

#include "lib/timer.h"


CSoundBase::CSoundBase()
{
    resetVars();
}

CSoundBase::~CSoundBase()
{
    stop();
    if ( mALSource != 0 ) {
        alDeleteSources( 1, &mALSource);
        mALSource = 0;
    }
    if ( mSoundData != 0 ) {
        CSoundData::releaseSoundData( mSoundData );
        mSoundData = 0;
    }
    if ( mName )
        delete mName;
}

void CSoundBase::resetVars()
{
    mALSource = 0;
    mSoundData = 0;
    mLastPlay = false;
    mLooping = false;
    mStartFadeTime = 0;
    mEndFadeTime = 0;
    mStartVolume = 0;
    mEndVolume = 0;
    resetFade();
    mName = new std::string( "sound name" );
}

void CSoundBase::resetFade()
{
    mStartFadeTime = 0;
    mEndFadeTime = 0;
    mStartVolume = 0;
    mEndVolume = 0;
}

void CSoundBase::setGain(ALfloat gain)
{
    alSourcef(mALSource, AL_GAIN, gain);
}



bool CSoundBase::initOpenAL()
{
    alGetError(); /* clear error */
    alGenSources( 1, &mALSource);
	long anErr = alGetError();
    if( anErr != AL_NO_ERROR) 
    {
        printf("- Error creating sources %ld !!\n", anErr );
    }
    else
    {
        ALfloat source0Pos[]={ -2.0, 0.0, 0.0};
        ALfloat source0Vel[]={ 0.0, 0.0, 0.0};
        
        alSourcef( mALSource,AL_PITCH,1.0f);
        alSourcef( mALSource,AL_GAIN,1.0f);
        alSourcefv( mALSource,AL_POSITION,source0Pos);
        alSourcefv( mALSource,AL_VELOCITY,source0Vel);
        alSourcei( mALSource,AL_LOOPING,AL_FALSE);
        return true;
    }
    return false;
}

bool CSoundBase::isPlaying()
{
    int proc_state;
    alGetSourceiv( mALSource, AL_SOURCE_STATE, &proc_state);

    return ( proc_state == AL_PLAYING );
}

void CSoundBase::setLastPlay( bool last )
{
    mLastPlay = last;
}

bool CSoundBase::idleTask()
{
	return true;
}

void CSoundBase::setLocation (const CVector3D& position)
{    
    const float*  loatArr = position.GetFloatArray();
    debug_printf(L"do upload and play at location:%f, %f, %f\n\n", loatArr[0], loatArr[1], loatArr[2] );

    
//    alSourcefv( mALSource,AL_POSITION, position.GetFloatArray() );
}

bool CSoundBase::handleFade()
{
    if ( mStartFadeTime != 0 ) {
        double currTime = timer_Time();
        double pctDone = std::min( 1.0, (currTime - mStartFadeTime) / (mEndFadeTime - mStartFadeTime) );
        pctDone = std::max( 0.0, pctDone );
        ALfloat curGain = ((mEndVolume - mStartVolume ) * pctDone) + mStartVolume;
        
        debug_printf(L"currtime is: %f\npctDone is: %f\ncurGain is %f", currTime,pctDone, curGain );

        if  (curGain == 0 )
            stop();
        else if ( curGain == mEndVolume ) {
            alSourcef( mALSource, AL_GAIN, curGain);     
            resetFade();
        }
        else
            alSourcef( mALSource, AL_GAIN, curGain);     
    }
	return true;
}

bool CSoundBase::getLooping()
{
    return mLooping;
}
void CSoundBase::setLooping( bool loops )
{
    mLooping = loops;
    alSourcei( mALSource, AL_LOOPING, loops ? AL_TRUE : AL_FALSE );
}

void CSoundBase::play()
{
    if ( mALSource != 0 )
        alSourcePlay( mALSource );
}
void CSoundBase::playAndDelete()
{
    setLastPlay( true );
    if ( mALSource != 0 )
        alSourcePlay( mALSource );
}

void CSoundBase::fadeAndDelete( double fadeTime )
{
    setLastPlay( true );
    fadeToIn( 0, fadeTime );
}

void    CSoundBase::stopAndDelete()
{
    setLastPlay( true );
    stop();
}

void CSoundBase::playLoop()
{
    if ( mALSource != 0 ) {
        setLooping( true );
        play();
    }
}

void CSoundBase::fadeToIn( ALfloat newVolume, double fadeDuration)
{
    int proc_state;
    alGetSourceiv( mALSource, AL_SOURCE_STATE, &proc_state);
    if ( proc_state == AL_PLAYING ) {
        mStartFadeTime = timer_Time();
        mEndFadeTime = mStartFadeTime + fadeDuration;
        alGetSourcef( mALSource, AL_GAIN, &mStartVolume);
        mEndVolume = newVolume;
    }

}

void CSoundBase::playAsMusic()
{
    g_SoundManager->setMusicItem( this );
}

void CSoundBase::playAsAmbient()
{
    g_SoundManager->setAmbientItem( this );
}

void CSoundBase::stop()
{
    if ( mALSource != 0 ) {
        int proc_state;
        alSourcei( mALSource, AL_LOOPING, AL_FALSE );
        alGetSourceiv( mALSource, AL_SOURCE_STATE, &proc_state);
        if ( proc_state == AL_PLAYING )
            alSourceStop( mALSource );
    }
}

const char* CSoundBase::Name()
{
    return mName->c_str();
}

std::string CSoundBase::getName()
{
    return std::string( mName->c_str() );
}

void CSoundBase::setNameFromPath(  char* fileLoc )
{
    std::string anst( fileLoc );
    size_t pos = anst.find_last_of("/");
    if(pos != std::wstring::npos)
        mName->assign(anst.begin() + pos + 1, anst.end());
    else
        mName->assign(anst.begin(), anst.end());
}

