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
    
public:
    CSoundBase      ();
    
    virtual ~CSoundBase      ();
    
    virtual bool    initOpenAL();
    virtual void    resetVars();

    virtual void setLastPlay( bool last );

    void    play            ();
    void    playAndDelete   ();
    bool    idleTask        ();
    void    playLoop        ();
    void    stop            ();
    void    stopAndDelete    ();

    void    playAsMusic      ();
    void    playAsAmbient    ();

    const char*   Name();

    virtual bool getLooping     ();
    virtual void setLooping     ( bool loops );

protected:

    void setNameFromPath(  char* fileLoc );

    
};






#endif
