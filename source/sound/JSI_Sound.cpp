#include "precompiled.h"
#include "JSI_Sound.h"

#include "lib/res/snd.h"
#include "lib/res/h_mgr.h"	// h_filename


JSI_Sound::JSI_Sound( const CStr& Filename )
{
	Handle m_Handle = snd_open(Filename.c_str());
}

JSI_Sound::~JSI_Sound()
{
	this->free();
}


// start playing the sound (one-shot).
// it will automatically be freed when done.
void JSI_Sound::play()
{
	snd_play(m_Handle);
}

// request the sound be played until free() is called. returns immediately.
void JSI_Sound::loop()
{
	snd_set_loop(m_Handle, true);
	snd_play(m_Handle);
}

// stop sound if currently playing and free resources.
// doesn't need to be called unless played via loop() -
// sounds are freed automatically when done playing.
void JSI_Sound::free()
{
	snd_free(m_Handle);	// resets it to 0
}


// Script-bound functions


void JSI_Sound::ScriptingInit()
{
	AddMethod<jsval, &JSI_Sound::ToString>( "toString", 0 );

	CJSObject<JSI_Sound>::ScriptingInit( "Sound" );
}

jsval JSI_Sound::ToString( JSContext* cx, uintN argc, jsval* argv )
{
	const char* Filename = h_filename(m_Handle);
	wchar_t buffer[256];
	swprintf( buffer, 256, L"[object Sound: %ls]", Filename );
	buffer[255] = 0;
	utf16string str16(buffer, buffer+wcslen(buffer));
	return( STRING_TO_JSVAL( JS_NewUCStringCopyZ( cx, str16.c_str() ) ) );
}
