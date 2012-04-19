//
//  CMusicItem.h
//  SoundTester
//
//  Created by Steven Fuchs on 3/23/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#ifndef SoundTester_CBufferItem_h
#define SoundTester_CBufferItem_h

#include "CSoundBase.h"

class CBufferItem : public CSoundBase
{
public:
    CBufferItem             (CSoundData* sndData);
    virtual ~CBufferItem    ();
    
    virtual void    setLooping   ( bool loops );
    virtual bool    idleTask     ();
    
protected:    
    virtual void    attach       ( CSoundData* itemData );

    
};


#endif
