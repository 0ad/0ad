//
//  CSoundItem.h
//  SoundTester
//
//  Created by Steven Fuchs on 3/23/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#ifndef SoundTester_CSoundData_h
#define SoundTester_CSoundData_h
#include "lib/os_path.h"

#include <OpenAL/al.h>
#include "string"
#include "map"

class CSoundData;
typedef std::map<std::wstring, CSoundData*> DataMap;



class CSoundData
{
public:
    static CSoundData* soundDataFromFile( OsPath& itemPath );
    static CSoundData* soundDataFromOgg( OsPath& itemPath );
//    static CSoundData* soundDataFromWAV( OsPath& itemPath );

    static void releaseSoundData( CSoundData* theData );

    CSoundData      ();
    CSoundData      (ALuint dataSource);
    virtual ~CSoundData     ();
    
    CSoundData*     incrementCount();
    bool            decrementCount();
    void            initProperties();
    virtual bool isOneShot();

    
    virtual ALuint      getBuffer();
    virtual ALsizei     getBufferCount();
    std::wstring        getFileName();
    virtual ALuint*     getBufferPtr();

protected:
    static     DataMap*  sSoundData;

    ALuint          mALBuffer;
    int             mRetentionCount;
    std::wstring    mFileName;
    
    
    
};






#endif
