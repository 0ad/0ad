#include "precompiled.h"
#include "JSI_Sound.h"

#include "lib/res/snd.h"
#include "lib/res/h_mgr.h"	// h_filename


JSI_Sound::JSI_Sound( const CStr& Filename )
{
	const char* fn = Filename.c_str();
	m_Handle = snd_open( fn );

	snd_set_pos( m_Handle, 0,0,0, true);
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
	AddMethod<bool, &JSI_Sound::Play>( "play", 0 );
	AddMethod<bool, &JSI_Sound::Loop>( "loop", 0 );
	AddMethod<bool, &JSI_Sound::Free>( "free", 0 );

	CJSObject<JSI_Sound>::ScriptingInit( "Sound", &JSI_Sound::Construct, 1 );
}

jsval JSI_Sound::ToString( JSContext* cx, uintN argc, jsval* argv )
{
	const char* Filename = h_filename(m_Handle);
	wchar_t buffer[256];
	swprintf( buffer, 256, L"[object Sound: %hs]", Filename );
	buffer[255] = 0;
	utf16string str16(buffer, buffer+wcslen(buffer));
	return( STRING_TO_JSVAL( JS_NewUCStringCopyZ( cx, str16.c_str() ) ) );
}

JSBool JSI_Sound::Construct( JSContext* cx, JSObject* obj, unsigned int argc, jsval* argv, jsval* rval )
{
	assert( argc >= 1 );
	CStrW filename;
	if( !ToPrimitive<CStrW>( cx, argv[0], filename ) )
		return( JS_FALSE );

	JSI_Sound* newObject = new JSI_Sound( filename );
	newObject->m_EngineOwned = false;
	*rval = OBJECT_TO_JSVAL( newObject->GetScript() );

	return( JS_TRUE );
}