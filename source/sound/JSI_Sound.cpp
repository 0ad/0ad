#include "precompiled.h"
#include "JSI_Sound.h"
#include "Vector3D.h"

#include "lib/res/snd.h"
#include "lib/res/h_mgr.h"	// h_filename


JSI_Sound::JSI_Sound( const CStr& Filename )
{
	const char* fn = Filename.c_str();
	m_Handle = snd_open( fn );
	// open failed. raising an exception is the only way to report errors,
	// since we're in the ctor and don't want to move the open call elsewhere
	// (by requiring an explicit open() call).
	// will be caught by JSI_Sound::Construct.
	if(m_Handle <= 0)
		throw (int)m_Handle;

	snd_set_pos( m_Handle, 0,0,0, true);
}

JSI_Sound::~JSI_Sound()
{
	this->Free(0, 0, 0);
}


bool JSI_Sound::SetGain( JSContext* cx, uintN argc, jsval* argv )
{
	assert( argc >= 1 );
	float gain;
	if( !ToPrimitive<float>( cx, argv[0], gain) )
		return false;

	snd_set_gain(m_Handle, gain);
	return true;
}

bool JSI_Sound::SetPitch( JSContext* cx, uintN argc, jsval* argv )
{
	assert( argc >= 1 );
	float pitch;
	if( !ToPrimitive<float>( cx, argv[0], pitch) )
		return false;

	snd_set_pitch(m_Handle, pitch);
	return true;
}

bool JSI_Sound::SetPosition( JSContext* cx, uintN argc, jsval* argv )
{
	assert( argc >= 1 );
	CVector3D pos;
	// absolute world coords
	if( ToPrimitive<CVector3D>( cx, argv[0], pos ) )
		snd_set_pos(m_Handle, pos[0], pos[1], pos[2]);
	// relative, 0 offset - right on top of the listener
	// (we don't need displacement from the listener, e.g. always behind)
	else
		snd_set_pos(m_Handle, 0,0,0, true);

	return true;
}

// start playing the sound (one-shot).
// it will automatically be freed when done.
bool JSI_Sound::Play( JSContext* cx, uintN argc, jsval* argv )
{
	snd_play(m_Handle);
	return true;
}

// request the sound be played until free() is called. returns immediately.
bool JSI_Sound::Loop( JSContext* cx, uintN argc, jsval* argv )
{
	snd_set_loop(m_Handle, true);
	snd_play(m_Handle);
	return true;
}

// stop sound if currently playing and free resources.
// doesn't need to be called unless played via loop() -
// sounds are freed automatically when done playing.
bool JSI_Sound::Free( JSContext* cx, uintN argc, jsval* argv )
{
	snd_free(m_Handle);	// resets it to 0
	return true;
}


// Script-bound functions


void JSI_Sound::ScriptingInit()
{
	AddMethod<jsval, &JSI_Sound::ToString>( "toString", 0 );
	AddMethod<bool, &JSI_Sound::Play>( "play", 0 );
	AddMethod<bool, &JSI_Sound::Loop>( "loop", 0 );
	AddMethod<bool, &JSI_Sound::Free>( "free", 0 );
	AddMethod<bool, &JSI_Sound::SetGain>( "setGain", 0 );
	AddMethod<bool, &JSI_Sound::SetPitch>( "setPitch", 0 );
	AddMethod<bool, &JSI_Sound::SetPosition>( "setPosition", 0 );

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

	try
	{
		JSI_Sound* newObject = new JSI_Sound( filename );
		newObject->m_EngineOwned = false;
		*rval = OBJECT_TO_JSVAL( newObject->GetScript() );
	}
	catch(int)
	{
		// failed, but this can happen easily enough that we don't want to
		// return JS_FALSE (since that stops the script).
		*rval = JSVAL_NULL;
	}

	return( JS_TRUE );
}
