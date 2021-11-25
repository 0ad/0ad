/* Copyright (C) 2021 Wildfire Games.
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

#include "lib/self_test.h"

#include "lib/self_test.h"
#include "lib/res/file/archive/codec_zlib.h"

#include <random>

class TestCodecZLib : public CxxTest::TestSuite
{
public:
	void test_compress_decompress_compare()
	{
		size_t inConsumed, outProduced;
		u32 checksum;

		// generate random input udata
		// (limit values to 0..7 so that the udata will actually be compressible)
		std::mt19937 engine(42);
		std::uniform_int_distribution<u8> distribution(0x00, 0x07);
		const size_t usize = 10000;
		u8 udata[usize];
		for(size_t i = 0; i < usize; i++)
			udata[i] = distribution(engine);

		// compress
		u8* cdata; size_t csize;
		{
			boost::shared_ptr<ICodec> compressor_zlib = CreateCompressor_ZLib();
			ICodec* c = compressor_zlib.get();
			const size_t csizeMax = c->MaxOutputSize(usize);
			cdata = new u8[csizeMax];
			TS_ASSERT_OK(c->Process(udata, usize, cdata, csizeMax, inConsumed, outProduced));
			TS_ASSERT_EQUALS(inConsumed, usize);
			TS_ASSERT_LESS_THAN_EQUALS(outProduced, csizeMax);
			u8* cdata2;
			TS_ASSERT_OK(c->Finish(cdata2, csize, checksum));
			TS_ASSERT_EQUALS(cdata, cdata2);
			TS_ASSERT_EQUALS(csize, outProduced);
		}

		// make sure the data changed during compression
		TS_ASSERT(csize != usize || memcmp(udata, cdata, std::min(usize, csize)) != 0);

		// decompress
		u8 ddata[usize];
		{
			boost::shared_ptr<ICodec> decompressor_zlib = CreateDecompressor_ZLib();
			ICodec* d = decompressor_zlib.get();
			TS_ASSERT_OK(decompressor_zlib->Process(cdata, csize, ddata, usize, inConsumed, outProduced));
			TS_ASSERT_EQUALS(inConsumed, csize);	// ZLib always outputs as much data as possible
			TS_ASSERT_EQUALS(outProduced, usize);	// .. so these figures are correct before Finish()
			u8* ddata2; size_t dsize;
			TS_ASSERT_OK(d->Finish(&ddata2, &dsize, &checksum));
			TS_ASSERT_EQUALS(ddata, ddata2);
			TS_ASSERT_EQUALS(dsize, outProduced);
		}

		// verify udata survived intact
		TS_ASSERT_SAME_DATA(udata, ddata, usize);

		delete[] cdata;
	}
};
