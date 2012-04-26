
//
//  Header.h
//  pyrogenesis
//
//  Created by Steven Fuchs on 4/15/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//
#ifndef SoundTester_ISoundItem_h
#define SoundTester_ISoundItem_h

#include <string>
#include <OpenAL/al.h>
#include "Vector3D.h"

class ISoundItem
{
    
public:
	virtual ~ISoundItem(){};
    virtual bool getLooping     () = 0;
    virtual void    setLooping       (bool loop) = 0;
    virtual bool    isPlaying        () = 0;
    
    
    virtual std::string    getName         () = 0;
    virtual bool    idleTask         () = 0;
    
    virtual void    play             () = 0;
    virtual void    stop             () = 0;

    virtual void    playAsMusic      () = 0;
    virtual void    playAsAmbient    () = 0;

    virtual void    playAndDelete    () = 0;
    virtual void    stopAndDelete    () = 0;
    virtual void    fadeToIn        ( ALfloat newVolume, double fadeDuration) = 0;
    virtual void    fadeAndDelete    ( double fadeTime ) = 0;
    virtual void    playLoop         () = 0;

    virtual void    setGain     (ALfloat gain) = 0;
    virtual void    setLocation (const CVector3D& position) = 0;
};


#endif //SoundTester_ISoundItem_h