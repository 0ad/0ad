//
//  CSoundItem.h
//  SoundTester
//
//  Created by Steven Fuchs on 3/23/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#ifndef SoundTester_CSoundItem_h
#define SoundTester_CSoundItem_h

#include "CSoundBase.h"
#include "soundmanager/data/CSoundData.h"


class CSoundItem :public CSoundBase
{
protected:
    
public:
    CSoundItem      ();
    CSoundItem      (CSoundData* sndData);
    
    virtual ~CSoundItem     ();
    void    attach          ( CSoundData* itemData );
    bool    idleTask        ();

protected:

    
};






#endif
