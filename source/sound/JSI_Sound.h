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

	// start playing the sound (one-shot).
	// it will automatically be freed when done.
	void play();

	// request the sound be played until free() is called. returns immediately.
	void loop();

	// stop sound if currently playing and free resources.
	// doesn't need to be called unless played via loop() -
	// sounds are freed automatically when done playing.
	void free();


	// Script-bound functions

	jsval ToString( JSContext* cx, uintN argc, jsval* argv );
	bool Play( JSContext* cx, uintN argc, jsval* argv ) { play(); return( true ); }
	bool Loop( JSContext* cx, uintN argc, jsval* argv ) { loop(); return( true ); }
	bool Free( JSContext* cx, uintN argc, jsval* argv ) { free(); return( true ); }
	static JSBool Construct( JSContext* cx, JSObject* obj, unsigned int argc, jsval* argv, jsval* rval );

	static void ScriptingInit();
};

#endif	// #ifndef JSSOUND_INCLUDED
