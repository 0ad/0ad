//
//  CSoundBase.h
//  SoundTester
//
//  Created by Steven Fuchs on 3/23/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#ifndef SoundTester_CSoundBase_h
#define SoundTester_CSoundBase_h

#include <string>
#include <OpenAL/al.h>
#include "soundmanager/items/ISoundItem.h"
#include "soundmanager/data/CSoundData.h"


class CSoundBase :public ISoundItem
{
protected:
    
    ALuint              mALSource;
    CSoundData*         mSoundData;
    std::string*        mName;
    bool                mLastPlay;
    bool                mLooping;
    
    double        mStartFadeTime;
    double        mEndFadeTime;
    ALfloat     mStartVolume;
    ALfloat     mEndVolume;

public:
    CSoundBase      ();
    
    virtual ~CSoundBase      ();
    
    virtual bool    initOpenAL();
    virtual void    resetVars();

    virtual void    setGain     (ALfloat gain);
    virtual void setLastPlay( bool last );

    void    play            ();
    void    playAndDelete   ();
    bool    idleTask        ();
    void    playLoop        ();
    void    stop            ();
    void    stopAndDelete    ();
    void    fadeToIn        ( ALfloat newVolume, double fadeDuration);

    void    playAsMusic      ();
    void    playAsAmbient    ();

    const char*   Name();
    std::string getName();

    virtual bool getLooping     ();
    virtual void setLooping     ( bool loops );
    virtual bool isPlaying();
    virtual void setLocation (const CVector3D& position);
    virtual void    fadeAndDelete    ( double fadeTime );

protected:

    void setNameFromPath(  char* fileLoc );
    void resetFade();
    bool handleFade();

    
};






#endif
