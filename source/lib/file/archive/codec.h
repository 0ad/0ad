/* Copyright (C) 2010 Wildfire Games.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/*
 * this layer allows for other compression methods/libraries
 * besides ZLib. it also simplifies the interface for user code and
 * does error checking, etc.
 */

#ifndef INCLUDED_CODEC
#define INCLUDED_CODEC

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
	virtual Status Reset() = 0;

	/**
	 * process (i.e. compress or decompress) data.
	 *
	 * @param in
	 * @param inSize
	 * @param out
	 * @param outSize Bytes remaining in the output buffer; shall not be zero.
	 * @param inConsumed,outProduced How many bytes in the input and
	 *		  output buffers were used. either or both of these can be zero if
	 *		  the input size is small or there's not enough output space.
	 **/
	virtual Status Process(const u8* in, size_t inSize, u8* out, size_t outSize, size_t& inConsumed, size_t& outProduced) = 0;

	/**
	 * Flush buffers and make sure all output has been produced.
	 *
	 * @param checksum Checksum over all input data.
	 * @param outProduced
	 * @return error status for the entire operation.
	 **/
	virtual Status Finish(u32& checksum, size_t& outProduced) = 0;

	/**
	 * update a checksum to reflect the contents of a buffer.
	 *
	 * @param checksum the initial value (must be 0 on first call)
	 * @param in
	 * @param inSize
	 * @return the new checksum. note: after all data has been seen, this is
	 * identical to the what Finish would return.
	 **/
	virtual u32 UpdateChecksum(u32 checksum, const u8* in, size_t inSize) const = 0;
};

typedef shared_ptr<ICodec> PICodec;

#endif	// #ifndef INCLUDED_CODEC
