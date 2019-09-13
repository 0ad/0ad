/* Copyright (C) 2019 Wildfire Games.
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

#include "precompiled.h"

#include "lib/config2.h"
#if CONFIG2_LOBBY

#include "lobby/XmppClient.h"
#include "scriptinterface/ScriptInterface.h"

template<> void ScriptInterface::ToJSVal<glooxwrapper::string>(JSContext* cx, JS::MutableHandleValue ret, const glooxwrapper::string& val)
{
	ToJSVal(cx, ret, wstring_from_utf8(val.to_string()));
}

template<> void ScriptInterface::ToJSVal<gloox::Presence::PresenceType>(JSContext* cx, JS::MutableHandleValue ret, const gloox::Presence::PresenceType& val)
{
	ToJSVal(cx, ret, XmppClient::GetPresenceString(val));
}

template<> void ScriptInterface::ToJSVal<gloox::MUCRoomRole>(JSContext* cx, JS::MutableHandleValue ret, const gloox::MUCRoomRole& val)
{
	ToJSVal(cx, ret, XmppClient::GetRoleString(val));
}

template<> void ScriptInterface::ToJSVal<gloox::StanzaError>(JSContext* cx, JS::MutableHandleValue ret, const gloox::StanzaError& val)
{
	ToJSVal(cx, ret, wstring_from_utf8(XmppClient::StanzaErrorToString(val)));
}

template<> void ScriptInterface::ToJSVal<gloox::ConnectionError>(JSContext* cx, JS::MutableHandleValue ret, const gloox::ConnectionError& val)
{
	ToJSVal(cx, ret, wstring_from_utf8(XmppClient::ConnectionErrorToString(val)));
}

template<> void ScriptInterface::ToJSVal<gloox::RegistrationResult>(JSContext* cx, JS::MutableHandleValue ret, const gloox::RegistrationResult& val)
{
	ToJSVal(cx, ret, wstring_from_utf8(XmppClient::RegistrationResultToString(val)));
}

template<> void ScriptInterface::ToJSVal<gloox::CertStatus>(JSContext* cx, JS::MutableHandleValue ret, const gloox::CertStatus& val)
{
	ToJSVal(cx, ret, wstring_from_utf8(XmppClient::CertificateErrorToString(val)));
}

#endif // CONFIG2_LOBBY
