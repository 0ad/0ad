/* Copyright (C) 2009 Wildfire Games.
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

/**
 * =========================================================================
 * File        : codec.h
 * Project     : 0 A.D.
 * Description : 
 * =========================================================================
 */

// license: GPL; see lib/license.txt

#ifndef INCLUDED_CODEC
#define INCLUDED_CODEC

// rationale: this layer allows for other compression methods/libraries
// besides ZLib. it also simplifies the interface for user code and
// does error checking, etc.

#define CODEC_COMPUTE_CHECKSUM 1

struct ICodec
{
public:
	/**
	 * note: the implementation should not check whether any data remains -
	 * codecs are sometimes destroyed without completing a transfer.
	 **/
	virtual ~ICodec();

	/**
	 * @return an upper bound on the output size for the given amount of input.
	 * this is used when allocating a single buffer for the whole operation.
	 **/
	virtual size_t MaxOutputSize(size_t inSize) const = 0;

	/**
	 * clear all previous state and prepare for reuse.
	 *
	 * this is as if the object were destroyed and re-created, but more
	 * efficient since it avoids reallocating a considerable amount of
	 * memory (about 200KB for LZ).
	 **/
	virtual LibError Reset() = 0;

	/**
	 * process (i.e. compress or decompress) data.
	 *
	 * @param outSize bytes remaining in the output buffer; shall not be zero.
	 * @param inConsumed, outProduced how many bytes in the input and
	 * output buffers were used. either or both of these can be zero if
	 * the input size is small or there's not enough output space.
	 **/
	virtual LibError Process(const u8* in, size_t inSize, u8* out, size_t outSize, size_t& inConsumed, size_t& outProduced) = 0;

	/**
	 * flush buffers and make sure all output has been produced.
	 *
	 * @param checksum over all input data.
	 * @return error status for the entire operation.
	 **/
	virtual LibError Finish(u32& checksum, size_t& outProduced) = 0;

	/**
	 * update a checksum to reflect the contents of a buffer.
	 *
	 * @param checksum the initial value (must be 0 on first call)
	 * @return the new checksum. note: after all data has been seen, this is
	 * identical to the what Finish would return.
	 **/
	virtual u32 UpdateChecksum(u32 checksum, const u8* in, size_t inSize) const = 0;
};

typedef shared_ptr<ICodec> PICodec;

#endif	// #ifndef INCLUDED_CODEC
