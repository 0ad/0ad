//
//  CSoundManager.cpp
//  SoundTester
//
//  Created by Steven Fuchs on 3/21/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#include <OpenAL/al.h>
#include "CSoundManager.h"
#include "soundmanager/items/CSoundItem.h"
#include "soundmanager/items/CBufferItem.h"
#include "soundmanager/items/CStreamItem.h"
#include "soundmanager/js/JAmbientSound.h"
#include "soundmanager/js/JMusicSound.h"
#include "soundmanager/js/JSound.h"
#include "soundmanager/data/CSoundData.h"


CSoundManager*  g_SoundManager;

void CSoundManager::ScriptingInit()
{
    JAmbientSound::ScriptingInit();
    JMusicSound::ScriptingInit();
    JSound::ScriptingInit();
}

CSoundManager::CSoundManager(OsPath& resourcePath)
{
    mResourcePath = resourcePath;
    
    mItems = new ItemsList;
    mCurrentEnvirons    = 0;
    mCurrentTune        = 0;
    mGain               = 1;
    mMusicGain          = 1;
    mAmbientGain        = 1;
    mActionGain         = 1;
    mEnabled            = true;
    
    debug_printf(L"initiate manager at: %ls\n\n", resourcePath.string().c_str());

	alc_init();
}

CSoundManager::~CSoundManager()
{
    alcDestroyContext( mContext );
    alcCloseDevice( mDevice );
    
    delete mItems;
}


Status CSoundManager::alc_init()
{
	ALCdevice *alc_dev;
	ALCcontext *alc_ctx;
	
	Status ret = INFO::OK;

	alc_dev = alcOpenDevice(NULL);
	if(alc_dev)
	{
		alc_ctx = alcCreateContext(alc_dev, 0);	// no attrlist needed
		if(alc_ctx)
			alcMakeContextCurrent(alc_ctx);
	}

	// check if init succeeded.
	// some OpenAL implementations don't indicate failure here correctly;
	// we need to check if the device and context pointers are actually valid.
	ALCenum err = alcGetError(alc_dev);
	if(err != ALC_NO_ERROR || !alc_dev || !alc_ctx)
	{
//		debug_printf(L"alc_init failed. alc_dev=%p alc_ctx=%p alc_dev_name=%hs err=%d\n", alc_dev, alc_ctx, alc_dev_name, err);
// FIXME Hack to get around exclusive access to the sound device
#if OS_UNIX
		ret = INFO::OK;
#else
		ret = ERR::FAIL;
#endif
	}

	// make note of which sound device is actually being used
	// (e.g. DS3D, native, MMSYSTEM) - needed when reporting OpenAL bugs.
	const char* dev_name = (const char*)alcGetString(alc_dev, ALC_DEVICE_SPECIFIER);
	wchar_t buf[200];
	swprintf(buf, ARRAY_SIZE(buf), L"SND| alc_init: success, using %hs\n", dev_name);
//	ah_log(buf);

	return ret;
}

void CSoundManager::setMasterGain( float gain)
{
    mGain = gain;
}
void CSoundManager::setMusicGain( float gain)
{
    mMusicGain = gain;
}
void CSoundManager::setAmbientGain( float gain)
{
    mAmbientGain = gain;
}
void CSoundManager::setActionGain( float gain)
{
    mActionGain = gain;
}


ISoundItem* CSoundManager::loadItem( OsPath& itemPath )
{
    debug_printf(L"initiate item at: %ls\n\n", itemPath.string().c_str());
    
    OsPath thePath = mResourcePath/itemPath;
    
    CSoundData*   itemData = CSoundData::soundDataFromFile( thePath );
    ISoundItem*   answer  = NULL;
    
    if ( itemData != NULL ) {
        if ( itemData->isOneShot() ) {
            if ( itemData->getBufferCount() == 1 )
                answer = new CSoundItem( itemData );
            else
                answer = new CBufferItem( itemData );
        }
        else {
            answer = new CStreamItem( itemData );
        }

        if ( answer != NULL )
            mItems->push_back( answer );
    }

    
    return answer;
}

unsigned long CSoundManager::count()
{
    return mItems->size();
}

void CSoundManager::idleTask()
{
    ItemsList::iterator lstr = mItems->begin();
    ItemsList  deadItemList;
    ItemsList* nextItemList = new ItemsList;
    
    
    while ( lstr != mItems->end() ) {
        if ( (*lstr)->idleTask() )
            nextItemList->push_back( *lstr );
        else
            deadItemList.push_back( *lstr );
        lstr++;
    }
    delete mItems;
    mItems = nextItemList;
    
    ItemsList::iterator deadItems = deadItemList.begin();
    while ( deadItems != deadItemList.end() )
    {   
        delete *deadItems;
        deadItems++;
    }

}

void CSoundManager::deleteItem( long itemNum )
{
    ItemsList::iterator lstr = mItems->begin();
    lstr += itemNum;
    
    delete *lstr;
    
    mItems->erase( lstr );
}

ISoundItem* CSoundManager::getSoundItem( unsigned long itemRow )
{
   return (*mItems)[itemRow];
}

void CSoundManager::InitListener()
{
    ALfloat listenerPos[]={0.0,0.0,4.0};
    ALfloat listenerVel[]={0.0,0.0,0.0};
    ALfloat	listenerOri[]={0.0,0.0,1.0, 0.0,1.0,0.0};

    alListenerfv(AL_POSITION,listenerPos);
    alListenerfv(AL_VELOCITY,listenerVel);
    alListenerfv(AL_ORIENTATION,listenerOri);
}
void CSoundManager::setEnabled( bool doEnable )
{
    mEnabled = doEnable;
}

void CSoundManager::playActionItem( ISoundItem* anItem )
{
    if ( anItem )
    {
        if ( mEnabled && ( mActionGain > 0 ) ) {
            anItem->setGain( mGain * mActionGain );
            anItem->play();
        }
    }
}
void CSoundManager::setMusicItem( ISoundItem* anItem )
{
    if ( mCurrentTune ) {
        mCurrentTune->fadeAndDelete(5.00);
        mCurrentTune = NULL;
    }
    idleTask();
    if ( anItem )
    {
        if ( mEnabled && ( mMusicGain > 0 ) ) {
            mCurrentTune = anItem;
            mCurrentTune->setGain( 0 );
            mCurrentTune->playLoop();
            mCurrentTune->fadeToIn( mGain * mMusicGain, 3.00 );
        }
    }
}

void CSoundManager::setAmbientItem( ISoundItem* anItem )
{
    if ( mCurrentEnvirons ) {
        mCurrentEnvirons->stop();
        mCurrentEnvirons = NULL;
    }
    idleTask();
    
    if ( anItem )
    {
        if ( mEnabled && ( mAmbientGain > 0 ) ) {
            mCurrentEnvirons = anItem;
            mCurrentEnvirons->setGain( 0 );
            mCurrentEnvirons->playLoop();
            mCurrentTune->fadeToIn( mGain * mAmbientGain, 3.00 );
        }
    }
}

