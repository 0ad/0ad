/* Copyright (C) 2011 Wildfire Games.
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

#ifndef INCLUDED_ISERIALIZER
#define INCLUDED_ISERIALIZER

#include "maths/Fixed.h"
#include "ps/Errors.h"
#include "scriptinterface/ScriptVal.h"

ERROR_GROUP(Serialize);
ERROR_TYPE(Serialize, OutOfBounds);
ERROR_TYPE(Serialize, InvalidCharInString);
ERROR_TYPE(Serialize, ScriptError);
ERROR_TYPE(Serialize, InvalidScriptValue);

/**
 * @page serialization Component serialization
 *
 * This page gives a high-level description of ISerializer and IDeserializer.
 *
 * @section design Rough design notes
 *
 * We want to handle the following serialization-related features:
 * - Out-of-sync detection: Based on hash of simulation state.
 *   Must be fast.
 * - Debugging OOS failures: Human-readable output of simulation state,
 *   that can be diffed against other states.
 * - Saving and loading games to disk: Machine-readable output of state.
 *   Should remain usable after patches to the game.
 * - Joining in-progress network games: Machine-readable output of state.
 *   Must be compact (minimise network traffic).
 *   Must be secure (no remote code execution from invalid network data).
 *
 * All cases must be portable across OSes, CPU architectures (endianness, bitness),
 * compilers, compiler settings, etc.
 *
 * This is designed for serializing components, which we consider to have three main kinds of state:
 * - Basic deterministic state: Stuff that's critical to the simulation state,
 *   and is identical between all players in a network game.
 *   Always needs to be saved and loaded and hashed and dumped etc.
 * - Derived deterministic state: Stuff that can be identically recomputed from
 *   the basic deterministic state,
 *   but typically is stored in memory as an optimisation (e.g. caches).
 *   Shouldn't be saved to disk/network (to save space), and should be recomputed on loading.
 *   Should be included in hashes and debug dumps, to detect errors sooner.
 * - Non-deterministic state: Stuff that's not part of the simulation state,
 *   and may vary between players in a network game,
 *   but is kept for graphics or for optimisations etc.
 *   Shouldn't be saved or hashed or dumped or anything.
 *
 * (TODO: Versioning (for saved games to survive patches) is not yet implemented.)
 *
 * We have separate serialization and deserialization APIs (thus requiring components
 * to implement separate serialize/deserialize functions), rather than a single unified
 * API (that can either serialize or deserialize depending on the particular instance).
 * This means more work for simple components, but it simplifies complex components (those
 * which have versioning, or recompute derived state) since they don't need lots of
 * "if (IsDeserializing()) ..." checks.
 *
 * Callers must use the same sequence of IDeserializer calls as they used
 * ISerializer calls. For efficiency, serializations typically don't encode
 * the data type, and so mismatches will make them read bogus values and
 * hopefully hit an out-of-bounds exception.
 *
 * The ISerializable interface used by some code in the engine is irrelevant here. Serialization
 * of interesting classes is implemented in this module, not in the classes themselves, so that
 * the serialization system has greater control (e.g. the human-readable debug output can handle
 * classes very differently to binary output).
 *
 * To encourage portability, only fixed-size types are exposed in the API (e.g. int32_t,
 * not int). Components should use fixed-size types internally, to ensure deterministic
 * simulation across 32-bit and 64-bit architectures.
 *
 * TODO: At least some compound types like lists ought to be supported somehow.
 *
 * To encourage security, the API accepts bounds on numbers and string lengths etc,
 * and will fail if the data exceeds the bounds. Callers should provide sensible bounds
 * (they must never be exceeded, but the caller should be able to safely cope with any values
 * within the bounds and not crash or run out of memory etc).
 *
 * For the OOS debugging output, the API requires field names for all values. These are
 * only used for human-readable output, so they should be readable but don't have to
 * be unique or follow any particular pattern.
 *
 * The APIs throw exceptions whenever something fails (all errors are fatal).
 * Component (de)serialization functions must be exception-safe.
 */

/*
 * Implementation details:
 *
 * The current implementation has lots of nested virtual function calls,
 * to maximise flexibility. If this turns out to be too inefficient,
 * it'll need to be rewritten somehow with less virtuals (probably using
 * templates instead). (This is a more generic design than will be needed
 * in practice.)
 *
 * The public API functions do bounds checking, then pass the data
 * to the Put* methods, which subclasses must handle.
 */

/**
 * Serialization interface; see @ref serialization "serialization overview".
 */
class ISerializer
{
public:
	virtual ~ISerializer();

	/**
	 * Serialize a number, which must be lower <= value <= upper.
	 * The same bounds must be used when deserializing the number.
	 * This should be used in preference to the Unbounded functions, to
	 * detect erroneous or malicious values that might cause problems when used.
	 * @throw PSERROR_Serialize_OutOfBounds if value is out of bounds
	 * @throw PSERROR_Serialize for any other errors
	 * @param name informative name for debug output
	 * @param value value to serialize
	 * @param lower inclusive lower bound
	 * @param upper inclusive upper bound
	 */
	void NumberU8(const char* name, uint8_t value, uint8_t lower, uint8_t upper);
	void NumberI8(const char* name, int8_t value, int8_t lower, int8_t upper);
	void NumberU16(const char* name, uint16_t value, uint16_t lower, uint16_t upper); ///< @copydoc NumberU8
	void NumberI16(const char* name, int16_t value, int16_t lower, int16_t upper); ///< @copydoc NumberU8
	void NumberU32(const char* name, uint32_t value, uint32_t lower, uint32_t upper); ///< @copydoc NumberU8
	void NumberI32(const char* name, int32_t value, int32_t lower, int32_t upper); ///< @copydoc NumberU8

	/**
	 * Serialize a number.
	 * @throw PSERROR_Serialize for any errors
	 * @param name informative name for debug output
	 * @param value value to serialize
	 */
	void NumberU8_Unbounded(const char* name, uint8_t value)
	{
		// (These functions are defined inline for efficiency)
		PutNumber(name, value);
	}

	void NumberI8_Unbounded(const char* name, int8_t value) ///@copydoc NumberU8_Unbounded()
	{
		PutNumber(name, value);
	}

	void NumberU16_Unbounded(const char* name, uint16_t value) ///@copydoc NumberU8_Unbounded()
	{
		PutNumber(name, value);
	}

	void NumberI16_Unbounded(const char* name, int16_t value) ///@copydoc NumberU8_Unbounded()
	{
		PutNumber(name, value);
	}

	void NumberU32_Unbounded(const char* name, uint32_t value) ///@copydoc NumberU8_Unbounded()
	{
		PutNumber(name, value);
	}

	void NumberI32_Unbounded(const char* name, int32_t value) ///@copydoc NumberU8_Unbounded()
	{
		PutNumber(name, value);
	}

	void NumberFloat_Unbounded(const char* name, float value) ///@copydoc NumberU8_Unbounded()
	{
		PutNumber(name, value);
	}

	void NumberDouble_Unbounded(const char* name, double value) ///@copydoc NumberU8_Unbounded()
	{
		PutNumber(name, value);
	}

	void NumberFixed_Unbounded(const char* name, fixed value) ///@copydoc NumberU8_Unbounded()
	{
		PutNumber(name, value);
	}

	/**
	 * Serialize a boolean.
	 */
	void Bool(const char* name, bool value)
	{
		PutBool(name, value);
	}

	/**
	 * Serialize an ASCII string.
	 * Characters must be in the range U+0001 .. U+00FF (inclusive).
	 * The string must be between minlength .. maxlength (inclusive) characters.
	 */
	void StringASCII(const char* name, const std::string& value, uint32_t minlength, uint32_t maxlength);

	/**
	 * Serialize a Unicode string.
	 * Characters must be in the range U+0000..U+D7FF, U+E000..U+FDCF, U+FDF0..U+FFFD (inclusive).
	 * The string must be between minlength .. maxlength (inclusive) characters.
	 */
	void String(const char* name, const std::wstring& value, uint32_t minlength, uint32_t maxlength);

	/**
	 * Serialize a JS::MutableHandleValue.
	 * The value must not contain any unserializable values (like functions).
	 * NOTE: We have to use a mutable handle because JS_Stringify requires that for unknown reasons.
	 */
	void ScriptVal(const char* name, JS::MutableHandleValue value);

	/**
	 * Serialize a stream of bytes.
	 * It is the caller's responsibility to deal with portability (padding, endianness, etc);
	 * the typed serialize methods should usually be used instead of this.
	 */
	void RawBytes(const char* name, const u8* data, size_t len);

	/**
	 * Returns true if the serializer is being used in debug mode.
	 * Components should serialize non-critical data (e.g. data that is unchanged
	 * from the template) only if this is true. (It must still only be used
	 * for synchronised, deterministic data.)
	 */
	virtual bool IsDebug() const;

	/**
	 * Returns a stream which can be used to serialize data directly.
	 * (This is particularly useful for chaining multiple serializers
	 * together.)
	 */
	virtual std::ostream& GetStream() = 0;

protected:
	virtual void PutNumber(const char* name, uint8_t value) = 0;
	virtual void PutNumber(const char* name, int8_t value) = 0;
	virtual void PutNumber(const char* name, uint16_t value) = 0;
	virtual void PutNumber(const char* name, int16_t value) = 0;
	virtual void PutNumber(const char* name, uint32_t value) = 0;
	virtual void PutNumber(const char* name, int32_t value) = 0;
	virtual void PutNumber(const char* name, float value) = 0;
	virtual void PutNumber(const char* name, double value) = 0;
	virtual void PutNumber(const char* name, fixed value) = 0;
	virtual void PutBool(const char* name, bool value) = 0;
	virtual void PutString(const char* name, const std::string& value) = 0;
	// We have to use a mutable handle because JS_Stringify requires that for unknown reasons.
	virtual void PutScriptVal(const char* name, JS::MutableHandleValue value) = 0;
	virtual void PutRaw(const char* name, const u8* data, size_t len) = 0;
};

#endif // INCLUDED_ISERIALIZER
