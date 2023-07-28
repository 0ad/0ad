/* Copyright (C) 2021 Wildfire Games.
 * This file is part of 0 A.D.
 *
 * 0 A.D. is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * 0 A.D. is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 0 A.D.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef INCLUDED_JSINTERFACE_L10N
#define INCLUDED_JSINTERFACE_L10N

class ScriptRequest;

/**
 * Namespace for the functions of the JavaScript interface for
 * internationalization and localization.
 *
 * This namespace defines JavaScript interfaces to functions defined in L10n and
 * related helper functions.
 *
 * @sa http://trac.wildfiregames.com/wiki/Internationalization_and_Localization
 */
namespace JSI_L10n
{
	/**
	 * Registers the functions of the JavaScript interface for
	 * internationalization and localization into the specified JavaScript
	 * context.
	 *
	 * @param ScriptRequest Script Request where RegisterScriptFunctions()
	 *        registers the functions.
	 *
	 * @sa GuiScriptingInit()
	 */
	void RegisterScriptFunctions(const ScriptRequest& rq);
}

#endif // INCLUDED_JSINTERFACE_L10N
