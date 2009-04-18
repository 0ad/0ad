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

#include "lib/self_test.h"

#include "lib/self_test.h"
#include "lib/res/file/archive/codec_zlib.h"

class TestCodecZLib : public CxxTest::TestSuite 
{
public:
	void test_compress_decompress_compare()
	{
		size_t inConsumed, outProduced;
		u32 checksum;

		// generate random input udata
		// (limit values to 0..7 so that the udata will actually be compressible)
		const size_t usize = 10000;
		u8 udata[usize];
		for(size_t i = 0; i < usize; i++)
			udata[i] = rand() & 0x07;

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
