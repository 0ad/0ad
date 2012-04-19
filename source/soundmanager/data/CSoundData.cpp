//
//  CSoundItem.cpp
//  SoundTester
//
//  Created by Steven Fuchs on 3/23/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#import "CSoundData.h"


#include <iostream>
//#include "AL/alut.h"
//#include "soundmanager/data/CWAVData.h"
#include "COggData.h"


DataMap* CSoundData::sSoundData = NULL;

CSoundData::CSoundData()
{
    initProperties();
}

CSoundData::~CSoundData()
{
    if ( mALBuffer != 0 )
        alDeleteBuffers( 1, &mALBuffer );
}

void CSoundData::initProperties()
{
    mALBuffer = 0;
    mRetentionCount = 0;
}

void CSoundData::releaseSoundData( CSoundData* theData )
{
    DataMap::iterator   itemFind;

    if ( theData->decrementCount() ) {
        if ( ( itemFind = CSoundData::sSoundData->find( theData->getFileName() ) ) != sSoundData->end() )
        {
            CSoundData* dier = itemFind->second;
            CSoundData::sSoundData->erase( itemFind );
            delete dier;
        }
    }
}

CSoundData* CSoundData::soundDataFromFile( OsPath& itemPath )
{
    if ( CSoundData::sSoundData == NULL )
        CSoundData::sSoundData = new DataMap;
    
    Path                fExt = itemPath.Extension();
    DataMap::iterator   itemFind;
    CSoundData*         answer;

    
    debug_printf(L"creating data at: %ls\n\n", itemPath.string().c_str());

    if ( ( itemFind = CSoundData::sSoundData->find( itemPath.string() ) ) != sSoundData->end() )
    {
        debug_printf(L"data found in cache at: %ls\n\n", itemPath.string().c_str());
        answer = itemFind->second;
    }
    else
    {
       	if ( fExt == ".ogg" )
            answer = soundDataFromOgg( itemPath );
// 		else if ( fExt == ".wav" )
//          answer = soundDataFromWAV( itemPath );

    
        if ( answer && answer->isOneShot() )
            (*CSoundData::sSoundData)[itemPath.string()] = answer;
    
    }
    return answer;
}

bool CSoundData::isOneShot()
{
    return true;
}


CSoundData* CSoundData::soundDataFromOgg(OsPath& itemPath )
{
    CSoundData* answer = NULL;
    COggData*   oggAnswer = new COggData();
    if ( oggAnswer->InitOggFile( itemPath.string().c_str() ) ) {
        answer = oggAnswer;
    }
    
    return answer;
}


//CSoundData* CSoundData::soundDataFromWAV( OsPath& itemPath )
//{
//    CSoundData* answer = NULL;
//    CWAVData*   wavAnswer = new CWAVData();

//    if ( wavAnswer->InitWAVFile( itemPath.string().c_str() ) ) {
//        answer = wavAnswer;
//    }
    
//    return answer;
//}

ALsizei CSoundData::getBufferCount()
{
    return 1;
}

std::wstring     CSoundData::getFileName()
{
    return mFileName;
}









CSoundData* CSoundData::incrementCount()
{
    mRetentionCount++;
    return this;
}

bool CSoundData::decrementCount()
{
    mRetentionCount--;
    
    return ( mRetentionCount <= 0 );
}

ALuint CSoundData::getBuffer()
{
    return mALBuffer;
}
ALuint* CSoundData::getBufferPtr()
{
    return &mALBuffer;
}

