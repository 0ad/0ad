// This code is in the public domain -- castanyo@yahoo.es

#include "ImageIO.h"
#include "Image.h"
#include "FloatImage.h"
#include "TgaFile.h"
#include "PsdFile.h"
#include "DirectDrawSurface.h"
#include "PixelFormat.h"

#include "nvmath/Color.h"
#include "nvmath/Half.h"

#include "nvcore/Ptr.h"
#include "nvcore/Utils.h"
#include "nvcore/Array.inl"
#include "nvcore/StrLib.h"
#include "nvcore/StdStream.h"
#include "nvcore/TextWriter.h"

// Extern
#if defined(HAVE_FREEIMAGE)
#   include <FreeImage.h>
// If FreeImage available, do not use individual libraries, since that produces link conflicts in some platforms.
#   undef HAVE_JPEG
#   undef HAVE_PNG
#   undef HAVE_TIFF
#   undef HAVE_OPENEXR
#endif

#if defined(HAVE_JPEG)
extern "C" {
#   include <jpeglib.h>
}
#endif

#if defined(HAVE_PNG)
#   include <png.h>
#endif

#if defined(HAVE_TIFF)
#   define _TIFF_DATA_TYPEDEFS_
#   include <tiffio.h>
#endif

#if defined(HAVE_OPENEXR)
#   include <ImfIO.h>
#   include <ImathBox.h>
#   include <ImfChannelList.h>
#   include <ImfInputFile.h>
#   include <ImfOutputFile.h>
#   include <ImfArray.h>
#endif

#if defined(HAVE_STBIMAGE)
#   define STBI_NO_STDIO
#   include <stb_image.h>
#endif


using namespace nv;



struct Color555 {
    uint16 b : 5;
    uint16 g : 5;
    uint16 r : 5;
};

// Load TGA image.
static Image * loadTGA(Stream & s)
{
    nvCheck(!s.isError());
    nvCheck(s.isLoading());

    TgaHeader tga;
    s << tga;
    s.seek(TgaHeader::Size + tga.id_length);

    // Get header info.
    bool rle = false;
    bool pal = false;
    bool rgb = false;
    bool grey = false;

    switch( tga.image_type ) {
        case TGA_TYPE_RLE_INDEXED:
            rle = true;
            // no break is intended!
        case TGA_TYPE_INDEXED:
            if( tga.colormap_type!=1 || tga.colormap_size!=24 || tga.colormap_length>256 ) {
                nvDebug( "*** loadTGA: Error, only 24bit paletted images are supported.\n" );
                return NULL;
            }
            pal = true;
            break;

	case TGA_TYPE_RLE_RGB:
	    rle = true;
	    // no break is intended!
	case TGA_TYPE_RGB:
	    rgb = true;
	    break;

	case TGA_TYPE_RLE_GREY:
	    rle = true;
	    // no break is intended!
	case TGA_TYPE_GREY:
	    grey = true;
	    break;

	default:
	    nvDebug( "*** loadTGA: Error, unsupported image type.\n" );
	    return NULL;
    }

    const uint pixel_size = (tga.pixel_size/8);
    nvDebugCheck(pixel_size <= 4);

    const uint size = tga.width * tga.height * pixel_size;


    // Read palette
    uint8 palette[768];
    if( pal ) {
        nvDebugCheck(tga.colormap_length <= 256);
        s.serialize(palette, 3 * tga.colormap_length);
    }

    // Decode image.
    uint8 * mem = new uint8[size];
    if( rle ) {
        // Decompress image in src.
        uint8 * dst = mem;
        int num = size;

	while (num > 0) {
	    // Get packet header
	    uint8 c;
	    s << c;

	    uint count = (c & 0x7f) + 1;
	    num -= count * pixel_size;

	    if (c & 0x80) {
		// RLE pixels.
		uint8 pixel[4];	// uint8 pixel[pixel_size];
		s.serialize( pixel, pixel_size );
		do {
		    memcpy(dst, pixel, pixel_size);
		    dst += pixel_size;
		} while (--count);
	    }
	    else {
		// Raw pixels.
		count *= pixel_size;
		//file->Read8(dst, count);
		s.serialize(dst, count);
		dst += count;
	    }
	}
    }
    else {
        s.serialize(mem, size);
    }

    // Allocate image.
    AutoPtr<Image> img(new Image());
    img->allocate(tga.width, tga.height);

    int lstep;
    Color32 * dst;
    if( tga.flags & TGA_ORIGIN_UPPER ) {
        lstep = tga.width;
        dst = img->pixels();
    }
    else {
        lstep = - tga.width;
        dst = img->pixels() + (tga.height-1) * tga.width;
    }

    // Write image.
    uint8 * src = mem;
    if( pal ) {
        for( int y = 0; y < tga.height; y++ ) {
            for( int x = 0; x < tga.width; x++ ) {
                uint8 idx = *src++;
                dst[x].setBGRA(palette[3*idx+0], palette[3*idx+1], palette[3*idx+2], 0xFF);
            }
            dst += lstep;
        }
    }
    else if( grey ) {
        img->setFormat(Image::Format_ARGB);

        for( int y = 0; y < tga.height; y++ ) {
            for( int x = 0; x < tga.width; x++ ) {
                dst[x].setBGRA(*src, *src, *src, *src);
                src++;
            }
            dst += lstep;
        }
    }
    else {

        if( tga.pixel_size == 16 ) {
            for( int y = 0; y < tga.height; y++ ) {
                for( int x = 0; x < tga.width; x++ ) {
                    Color555 c = *reinterpret_cast<Color555 *>(src);
                    uint8 b = (c.b << 3) | (c.b >> 2);
                    uint8 g = (c.g << 3) | (c.g >> 2);
                    uint8 r = (c.r << 3) | (c.r >> 2);
                    dst[x].setBGRA(b, g, r, 0xFF);
                    src += 2;
                }
                dst += lstep;
            }
        }
        else if( tga.pixel_size == 24 ) {
            for( int y = 0; y < tga.height; y++ ) {
                for( int x = 0; x < tga.width; x++ ) {
                    dst[x].setBGRA(src[0], src[1], src[2], 0xFF);
                    src += 3;
                }
                dst += lstep;
            }
        }
        else if( tga.pixel_size == 32 ) {
            img->setFormat(Image::Format_ARGB);

            for( int y = 0; y < tga.height; y++ ) {
                for( int x = 0; x < tga.width; x++ ) {
                    dst[x].setBGRA(src[0], src[1], src[2], src[3]);
                    src += 4;
                }
                dst += lstep;
            }
        }
    }

    // free uncompressed data.
    delete [] mem;

    return img.release();
}

// Save TGA image.
static bool saveTGA(Stream & s, const Image * img)
{
    nvCheck(!s.isError());
    nvCheck(img != NULL);
    nvCheck(img->pixels() != NULL);

    TgaFile tga;
    tga.head.id_length = 0;
    tga.head.colormap_type = 0;
    tga.head.image_type = TGA_TYPE_RGB;

    tga.head.colormap_index = 0;
    tga.head.colormap_length = 0;
    tga.head.colormap_size = 0;

    tga.head.x_origin = 0;
    tga.head.y_origin = 0;
    tga.head.width = img->width();
    tga.head.height = img->height();
    if(img->format() == Image::Format_ARGB) {
        tga.head.pixel_size = 32;
        tga.head.flags = TGA_ORIGIN_UPPER | TGA_HAS_ALPHA;
    }
    else {
        tga.head.pixel_size = 24;
        tga.head.flags = TGA_ORIGIN_UPPER;
    }

    // @@ Serialize directly.
    tga.allocate();

    const uint n = img->width() * img->height();
    if(img->format() == Image::Format_ARGB) {
        for(uint i = 0; i < n; i++) {
            Color32 color = img->pixel(i);
            tga.mem[4 * i + 0] = color.b;
            tga.mem[4 * i + 1] = color.g;
            tga.mem[4 * i + 2] = color.r;
            tga.mem[4 * i + 3] = color.a;
        }
    }
    else {
        for(uint i = 0; i < n; i++) {
            Color32 color = img->pixel(i);
            tga.mem[3 * i + 0] = color.b;
            tga.mem[3 * i + 1] = color.g;
            tga.mem[3 * i + 2] = color.r;
        }
    }

    s << tga;

    tga.free();

    return true;
}

/*static Image * loadPPM(Stream & s)
{
    // @@
    return NULL;
}*/

// Save PPM image.
static bool savePPM(Stream & s, const Image * img)
{
    //if (img->depth() != 1) return false;
    //if (img->format() == Image::Format_ARGB) return false;

    uint w = img->width();
    uint h = img->height();

    TextWriter writer(&s);
    writer.format("P6\n");
    writer.format("%d %d\n", w, h);
    writer.writeString("255\n");
    for (uint i = 0; i < w * h; i++) {
        Color32 c = img->pixel(i);
        s << (uint8_t&)c.r << (uint8_t&)c.g << (uint8_t&)c.b;
    }

    return true;
}


/*static FloatImage * loadFloatPFM(Stream & s)
{
    return NULL;
}*/

/*static bool saveFloatPFM(Stream & s, const FloatImage * img, uint base_channel, uint channel_count)
{
    return false;
}*/

// Load PSD image.
static Image * loadPSD(Stream & s)
{
    nvCheck(!s.isError());
    nvCheck(s.isLoading());

    s.setByteOrder(Stream::BigEndian);

    PsdHeader header;
    s << header;

    if (!header.isValid())
    {
        printf("invalid header!\n");
        return NULL;
    }

    if (!header.isSupported())
    {
        printf("unsupported file!\n");
        return NULL;
    }

    int tmp;

    // Skip mode data.
    s << tmp;
    s.seek(s.tell() + tmp);

    // Skip image resources.
    s << tmp;
    s.seek(s.tell() + tmp);

    // Skip the reserved data.
    s << tmp;
    s.seek(s.tell() + tmp);

    // Find out if the data is compressed.
    // Known values:
    //   0: no compression
    //   1: RLE compressed
    uint16 compression;
    s << compression;

    if (compression > 1) {
        // Unknown compression type.
        return NULL;
    }

    uint channel_num = header.channel_count;

    AutoPtr<Image> img(new Image());
    img->allocate(header.width, header.height);

    if (channel_num < 4)
    {
        // Clear the image.
        img->fill(Color32(0, 0, 0, 0xFF));
    }
    else
    {
        // Enable alpha.
        img->setFormat(Image::Format_ARGB);

        // Ignore remaining channels.
        channel_num = 4;
    }


    const uint pixel_count = header.height * header.width;

    static const uint components[4] = {2, 1, 0, 3};

    if (compression)
    {
        s.seek(s.tell() + header.height * header.channel_count * sizeof(uint16));

        // Read RLE data.
        for (uint channel = 0; channel < channel_num; channel++)
        {
            uint8 * ptr = (uint8 *)img->pixels() + components[channel];

            uint count = 0;
            while( count < pixel_count )
            {
                if (s.isAtEnd()) return NULL;

                uint8 c;
                s << c;

                uint len = c;
                if (len < 128)
                {
                    // Copy next len+1 bytes literally.
                    len++;
                    count += len;
                    if (count > pixel_count) return NULL;

                    while (len != 0)
                    {
                        s << *ptr;
                        ptr += 4;
                        len--;
                    }
                }
                else if (len > 128)
                {
                    // Next -len+1 bytes in the dest are replicated from next source byte.
                    // (Interpret len as a negative 8-bit int.)
                    len ^= 0xFF;
                    len += 2;
                    count += len;
                    if (s.isAtEnd() || count > pixel_count) return NULL;

                    uint8 val;
                    s << val;
                    while( len != 0 ) {
                        *ptr = val;
                        ptr += 4;
                        len--;
                    }
                }
                else if( len == 128 ) {
                    // No-op.
                }
            }
        }
    }
    else
    {
        // We're at the raw image data. It's each channel in order (Red, Green, Blue, Alpha, ...)
        // where each channel consists of an 8-bit value for each pixel in the image.

        // Read the data by channel.
        for (uint channel = 0; channel < channel_num; channel++)
        {
            uint8 * ptr = (uint8 *)img->pixels() + components[channel];

            // Read the data.
            uint count = pixel_count;
            while (count != 0)
            {
                s << *ptr;
                ptr += 4;
                count--;
            }
        }
    }

    return img.release();
}

static FloatImage * loadFloatDDS(Stream & s)
{
    nvCheck(s.isLoading());
    nvCheck(!s.isError());

    DDSHeader header;
    s << header;

    // @@ We only support a few formats for now.

    if (header.pf.fourcc == D3DFMT_A16B16G16R16F) {
        const int size = header.width * header.height;
        uint16 * const data = new uint16[size * 4];

        //s.serialize(data, size * 4 * sizeof(uint16));
        for (int i = 0; i < 4* size; i++) {
            s << data[i];
        }

        FloatImage * img = new FloatImage;
        img->allocate(4, header.width, header.height);

        uint32 * r = (uint32 *)img->channel(0);
        uint32 * g = (uint32 *)img->channel(1);
        uint32 * b = (uint32 *)img->channel(2);
        uint32 * a = (uint32 *)img->channel(3);

        uint16 * ptr = data;
        for (int i = 0; i < size; i++) {
            *r++ = half_to_float( *ptr++ );
            *g++ = half_to_float( *ptr++ );
            *b++ = half_to_float( *ptr++ );
            *a++ = half_to_float( *ptr++ );
        }

        delete [] data;

        return img;
    }
    else if (header.pf.fourcc == D3DFMT_R32F) {
        const int size = header.width * header.height;
        float * const data = new float[size];

        for (int i = 0; i < size; i++) {
            s << data[i];
        }

        FloatImage * img = new FloatImage;
        img->allocate(4, header.width, header.height);

        float * r = img->channel(0);

        float * ptr = data;
        for (int i = 0; i < size; i++) {
            *r++ = *ptr++;
        }

        delete [] data;

        img->clear(1, 0.0f);
        img->clear(2, 0.0f);
        img->clear(3, 1.0f);

        return img;
    }
    else if (header.pf.fourcc == D3DFMT_L16 || (header.pf.bitcount == 16 && header.pf.rmask == 0xFFFF && header.pf.gmask == 0 && header.pf.bmask == 0 && header.pf.amask == 0))
    {
        const int size = header.width * header.height;
        uint16 * const data = new uint16[size];

        for (int i = 0; i < size; i++) {
            s << data[i];
        }

        FloatImage * img = new FloatImage;
        img->allocate(4, header.width, header.height);

        float * r = img->channel(0);

        uint16 * ptr = data;
        for (int i = 0; i < size; i++) {
            *r++ = float(*ptr++) / 65535.0f;
        }

        delete [] data;

        img->clear(1, 0.0f);
        img->clear(2, 0.0f);
        img->clear(3, 1.0f);

        return img;
    }
    else if (header.pf.fourcc == D3DFMT_L8 || (header.pf.bitcount == 8 && header.pf.rmask == 0xFF && header.pf.gmask == 0 && header.pf.bmask == 0 && header.pf.amask == 0))
    {
        const int size = header.width * header.height;
        uint8 * const data = new uint8[size];

        s.serialize(data, size);

        FloatImage * img = new FloatImage;
        img->allocate(4, header.width, header.height);

        float * r = img->channel(0);

        uint8 * ptr = data;
        for (int i = 0; i < size; i++) {
            *r++ = float(*ptr++) / 255.0f;
        }

        delete [] data;

        img->clear(1, 0.0f);
        img->clear(2, 0.0f);
        img->clear(3, 1.0f);

        return img;
    }
    return NULL;
}

static bool saveFloatDDS(Stream & s, const FloatImage * img, uint base_component, uint num_components)
{
    nvCheck(s.isSaving());
    nvCheck(!s.isError());

    if (num_components != 4) return false;

    static const uint D3DFMT_A16B16G16R16F = 113;

    DDSHeader header;
    header.setTexture2D();
    header.setWidth(img->width());
    header.setHeight(img->height());
    header.setFormatCode(D3DFMT_A16B16G16R16F);
    // ...

    s << header;

    uint32 * r = (uint32 *)img->channel(base_component + 0);
    uint32 * g = (uint32 *)img->channel(base_component + 1);
    uint32 * b = (uint32 *)img->channel(base_component + 2);
    uint32 * a = (uint32 *)img->channel(base_component + 3);

    const uint size = img->width() * img->height();
    for (uint i = 0; i < size; i++) {
        uint16 R = half_from_float( *r++ );
        uint16 G = half_from_float( *g++ );
        uint16 B = half_from_float( *b++ );
        uint16 A = half_from_float( *a++ );

        s.serialize(&R, sizeof(uint16));
        s.serialize(&G, sizeof(uint16));
        s.serialize(&B, sizeof(uint16));
        s.serialize(&A, sizeof(uint16));
    }

    return true;
}


#if defined(HAVE_PNG)

static void user_read_data(png_structp png_ptr, png_bytep data, png_size_t length)
{
    nvDebugCheck(png_ptr != NULL);

    Stream * s = (Stream *)png_get_io_ptr(png_ptr);
    s->serialize(data, (int)length);

    if (s->isError()) {
        png_error(png_ptr, "Read Error");
    }
}


static Image * loadPNG(Stream & s)
{
    nvCheck(!s.isError());

    // Set up a read buffer and check the library version
    png_structp png_ptr;
    png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (png_ptr == NULL) {
        //	nvDebug( "*** LoadPNG: Error allocating read buffer in file '%s'.\n", name );
        return NULL;
    }

    // Allocate/initialize a memory block for the image information
    png_infop info_ptr = png_create_info_struct(png_ptr);
    if (info_ptr == NULL) {
        png_destroy_read_struct(&png_ptr, NULL, NULL);
        //	nvDebug( "*** LoadPNG: Error allocating image information for '%s'.\n", name );
        return NULL;
    }

    // Set up the error handling
    if (setjmp(png_jmpbuf(png_ptr))) {
        png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
        //	nvDebug( "*** LoadPNG: Error reading png file '%s'.\n", name );
        return NULL;
    }

    // Set up the I/O functions.
    png_set_read_fn(png_ptr, (void*)&s, user_read_data);


    // Retrieve the image header information
    png_uint_32 width, height;
    int bit_depth, color_type, interlace_type;
    png_read_info(png_ptr, info_ptr);
    png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth, &color_type, &interlace_type, NULL, NULL);


    if (color_type == PNG_COLOR_TYPE_PALETTE && bit_depth <= 8) {
        // Convert indexed images to RGB.
        png_set_expand(png_ptr);
    }
    else if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8) {
        // Convert grayscale to RGB.
        png_set_expand(png_ptr);
    }
    else if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS)) {
        // Expand images with transparency to full alpha channels
        // so the data will be available as RGBA quartets.
        png_set_expand(png_ptr);
    }
    else if (bit_depth < 8) {
        // If we have < 8 scale it up to 8.
        //png_set_expand(png_ptr);
        png_set_packing(png_ptr);
    }

    // Reduce bit depth.
    if (bit_depth == 16) {
        png_set_strip_16(png_ptr);
    }

    // Represent gray as RGB
    if (color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_GRAY_ALPHA) {
        png_set_gray_to_rgb(png_ptr);
    }

    // Convert to RGBA filling alpha with 0xFF.
    if (!(color_type & PNG_COLOR_MASK_ALPHA)) {
        png_set_filler(png_ptr, 0xFF, PNG_FILLER_AFTER);
    }

    // @todo Choose gamma according to the platform?
    double screen_gamma = 2.2;
    int intent;
    if (png_get_sRGB(png_ptr, info_ptr, &intent)) {
        png_set_gamma(png_ptr, screen_gamma, 0.45455);
    }
    else {
        double image_gamma;
        if (png_get_gAMA(png_ptr, info_ptr, &image_gamma)) {
            png_set_gamma(png_ptr, screen_gamma, image_gamma);
        }
        else {
            png_set_gamma(png_ptr, screen_gamma, 0.45455);
        }
    }

    // Perform the selected transforms.
    png_read_update_info(png_ptr, info_ptr);

    png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth, &color_type, &interlace_type, NULL, NULL);

    AutoPtr<Image> img(new Image());
    img->allocate(width, height);

    // Set internal format flags.
    if(color_type & PNG_COLOR_MASK_COLOR) {
        //img->flags |= PI_IF_HAS_COLOR;
    }
    if(color_type & PNG_COLOR_MASK_ALPHA) {
        //img->flags |= PI_IF_HAS_ALPHA;
        img->setFormat(Image::Format_ARGB);
    }

    // Read the image
    uint8 * pixels = (uint8 *)img->pixels();
    png_bytep * row_data = new png_bytep[sizeof(png_byte) * height];
    for (uint i = 0; i < height; i++) {
        row_data[i] = &(pixels[width * 4 * i]);
    }

    png_read_image(png_ptr, row_data);
    delete [] row_data;

    // Finish things up
    png_read_end(png_ptr, info_ptr);
    png_destroy_read_struct(&png_ptr, &info_ptr, NULL);

    // RGBA to BGRA.
    uint num = width * height;
    for(uint i = 0; i < num; i++)
    {
        Color32 c = img->pixel(i);
        img->pixel(i) = Color32(c.b, c.g, c.r, c.a);
    }

    // Compute alpha channel if needed.
    /*if( img->flags & PI_IU_BUMPMAP || img->flags & PI_IU_ALPHAMAP ) {
        if( img->flags & PI_IF_HAS_COLOR && !(img->flags & PI_IF_HAS_ALPHA)) {
            img->ComputeAlphaFromColor();
        }
    }*/

    return img.release();
}

static void user_write_data(png_structp png_ptr, png_bytep data, png_size_t length)
{
    nvDebugCheck(png_ptr != NULL);

    Stream * s = (Stream *)png_get_io_ptr(png_ptr);
    s->serialize(data, (int)length);

    if (s->isError()) {
        png_error(png_ptr, "Write Error");
    }
}

static void user_write_flush(png_structp png_ptr) { }

static bool savePNG(Stream & s, const Image * img, const char ** tags/*=NULL*/)
{
    nvCheck(!s.isError());
    nvCheck(img != NULL);
    nvCheck(img->pixels() != NULL);

    // Set up a write buffer and check the library version
    png_structp png_ptr;
    png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (png_ptr == NULL) {
        return false;
    }

    // Allocate/initialize a memory block for the image information
    png_infop info_ptr = png_create_info_struct(png_ptr);
    if (info_ptr == NULL) {
        png_destroy_write_struct(&png_ptr, NULL);
        return false;
    }

    // Set up the error handling
    if (setjmp(png_jmpbuf(png_ptr))) {
        png_destroy_write_struct(&png_ptr, &info_ptr);
        return false;
    }

    // Set up the I/O functions.
    png_set_write_fn(png_ptr, (void*)&s, user_write_data, user_write_flush);

    // Set image header information
    int color_type = PNG_COLOR_TYPE_RGBA;
    switch(img->format())
    {
        case Image::Format_RGB:		color_type = PNG_COLOR_TYPE_RGB; break;
        case Image::Format_ARGB:	color_type = PNG_COLOR_TYPE_RGBA; break;
    }
    png_set_IHDR(png_ptr, info_ptr, img->width(), img->height(),
        8, color_type, PNG_INTERLACE_NONE,
        PNG_COMPRESSION_TYPE_DEFAULT,
        PNG_FILTER_TYPE_DEFAULT);

    // Set image data
    png_bytep * row_data = new png_bytep[sizeof(png_byte) * img->height()];
    for (uint i = 0; i < img->height(); i++) {
        row_data[i] = (png_byte*)img->scanline (i);
        if (img->format() == Image::Format_RGB) row_data[i]--; // This is a bit of a hack, libpng expects images in ARGB format not BGRA, it supports BGR swapping, but not alpha swapping.
    }
    png_set_rows(png_ptr, info_ptr, row_data);

    png_text * text = NULL;
    if (tags != NULL)
    {
        int count = 0;
        while(tags[2 * count] != NULL) count++;

        text = new png_text[count];
        memset(text, 0, count * sizeof(png_text);

        for (int i = 0; i < count; i++) {
            text[i].compression = PNG_TEXT_COMPRESSION_NONE;
            text[i].key = tags[2 * i + 0];
            text[i].text = tags[2 * i + 1];
        }

        png_set_text(png_ptr, info_ptr, text, count);
    }

    png_write_png(png_ptr, info_ptr,
        // component order is BGR(A)
        PNG_TRANSFORM_BGR |
        // Strip alpha byte for RGB images
        (img->format() == Image::Format_RGB ? PNG_TRANSFORM_STRIP_FILLER : 0)
        , NULL);

    // Finish things up
    png_destroy_write_struct(&png_ptr, &info_ptr);

    delete [] row_data;
    delete [] text;

    return true;
}

#endif // defined(HAVE_PNG)

#if defined(HAVE_JPEG)

static void init_source (j_decompress_ptr /*cinfo*/){
}

static boolean fill_input_buffer (j_decompress_ptr cinfo) {
    struct jpeg_source_mgr * src = cinfo->src;
    static JOCTET FakeEOI[] = { 0xFF, JPEG_EOI };

    // Generate warning
    nvDebug("jpeglib: Premature end of file\n");

    // Insert a fake EOI marker
    src->next_input_byte = FakeEOI;
    src->bytes_in_buffer = 2;

    return TRUE;
}

static void skip_input_data (j_decompress_ptr cinfo, long num_bytes) {
    struct jpeg_source_mgr * src = cinfo->src;

    if(num_bytes >= (long)src->bytes_in_buffer) {
        fill_input_buffer(cinfo);
        return;
    }

    src->bytes_in_buffer -= num_bytes;
    src->next_input_byte += num_bytes;
}

static void term_source (j_decompress_ptr /*cinfo*/){
    // no work necessary here
}


static Image * loadJPG(Stream & s)
{
    nvCheck(!s.isError());

    // Read the entire file.
    Array<uint8> byte_array;
    byte_array.resize(s.size());
    s.serialize(byte_array.buffer(), s.size());

    jpeg_decompress_struct cinfo;
    jpeg_error_mgr jerr;

    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_decompress(&cinfo);

    cinfo.src = (struct jpeg_source_mgr *) (*cinfo.mem->alloc_small)
                ((j_common_ptr) &cinfo, JPOOL_PERMANENT, sizeof(struct jpeg_source_mgr));
    cinfo.src->init_source = init_source;
    cinfo.src->fill_input_buffer = fill_input_buffer;
    cinfo.src->skip_input_data = skip_input_data;
    cinfo.src->resync_to_restart = jpeg_resync_to_restart;	// use default method
    cinfo.src->term_source = term_source;
    cinfo.src->bytes_in_buffer = byte_array.size();
    cinfo.src->next_input_byte = byte_array.buffer();

    jpeg_read_header(&cinfo, TRUE);
    jpeg_start_decompress(&cinfo);

    /*
    cinfo.do_fancy_upsampling = FALSE;	// fast decompression
    cinfo.dct_method = JDCT_FLOAT;			// Choose floating point DCT method.
    */

    uint8 * tmp_buffer = new uint8 [cinfo.output_width * cinfo.output_height * cinfo.num_components];
    uint8 * scanline = tmp_buffer;

    while( cinfo.output_scanline < cinfo.output_height ){
        int num_scanlines = jpeg_read_scanlines (&cinfo, &scanline, 1);
        scanline += num_scanlines * cinfo.output_width * cinfo.num_components;
    }

    jpeg_finish_decompress(&cinfo);

    AutoPtr<Image> img(new Image());
    img->allocate(cinfo.output_width, cinfo.output_height);

    Color32 * dst = img->pixels();
    const int size = img->height() * img->width();
    const uint8 * src = tmp_buffer;

    if( cinfo.num_components == 3 ) {
        img->setFormat(Image::Format_RGB);
        for( int i = 0; i < size; i++ ) {
            *dst++ = Color32(src[0], src[1], src[2]);
            src += 3;
        }
    }
    else {
        img->setFormat(Image::Format_ARGB);
        for( int i = 0; i < size; i++ ) {
            *dst++ = Color32(*src, *src, *src, *src);
            src++;
        }
    }

    delete [] tmp_buffer;
    jpeg_destroy_decompress (&cinfo);

    return img.release();
}

#endif // defined(HAVE_JPEG)

#if defined(HAVE_TIFF)

/*
static tsize_t tiffReadWriteProc(thandle_t h, tdata_t ptr, tsize_t size)
{
    Stream * s = (Stream *)h;
    nvDebugCheck(s != NULL);

    s->serialize(ptr, size);

    return size;
}

static toff_t tiffSeekProc(thandle_t h, toff_t offset, int whence)
{
    Stream * s = (Stream *)h;
    nvDebugCheck(s != NULL);

    if (!s->isSeekable())
    {
        return (toff_t)-1;
    }

    if (whence == SEEK_SET)
    {
        s->seek(offset);
    }
    else if (whence == SEEK_CUR)
    {
        s->seek(s->tell() + offset);
    }
    else if (whence == SEEK_END)
    {
        s->seek(s->size() + offset);
    }

    return s->tell();
}

static int tiffCloseProc(thandle_t)
{
    return 0;
}

static toff_t tiffSizeProc(thandle_t h)
{
    Stream * s = (Stream *)h;
    nvDebugCheck(s != NULL);
    return s->size();
}

static int tiffMapFileProc(thandle_t, tdata_t*, toff_t*)
{
    // @@ TODO, Implement these functions.
    return -1;
}

static void tiffUnmapFileProc(thandle_t, tdata_t, toff_t)
{
    // @@ TODO, Implement these functions.
}
*/

static FloatImage * loadFloatTIFF(const char * fileName, Stream & s)
{
    nvCheck(!s.isError());

    TIFF * tif = TIFFOpen(fileName, "r");
    //TIFF * tif = TIFFClientOpen(fileName, "r", &s, tiffReadWriteProc, tiffReadWriteProc, tiffSeekProc, tiffCloseProc, tiffSizeProc, tiffMapFileProc, tiffUnmapFileProc);

    if (!tif)
    {
        nvDebug("Can't open '%s' for reading\n", fileName);
        return NULL;
    }

    ::uint16 spp, bpp, format;
    ::uint32 width, height;
    TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &height);
    TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &width);
    TIFFGetField(tif, TIFFTAG_BITSPERSAMPLE, &bpp);
    TIFFGetField(tif, TIFFTAG_SAMPLESPERPIXEL, &spp);
    TIFFGetField(tif, TIFFTAG_SAMPLEFORMAT, &format);

    if (bpp != 8 && bpp != 16 && bpp != 32) {
        nvDebug("Can't load '%s', only 1 sample per pixel supported\n", fileName);
        TIFFClose(tif);
        return NULL;
    }

    AutoPtr<FloatImage> fimage(new FloatImage());
    fimage->allocate(spp, width, height);

    int linesize = TIFFScanlineSize(tif);
    tdata_t buf = malloc<uint8>(linesize);

    for (uint y = 0; y < height; y++)
    {
        TIFFReadScanline(tif, buf, y, 0);

	for (uint c=0; c<spp; c++ )
	{
	    float * dst = fimage->scanline(y, c);

	    for(uint x = 0; x < width; x++)
	    {
		if (bpp == 8)
		{
			dst[x] = float(((::uint8 *)buf)[x*spp+c]) / float(0xFF);
		}
		else if (bpp == 16)
		{
			dst[x] = float(((::uint16 *)buf)[x*spp+c]) / float(0xFFFF);
		}
		else if (bpp == 32)
		{
		    if (format==SAMPLEFORMAT_IEEEFP)
		    {
			dst[x] = float(((float *)buf)[x*spp+c]);
		    }
		    else
		    {
			dst[x] = float(((::uint32 *)buf)[x*spp+c] >> 8) / float(0xFFFFFF);
		    }
		}
	    }
	}
    }

    free(buf);

    TIFFClose(tif);

    return fimage.release();
}

static bool saveFloatTIFF(const char * fileName, const FloatImage * fimage, uint base_component, uint num_components)
{
    nvCheck(fileName != NULL);
    nvCheck(fimage != NULL);
    nvCheck(base_component + num_components <= fimage->componentCount());

    const int iW = fimage->width();
    const int iH = fimage->height();
    const int iC = num_components;

    TIFF * image = TIFFOpen(fileName, "w");

    // Open the TIFF file
    if (image == NULL)
    {
        nvDebug("Could not open '%s' for writing\n", fileName);
        return false;
    }

    TIFFSetField(image, TIFFTAG_IMAGEWIDTH,  iW);
    TIFFSetField(image, TIFFTAG_IMAGELENGTH, iH);
    TIFFSetField(image, TIFFTAG_SAMPLESPERPIXEL, iC);
    TIFFSetField(image, TIFFTAG_SAMPLEFORMAT, SAMPLEFORMAT_IEEEFP);
    TIFFSetField(image, TIFFTAG_BITSPERSAMPLE, 32);

    uint32 rowsperstrip = TIFFDefaultStripSize(image, (uint32)-1);

    TIFFSetField(image, TIFFTAG_ROWSPERSTRIP, rowsperstrip);
    TIFFSetField(image, TIFFTAG_COMPRESSION, COMPRESSION_PACKBITS);
    if (num_components == 3)
    {
        // Set this so that it can be visualized with pfstools.
        TIFFSetField(image, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB);
    }
    TIFFSetField(image, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
    TIFFSetField(image, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);

    float * scanline = new float[iW * iC];
    for (int y = 0; y < iH; y++)
    {
        for (int c = 0; c < iC; c++)
        {
            const float * src = fimage->scanline(y, base_component + c);
            for (int x = 0; x < iW; x++) scanline[x * iC + c] = src[x];
        }
        if (TIFFWriteScanline(image, scanline, y, 0)==-1)
        {
            nvDebug("Error writing scanline %d\n", y);
            return false;
        }
    }
    delete [] scanline;

    // Close the file
    TIFFClose(image);
    return true;
}

#endif // defined(HAVE_TIFF)

#if defined(HAVE_OPENEXR)

namespace
{
    class ExrStream : public Imf::IStream
    {
    public:
        ExrStream(const char * name, Stream & s) : Imf::IStream(name), m_stream(s)
        {
            nvDebugCheck(s.isLoading());
        }

	virtual bool read(char c[], int n)
	{
	    m_stream.serialize(c, n);

	    if (m_stream.isError())
	    {
		throw Iex::InputExc("I/O error.");
	    }

	    return m_stream.isAtEnd();
	}

	virtual Imf::Int64 tellg()
	{
	    return m_stream.tell();
	}

	virtual void seekg(Imf::Int64 pos)
	{
	    nvDebugCheck(pos >= 0 && pos < UINT_MAX);
	    m_stream.seek((uint)pos);
	}

	virtual void clear()
	{
	    m_stream.clearError();
	}

    private:
        Stream & m_stream;
    };

    static int channelIndexFromName(const char* name)
    {
        char c = tolower(name[0]);
        switch (c)
        {
        default:
        case 'r':
            return 0;
        case 'g':
            return 1;
        case 'b':
            return 2;
        case 'a':
            return 3;
        }
    }

} // namespace

static FloatImage * loadFloatEXR(const char * fileName, Stream & s)
{
    nvCheck(s.isLoading());
    nvCheck(!s.isError());

    ExrStream stream(fileName, s);
    Imf::InputFile inputFile(stream);

    Imath::Box2i box = inputFile.header().dataWindow();

    int width = box.max.x - box.min.y + 1;
    int height = box.max.x - box.min.y + 1;

    const Imf::ChannelList & channels = inputFile.header().channels();

    // Count channels.
    uint channelCount= 0;
    for (Imf::ChannelList::ConstIterator it = channels.begin(); it != channels.end(); ++it)
    {
        channelCount++;
    }

    // Allocate FloatImage.
    AutoPtr<FloatImage> fimage(new FloatImage());
    fimage->allocate(channelCount, width, height);

    // Describe image's layout with a framebuffer.
    Imf::FrameBuffer frameBuffer;
    uint i = 0;
    for (Imf::ChannelList::ConstIterator it = channels.begin(); it != channels.end(); ++it, ++i)
    {
        int channelIndex = channelIndexFromName(it.name());
        frameBuffer.insert(it.name(), Imf::Slice(Imf::FLOAT, (char *)fimage->channel(channelIndex), sizeof(float), sizeof(float) * width));
    }

    // Read it.
    inputFile.setFrameBuffer (frameBuffer);
    inputFile.readPixels (box.min.y, box.max.y);

    return fimage.release();
}

static bool saveFloatEXR(const char * fileName, const FloatImage * fimage, uint base_component, uint num_components)
{
    nvCheck(fileName != NULL);
    nvCheck(fimage != NULL);
    nvCheck(base_component + num_components <= fimage->componentCount());
    nvCheck(num_components > 0 && num_components <= 4);

    const int w = fimage->width();
    const int h = fimage->height();

    const char * channelNames[] = {"R", "G", "B", "A"};

    Imf::Header header (w, h);

    for (uint c = 0; c < num_components; c++)
    {
        header.channels().insert(channelNames[c], Imf::Channel(Imf::FLOAT));
    }

    Imf::OutputFile file(fileName, header);
    Imf::FrameBuffer frameBuffer;

    for (uint c = 0; c < num_components; c++)
    {
        char * channel = (char *) fimage->channel(base_component + c);
        frameBuffer.insert(channelNames[c], Imf::Slice(Imf::FLOAT, channel, sizeof(float), sizeof(float) * w));
    }

    file.setFrameBuffer(frameBuffer);
    file.writePixels(h);

    return true;
}

#endif // defined(HAVE_OPENEXR)


#if defined(HAVE_FREEIMAGE)

static unsigned DLL_CALLCONV ReadProc(void *buffer, unsigned size, unsigned count, fi_handle handle)
{
    Stream * s = (Stream *) handle;
    s->serialize(buffer, size * count);
    return count;
}

static unsigned DLL_CALLCONV WriteProc(void *buffer, unsigned size, unsigned count, fi_handle handle)
{
    Stream * s = (Stream *) handle;
    s->serialize(buffer, size * count);
    return count;
}

static int DLL_CALLCONV SeekProc(fi_handle handle, long offset, int origin)
{
    Stream * s = (Stream *) handle;

    switch(origin) {
        case SEEK_SET :
            s->seek(offset);
            break;
        case SEEK_END :
            s->seek(s->size() + offset);
            break;
        case SEEK_CUR :
            s->seek(s->tell() + offset);
            break;
        default :
            return 1;
    }

    return 0;
}

static long DLL_CALLCONV TellProc(fi_handle handle)
{
    Stream * s = (Stream *) handle;
    return s->tell();
}


Image * nv::ImageIO::loadFreeImage(FREE_IMAGE_FORMAT fif, Stream & s)
{
    nvCheck(!s.isError());

    FreeImageIO io;
    io.read_proc = ReadProc;
    io.write_proc = NULL;
    io.seek_proc = SeekProc;
    io.tell_proc = TellProc;

    FIBITMAP * bitmap = FreeImage_LoadFromHandle(fif, &io, (fi_handle)&s, 0);

    if (bitmap == NULL)
    {
        return NULL;
    }

    const int w = FreeImage_GetWidth(bitmap);
    const int h = FreeImage_GetHeight(bitmap);

    if (FreeImage_GetImageType(bitmap) != FIT_BITMAP)
    {
        // @@ Use tone mapping?
        FIBITMAP * tmp = FreeImage_ConvertToType(bitmap, FIT_BITMAP, true);
        FreeImage_Unload(bitmap);
        bitmap = tmp;
    }

    nvDebugCheck(FreeImage_GetImageType(bitmap) == FIT_BITMAP);
    if (FreeImage_GetBPP(bitmap) != 32)
    {
        FIBITMAP * tmp = FreeImage_ConvertTo32Bits(bitmap);
        FreeImage_Unload(bitmap);
        bitmap = tmp;
    }


    Image * image = new Image();
    image->allocate(w, h, 1); // freeimage can only load 2d images:

    // Copy the image over to our internal format, FreeImage has the scanlines bottom to top though.
    for (int y=0; y < h; y++)
    {
        const void * src = FreeImage_GetScanLine(bitmap, h - y - 1);
        void * dst = image->scanline(y);

        memcpy(dst, src, 4 * w);
    }

    FreeImage_Unload(bitmap);

    return image;
}

FloatImage * nv::ImageIO::loadFloatFreeImage(FREE_IMAGE_FORMAT fif, Stream & s)
{
    nvCheck(!s.isError());

    FreeImageIO io;
    io.read_proc = ReadProc;
    io.write_proc = NULL;
    io.seek_proc = SeekProc;
    io.tell_proc = TellProc;

    FIBITMAP * bitmap = FreeImage_LoadFromHandle(fif, &io, (fi_handle)&s, 0);

    if (bitmap == NULL)
    {
        return NULL;
    }

    const int w = FreeImage_GetWidth(bitmap);
    const int h = FreeImage_GetHeight(bitmap);

    FREE_IMAGE_TYPE fit = FreeImage_GetImageType(bitmap);

    FloatImage * floatImage = new FloatImage();

    switch (fit)
    {
        case FIT_BITMAP:
            floatImage->allocate(4, w, h);
            {
                FIBITMAP * tmp = FreeImage_ConvertTo32Bits(bitmap);

                uint bitcount = FreeImage_GetBPP(bitmap);
                uint byteCount = bitcount / 8;

                for (int y=0; y < h; y++)
                {
                    const Color32 * src = (const Color32 *)FreeImage_GetScanLine(bitmap, h - y - 1 );

                    float * r = floatImage->scanline(y, 0);
                    float * g = floatImage->scanline(y, 1);
                    float * b = floatImage->scanline(y, 2);
                    float * a = floatImage->scanline(y, 3);

                    for (int x=0; x < w; x++)
                    {
                        r[x] = float(src[x].r) / 255.0f;
                        g[x] = float(src[x].g) / 255.0f;
                        b[x] = float(src[x].b) / 255.0f;
                        a[x] = float(src[x].a) / 255.0f;
                    }

                    src += byteCount;
                }

                FreeImage_Unload(tmp);
            }
            break;
        case FIT_FLOAT:
            floatImage->allocate(1, w, h);

            for (int y=0; y < h; y++)
            {
                const float * src = (const float *)FreeImage_GetScanLine(bitmap, h - y - 1 );
                float * dst = floatImage->scanline(y, 0);

                for (int x=0; x < w; x++)
                {
                    dst[x] = src[x];
                }
            }
            break;
        case FIT_UINT16:
            floatImage->allocate(1, w, h);

            for (int y=0; y < h; y++)
            {
                const uint16 * src = (const uint16 *)FreeImage_GetScanLine(bitmap, h - y - 1 );
                float * dst = floatImage->scanline(y, 0);

                for (int x=0; x < w; x++)
                {
                    dst[x] = float(src[x]) / 65535;
                }
            }
            break;
        case FIT_COMPLEX:
            floatImage->allocate(2, w, h);

            for (int y=0; y < h; y++)
            {
                const FICOMPLEX * src = (const FICOMPLEX *)FreeImage_GetScanLine(bitmap, h - y - 1 );

                float * dst_real = floatImage->scanline(y, 0);
                float * dst_imag = floatImage->scanline(y, 1);

                for (int x=0; x < w; x++)
                {
                    dst_real[x] = (float)src[x].r;
                    dst_imag[x] = (float)src[x].i;
                }
            }
            break;
        case FIT_RGBF:
            floatImage->allocate(3, w, h);

            for (int y=0; y < h; y++)
            {
                const FIRGBF * src = (const FIRGBF *)FreeImage_GetScanLine(bitmap, h - y - 1 );

                float * dst_red = floatImage->scanline(y, 0);
                float * dst_green = floatImage->scanline(y, 1);
                float * dst_blue = floatImage->scanline(y, 2);

                for (int x=0; x < w; x++)
                {
                    dst_red[x] = src[x].red;
                    dst_green[x] = src[x].green;
                    dst_blue[x] = src[x].blue;
                }
            }
            break;
        case FIT_RGBAF:
            floatImage->allocate(4, w, h);

            for (int y=0; y < h; y++)
            {
                const FIRGBAF * src = (const FIRGBAF *)FreeImage_GetScanLine(bitmap, h - y - 1 );

                float * dst_red = floatImage->scanline(y, 0);
                float * dst_green = floatImage->scanline(y, 1);
                float * dst_blue = floatImage->scanline(y, 2);
                float * dst_alpha = floatImage->scanline(y, 3);

                for (int x=0; x < w; x++)
                {
                    dst_red[x] = src[x].red;
                    dst_green[x] = src[x].green;
                    dst_blue[x] = src[x].blue;
                    dst_alpha[x] = src[x].alpha;
                }
            }
            break;
        default:
            delete floatImage;
            floatImage = NULL;
    }

    FreeImage_Unload(bitmap);

    return floatImage;
}

bool nv::ImageIO::saveFreeImage(FREE_IMAGE_FORMAT fif, Stream & s, const Image * img, const char ** tags)
{
    nvCheck(!s.isError());

    FreeImageIO io;
    io.read_proc = NULL;
    io.write_proc = WriteProc;
    io.seek_proc = SeekProc;
    io.tell_proc = TellProc;

    const uint w = img->width();
    const uint h = img->height();

    FIBITMAP * bitmap = FreeImage_Allocate(w, h, 32);

    for (uint i = 0; i < h; i++)
    {
        uint8 * scanline = FreeImage_GetScanLine(bitmap, i);
        memcpy(scanline, img->scanline(h - i - 1), w * sizeof(Color32));
    }

    if (tags != NULL)
    {
    #pragma NV_MESSAGE("TODO: Save image metadata")
        //FreeImage_SetMetadata(
    }

    bool result = FreeImage_SaveToHandle(fif, bitmap, &io, (fi_handle)&s, 0) != 0;

    FreeImage_Unload(bitmap);

    return result;
}

bool nv::ImageIO::saveFloatFreeImage(FREE_IMAGE_FORMAT fif, Stream & s, const FloatImage * img, uint baseComponent, uint componentCount)
{
    nvCheck(!s.isError());

    FreeImageIO io;
    io.read_proc = NULL;
    io.write_proc = WriteProc;
    io.seek_proc = SeekProc;
    io.tell_proc = TellProc;

    const uint w = img->width();
    const uint h = img->height();

    FREE_IMAGE_TYPE type;
    if (componentCount == 1)
    {
        type = FIT_FLOAT;
    }
    else if (componentCount == 3)
    {
        type = FIT_RGBF;
    }
    else if (componentCount == 4)
    {
        type = FIT_RGBAF;
    }
    else {
        return false;
    }


    FIBITMAP * bitmap = FreeImage_AllocateT(type, w, h);

    for (uint y = 0; y < h; y++)
    {
        float * scanline = (float *)FreeImage_GetScanLine(bitmap, y);

        for (uint x = 0; x < w; x++)
        {
            for (uint c = 0; c < componentCount; c++)
            {
                scanline[x * componentCount + c] = img->pixel(x, y, baseComponent + c);
            }
        }
    }

    bool result = FreeImage_SaveToHandle(fif, bitmap, &io, (fi_handle)&s, 0) != 0;

    FreeImage_Unload(bitmap);

    return result;
}

#endif // defined(HAVE_FREEIMAGE)


#if defined(HAVE_STBIMAGE)

static Image * loadSTB(Stream & s)
{
    // @@ Assumes stream cursor is at the beginning and that image occupies the whole stream.
    const int size = s.size();
    uint8 * buffer = new uint8[size];

    s.serialize(buffer, size);

    int w, h, n;
    uint8 * data = stbi_load_from_memory(buffer, size, &w, &h, &n, 4);

    delete [] buffer;

    if (data != NULL) {
        Image * img = new Image;
        img->allocate(w, h);
        img->setFormat(n == 4 ? Image::Format_ARGB : Image::Format_RGB);

        for (int y = 0; y < h; ++y)
        {
            nv::Color32* dest = img->scanline(y);
            uint8* src = data + y * w * 4;

            for (int x = 0; x < w; ++x)
            {
                dest[x].r = src[x * 4 + 0];
                dest[x].g = src[x * 4 + 1];
                dest[x].b = src[x * 4 + 2];
                dest[x].a = src[x * 4 + 3];
            }
        }
        
        free(data);

        return img;
    }

    return NULL;
}

static FloatImage * loadFloatSTB(Stream & s)
{
    // @@ Assumes stream cursor is at the beginning and that image occupies the whole stream.
    const int size = s.size();
    uint8 * buffer = new uint8[size];

    s.serialize(buffer, size);

    int w, h, n;
    float * data = stbi_loadf_from_memory(buffer, size, &w, &h, &n, 0);

    delete [] buffer;

    // Copy to image.
    if (data != NULL) {
        FloatImage * img = new FloatImage;
        img->allocate(n, w, h);

        const int count = w * h;

        for (int c = 0; c < n; c++) {
            float * dst = img->channel(c);

            for (int i = 0; i < count; i++) {
                dst[i] = data[i*n + c];
            }
        }
        return img;
    }

    return NULL;
}

#endif // defined(HAVE_STBIMAGE)





Image * nv::ImageIO::load(const char * fileName)
{
    nvDebugCheck(fileName != NULL);

    StdInputStream stream(fileName);

    if (stream.isError()) {
        return NULL;
    }

    return ImageIO::load(fileName, stream);
}

Image * nv::ImageIO::load(const char * fileName, Stream & s)
{
    nvDebugCheck(fileName != NULL);
    nvDebugCheck(s.isLoading());

    const char * extension = Path::extension(fileName);

    if (strCaseDiff(extension, ".tga") == 0) {
        return loadTGA(s);
    }

    if (strCaseDiff(extension, ".psd") == 0) {
        return loadPSD(s);
    }

    /*if (strCaseDiff(extension, ".ppm") == 0) {
        return loadPPM(s);
    }*/

#if defined(HAVE_JPEG)
    if (strCaseDiff(extension, ".jpg") == 0 || strCaseDiff(extension, ".jpeg") == 0) {
        return loadJPG(s);
    }
#endif

#if defined(HAVE_PNG)
    if (strCaseDiff(extension, ".png") == 0) {
        return loadPNG(s);
    }
#endif

#if defined(HAVE_FREEIMAGE)
    FREE_IMAGE_FORMAT fif = FreeImage_GetFIFFromFilename(fileName);
    if (fif != FIF_UNKNOWN && FreeImage_FIFSupportsReading(fif)) {
        return loadFreeImage(fif, s);
    }
#endif

#if defined(HAVE_STBIMAGE)
    return loadSTB(s);
#endif

    return NULL;
}

bool nv::ImageIO::save(const char * fileName, Stream & s, const Image * img, const char ** tags/*=NULL*/)
{
    nvDebugCheck(fileName != NULL);
    nvDebugCheck(s.isSaving());
    nvDebugCheck(img != NULL);

    const char * extension = Path::extension(fileName);

    if (strCaseDiff(extension, ".tga") == 0) {
        return saveTGA(s, img);
    }

    if (strCaseDiff(extension, ".ppm") == 0) {
        return savePPM(s, img);
    }

#if defined(HAVE_PNG)
    if (strCaseDiff(extension, ".png") == 0) {
        return savePNG(s, img, tags);
    }
#endif

#if defined(HAVE_FREEIMAGE)
    FREE_IMAGE_FORMAT fif = FreeImage_GetFIFFromFilename(fileName);
    if (fif != FIF_UNKNOWN && FreeImage_FIFSupportsWriting(fif)) {
        return saveFreeImage(fif, s, img, tags);
    }
#endif

    return false;
}

bool nv::ImageIO::save(const char * fileName, const Image * img, const char ** tags/*=NULL*/)
{
    nvDebugCheck(fileName != NULL);
    nvDebugCheck(img != NULL);

    StdOutputStream stream(fileName);
    if (stream.isError())
    {
        return false;
    }

    return ImageIO::save(fileName, stream, img, tags);
}

FloatImage * nv::ImageIO::loadFloat(const char * fileName)
{
    nvDebugCheck(fileName != NULL);

    StdInputStream stream(fileName);

    if (stream.isError()) {
        return NULL;
    }

    return loadFloat(fileName, stream);
}

FloatImage * nv::ImageIO::loadFloat(const char * fileName, Stream & s)
{
    nvDebugCheck(fileName != NULL);

    const char * extension = Path::extension(fileName);

    /*if (strCaseDiff(extension, ".pfm") == 0) {
        return loadFloatPFM(s);
    }*/

#if defined(HAVE_TIFF)
    #pragma NV_MESSAGE("TODO: Load TIFF from stream.")
    if (strCaseDiff(extension, ".tif") == 0 || strCaseDiff(extension, ".tiff") == 0) {
        return loadFloatTIFF(fileName, s);
    }
#endif

#if defined(HAVE_OPENEXR)
    #pragma NV_MESSAGE("TODO: Load EXR from stream.")
    if (strCaseDiff(extension, ".exr") == 0) {
        return loadFloatEXR(fileName, s);
    }
#endif

#if defined(HAVE_FREEIMAGE)
    FREE_IMAGE_FORMAT fif = FreeImage_GetFIFFromFilename(fileName);
    if (fif != FIF_UNKNOWN && FreeImage_FIFSupportsReading(fif)) {
        return loadFloatFreeImage(fif, s);
    }
#endif

    if (strCaseDiff(extension, ".dds") == 0) {
        const uint spos = s.tell(); // Save stream position.
        FloatImage * floatImage = loadFloatDDS(s);
        if (floatImage != NULL) return floatImage;
        else s.seek(spos);
    }

    // Try to load as an RGBA8 image and convert to float.
    AutoPtr<Image> img(load(fileName, s));
    if (img != NULL) {
        return new FloatImage(img.ptr());
    }

    return NULL;
}

bool nv::ImageIO::saveFloat(const char * fileName, Stream & s, const FloatImage * fimage, uint baseComponent, uint componentCount)
{
    if (componentCount == 0) {
        componentCount = fimage->componentCount() - baseComponent;
    }
    if (baseComponent + componentCount < fimage->componentCount()) {
        return false;
    }

    const char * extension = Path::extension(fileName);

    if (strCaseDiff(extension, ".dds") == 0) {
        return saveFloatDDS(s, fimage, baseComponent, componentCount);
    }

    /*if (strCaseDiff(extension, ".pfm") == 0) {
        return saveFloatPFM(s, fimage, baseComponent, componentCount);
    }*/

#if defined(HAVE_FREEIMAGE)
    FREE_IMAGE_FORMAT fif = FreeImage_GetFIFFromFilename(fileName);
    if (fif != FIF_UNKNOWN && FreeImage_FIFSupportsWriting(fif)) {
        return saveFloatFreeImage(fif, s, fimage, baseComponent, componentCount);
    }
#endif

    // If everything else fails, save as LDR.
    if (componentCount <= 4)
    {
        AutoPtr<Image> image(fimage->createImage(baseComponent, componentCount));
        nvCheck(image != NULL);

        if (componentCount == 1)
        {
            Color32 * c = image->pixels();
            const uint count = image->width() * image->height();
            for (uint i = 0; i < count; i++)
            {
                c[i].b = c[i].g = c[i].r;
            }
        }

        if (componentCount == 4)
        {
            image->setFormat(Image::Format_ARGB);
        }

        return ImageIO::save(fileName, s, image.ptr());
    }

    return false;
}

bool nv::ImageIO::saveFloat(const char * fileName, const FloatImage * fimage, uint baseComponent, uint componentCount)
{
    if (componentCount == 0) {
        componentCount = fimage->componentCount() - baseComponent;
    }
    if (baseComponent + componentCount < fimage->componentCount()) {
        return false;
    }

    const char * extension = Path::extension(fileName);

#if defined(HAVE_OPENEXR)
    if (strCaseDiff(extension, ".exr") == 0) {
        return saveFloatEXR(fileName, fimage, baseComponent, componentCount);
    }
#endif

#if defined(HAVE_TIFF)
    if (strCaseDiff(extension, ".tif") == 0 || strCaseDiff(extension, ".tiff") == 0) {
        return saveFloatTIFF(fileName, fimage, baseComponent, componentCount);
    }
#endif

    StdOutputStream stream(fileName);

    if (stream.isError()) {
        return false;
    }

    return saveFloat(fileName, stream, fimage, baseComponent, componentCount);
}
