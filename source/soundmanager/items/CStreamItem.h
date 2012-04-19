//
//  CMusicItem.h
//  SoundTester
//
//  Created by Steven Fuchs on 3/23/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#ifndef SoundTester_CStreamItem_h
#define SoundTester_CStreamItem_h

#include "soundmanager/data/CSoundData.h"
#include "CSoundBase.h"

class CStreamItem : public CSoundBase
{
public:
    CStreamItem                 (CSoundData* sndData);
    virtual ~CStreamItem        ();
    
    virtual void    setLooping  ( bool loops );
    virtual bool    idleTask    ();
    
protected:    
    virtual void    attach      ( CSoundData* itemData );

};

#endif
