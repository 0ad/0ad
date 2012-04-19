
//
//  Header.h
//  pyrogenesis
//
//  Created by Steven Fuchs on 4/15/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//
#ifndef SoundTester_ISoundItem_h
#define SoundTester_ISoundItem_h



class ISoundItem
{
    
public:
	virtual ~ISoundItem(){};
    virtual bool    idleTask         () = 0;
    
    virtual void    play             () = 0;
    virtual void    stop             () = 0;

    virtual void    playAsMusic      () = 0;
    virtual void    playAsAmbient    () = 0;

    virtual void    playAndDelete    () = 0;
    virtual void    stopAndDelete    () = 0;
    virtual void    playLoop         () = 0;

    
};


#endif //SoundTester_ISoundItem_h