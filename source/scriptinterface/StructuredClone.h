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

#ifndef INCLUDED_SCRIPTINTERFACE_STRUCTUREDCLONE
#define INCLUDED_SCRIPTINTERFACE_STRUCTUREDCLONE

#include "ScriptForward.h"

#include <memory>

class JSStructuredCloneData;

namespace Script
{
/**
 * Structured clones are a way to serialize 'simple' JS::Values into a buffer
 * that can safely be passed between compartments and between threads.
 * A StructuredClone can be stored and read multiple times if desired.
 * We wrap them in shared_ptr so memory management is automatic and
 * thread-safe.
 */
using StructuredClone = std::shared_ptr<JSStructuredCloneData>;

StructuredClone WriteStructuredClone(const ScriptRequest& rq, JS::HandleValue v);
void ReadStructuredClone(const ScriptRequest& rq, const StructuredClone& ptr, JS::MutableHandleValue ret);

/**
 * Construct a new value by cloning a value (possibly from a different Compartment).
 * Complex values (functions, XML, etc) won't be cloned correctly, but basic
 * types and cyclic references should be fine.
 * Takes ScriptInterfaces to enter the correct realm.
 * Caller beware - manipulating several compartments in the same function is tricky.
 * @param to - ScriptInterface of the target. Should match the rooting context of the result.
 * @param from - ScriptInterface of @a val.
 */
JS::Value CloneValueFromOtherCompartment(const ScriptInterface& to, const ScriptInterface& from, JS::HandleValue val);

/**
 * Clone a JS value, ensuring that changes to the result
 * won't affect the original value.
 * Works by cloning, so the same restrictions as CloneValueFromOtherCompartment apply.
 */
JS::Value DeepCopy(const ScriptRequest& rq, JS::HandleValue val);

} // namespace Script

#endif // INCLUDED_SCRIPTINTERFACE_STRUCTUREDCLONE
