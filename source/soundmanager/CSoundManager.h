//
//  CSoundManager.h
//  SoundTester
//
//  Created by Steven Fuchs on 3/21/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#ifndef SoundTester_CSoundManager_h
#define SoundTester_CSoundManager_h

#include "vector"
#include "map"
#include <OpenAL/al.h>
#include <OpenAL/alc.h>
#include "lib/os_path.h"

#include "soundmanager/items/ISoundItem.h"

typedef std::vector<ISoundItem*> ItemsList;


class CSoundManager
{
protected:

    ALuint              mALEnvironment;
    ALCcontext*         mContext;
    ALCdevice*          mDevice;
    ISoundItem*         mCurrentTune;
    ISoundItem*         mCurrentEnvirons;
    ItemsList*          mItems;
    OsPath              mResourcePath;
    float               mGain;
    float               mMusicGain;
    float               mAmbientGain;
    float               mActionGain;
    
public:
     CSoundManager      (OsPath& resourcePath);
    ~CSoundManager      ();

    ISoundItem* loadItem( OsPath& itemPath );

    static void ScriptingInit();

    
    
    ISoundItem*     itemFromWAV     ( OsPath& fname);
    ISoundItem*     itemFromOgg     ( OsPath& fname);

    ISoundItem*     getSoundItem    ( unsigned long itemRow );
    unsigned long   count           ();
    void            idleTask        ();
    void            deleteItem      ( long itemNum );


    void        setMusicItem( ISoundItem* anItem );
    void        setAmbientItem( ISoundItem* anItem );
    void        playActionItem( ISoundItem* anItem );

    void        setMasterGain( float gain);
    void        setMusicGain( float gain);
    void        setAmbientGain( float gain);
    void        setActionGain( float gain);
	
protected:
    void    InitListener();
	virtual Status alc_init();

};



extern CSoundManager*  g_SoundManager;







#endif
