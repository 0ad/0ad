// JS sound binding
//
// Jan Wassenberg (jan@wildfiregames.com)

#ifndef JSSOUND_INCLUDED
#define JSSOUND_INCLUDED

#include "scripting/ScriptableObject.h"
#include "lib/res/handle.h"

// ....
#undef free
// ....

class JSI_Sound : public CJSObject<JSI_Sound>
{
public:

	Handle m_Handle;

	// note: filename is stored by handle manager; no need to keep a copy here.

	JSI_Sound(const CStr& Filename);
	~JSI_Sound();

	// Script-bound functions

	jsval ToString( JSContext* cx, uintN argc, jsval* argv );

	// start playing the sound (one-shot).
	// it will automatically be freed when done.
	bool Play( JSContext* cx, uintN argc, jsval* argv );

	// request the sound be played until free() is called. returns immediately.
	bool Loop( JSContext* cx, uintN argc, jsval* argv );

	// stop sound if currently playing and free resources.
	// doesn't need to be called unless played via loop() -
	// sounds are freed automatically when done playing.
	bool Free( JSContext* cx, uintN argc, jsval* argv );

	bool SetGain( JSContext* cx, uintN argc, jsval* argv );

	bool SetPosition( JSContext* cx, uintN argc, jsval* argv );

	static JSBool Construct( JSContext* cx, JSObject* obj, unsigned int argc, jsval* argv, jsval* rval );

	static void ScriptingInit();
};

#endif	// #ifndef JSSOUND_INCLUDED
