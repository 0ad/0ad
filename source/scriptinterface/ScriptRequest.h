/* Copyright (C) 2021 Wildfire Games.
 * This file is part of 0 A.D.
 *
 * 0 A.D. is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * 0 A.D. is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 0 A.D.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef INCLUDED_SCRIPTREQUEST
#define INCLUDED_SCRIPTREQUEST

#include "scriptinterface/ScriptForward.h"

// Ignore warnings in SM headers.
#if GCC_VERSION || CLANG_VERSION
# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Wunused-parameter"
# pragma GCC diagnostic ignored "-Wnon-virtual-dtor"
#elif MSC_VERSION
# pragma warning(push, 1)
#endif

#include "js/RootingAPI.h"

#if GCC_VERSION || CLANG_VERSION
# pragma GCC diagnostic pop
#elif MSC_VERSION
# pragma warning(pop)
#endif

#include <memory>

class ScriptInterface;

/**
 * Spidermonkey maintains some 'local' state via the JSContext* object.
 * This object is an argument to most JSAPI functions.
 * Furthermore, this state is Realm (~ global) dependent. For many reasons, including GC safety,
 * The JSContext* Realm must be set up correctly when accessing it.
 * 'Entering' and 'Leaving' realms must be done in a LIFO manner.
 * SM recommends using JSAutoRealm, which provides an RAII option.
 *
 * ScriptRequest combines both of the above in a single convenient package,
 * providing safe access to the JSContext*, the global object, and ensuring that the proper realm has been entered.
 * Most scriptinterface/ functions will take a ScriptRequest, to ensure proper rooting. You may sometimes
 * have to create one from a ScriptInterface.
 *
 * Be particularly careful when manipulating several script interfaces.
 */
class ScriptRequest
{
	ScriptRequest() = delete;
	ScriptRequest(const ScriptRequest& rq) = delete;
	ScriptRequest& operator=(const ScriptRequest& rq) = delete;
public:
	/**
	 * NB: the definitions are in scriptinterface.cpp, because these access members of the PImpled
	 * implementation of ScriptInterface, and that seemed more convenient.
	 */
	ScriptRequest(const ScriptInterface& scriptInterface);
	ScriptRequest(const ScriptInterface* scriptInterface) : ScriptRequest(*scriptInterface) {}
	ScriptRequest(std::shared_ptr<ScriptInterface> scriptInterface) : ScriptRequest(*scriptInterface) {}
	~ScriptRequest();

	JS::Value globalValue() const;
	JSContext* cx;
	JS::HandleObject glob;
	JS::HandleObject nativeScope;
private:
	JS::Realm* m_formerRealm;
};


#endif // INCLUDED_SCRIPTREQUEST
