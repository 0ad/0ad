#include "lib/self_test.h"

#include "lib/res/graphics/tex.h"
#include "lib/res/graphics/tex_codec.h"
#include "lib/res/graphics/tex_internal.h"	// tex_encode

class TestTex : public CxxTest::TestSuite 
{
	void generate_encode_decode_compare(uint w, uint h, uint flags, uint bpp,
		const char* filename)
	{
		// generate test data
		const size_t size = w*h*bpp/8;
		u8* img = new u8[size];
		for(size_t i = 0; i < size; i++)
			img[i] = rand() & 0xFF;

		// wrap in Tex
		Tex t;
		TS_ASSERT_OK(tex_wrap(w, h, bpp, flags, img, &t));

		// encode to file format
		DynArray da;
		TS_ASSERT_OK(tex_encode(&t, filename, &da));
		memset(&t, 0, sizeof(t));

		// decode from file format
		MEM_DTOR dtor = 0;	// we'll free da manually
		TS_ASSERT_OK(tex_decode(da.base, da.cur_size, dtor, &t));

		// make sure pixel format gets converted completely to plain
		TS_ASSERT_OK(tex_transform_to(&t, 0));

		// compare img
		TS_ASSERT_SAME_DATA(tex_get_data(&t), img, size);

		// cleanup
		TS_ASSERT_OK(tex_free(&t));
		TS_ASSERT_OK(da_free(&da));
		delete[] img;
	}

public:

	// this also covers BGR and orientation transforms.
	void test_encode_decode()
	{
		// for each codec
		const TexCodecVTbl* c = 0;
		for(;;)
		{
			c = tex_codec_next(c);
			if(!c)
				break;

			// get an extension that this codec will support
			// (so that tex_encode uses this codec)
			char extension[30] = {'.'};
			strcpy_s(extension+1, 29, c->name);
			// .. make sure the c->name hack worked
			const TexCodecVTbl* correct_c;
			TS_ASSERT_OK(tex_codec_for_filename(extension, &correct_c));
			TS_ASSERT_EQUALS(c, correct_c);

			// for each test width/height combination
			const uint widths [] = { 4, 5, 4, 256, 384 };
			const uint heights[] = { 4, 4, 5, 256, 256 };
			for(size_t i = 0; i < ARRAY_SIZE(widths); i++)
			{
				// for each bit depth
				for(uint bpp = 8; bpp <= 32; bpp += 8)
				{
					uint flags = 0;
					if(!strcmp(extension, ".dds"))
						flags |= (TEX_DXT&3);	// DXT3
					if(bpp == 8)
						flags |= TEX_GREY;
					else if(bpp == 32)
						flags |= TEX_ALPHA;

					// normal
					generate_encode_decode_compare(widths[i], heights[i], flags, bpp, extension);
					// top-down
					flags &= ~TEX_ORIENTATION; flags |= TEX_TOP_DOWN;
					generate_encode_decode_compare(widths[i], heights[i], flags, bpp, extension);
					// bottom up
					flags &= ~TEX_ORIENTATION; flags |= TEX_BOTTOM_UP;
					generate_encode_decode_compare(widths[i], heights[i], flags, bpp, extension);

					flags &= ~TEX_ORIENTATION;
					flags |= TEX_BGR;

					// bgr, normal
					generate_encode_decode_compare(widths[i], heights[i], flags, bpp, extension);
					// bgr, top-down
					flags &= ~TEX_ORIENTATION; flags |= TEX_TOP_DOWN;
					generate_encode_decode_compare(widths[i], heights[i], flags, bpp, extension);
					// bgr, bottom up
					flags &= ~TEX_ORIENTATION; flags |= TEX_BOTTOM_UP;
					generate_encode_decode_compare(widths[i], heights[i], flags, bpp, extension);
				}	// for bpp
			}	// for width/height
		}	 // foreach codec
	}

	// have mipmaps be created for a test image; check resulting size and pixels
	void test_mipmap_create()
	{
		u8 img[] = { 0x10,0x20,0x30, 0x40,0x60,0x80, 0xA0,0xA4,0xA8, 0xC0,0xC1,0xC2 };
		// assumes 2x2 box filter algorithm with rounding
		const u8 mipmap[] = { 0x6C,0x79,0x87 };
		Tex t;
		TS_ASSERT_OK(tex_wrap(2, 2, 24, 0, img, &t));
		TS_ASSERT_OK(tex_transform_to(&t, TEX_MIPMAPS));
		const u8* const out_img = tex_get_data(&t);
		TS_ASSERT_EQUALS((int)tex_img_size(&t), 12+3);
		TS_ASSERT_SAME_DATA(out_img, img, 12);
		TS_ASSERT_SAME_DATA(out_img+12, mipmap, 3);
	}

	void test_img_size()
	{
		char dummy_img[100*100*4];	// required

		Tex t;
		TS_ASSERT_OK(tex_wrap(100, 100, 32, TEX_ALPHA, dummy_img, &t));
		TS_ASSERT_EQUALS((int)tex_img_size(&t), 40000);

		// DXT rounds up to 4x4 blocks; DXT1a is 4bpp
		Tex t2;
		TS_ASSERT_OK(tex_wrap(97, 97, 4, DXT1A, dummy_img, &t2));
		TS_ASSERT_EQUALS((int)tex_img_size(&t2),  5000);
	}
};
