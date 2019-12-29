// This code is in the public domain -- Ignacio Castaño <castano@gmail.com>

#pragma once
#ifndef NV_IMAGE_KTXFILE_H
#define NV_IMAGE_KTXFILE_H

#include "nvimage.h"
#include "nvcore/StrLib.h"

// KTX File format specification:
// http://www.khronos.org/opengles/sdk/tools/KTX/file_format_spec/#key

namespace nv
{
    class Stream;

    // GL types (Table 3.2)
    const uint KTX_UNSIGNED_BYTE;
    const uint KTX_UNSIGNED_SHORT_5_6_5;
    // ...

    // GL formats (Table 3.3)
    // ...

    // GL internal formats (Table 3.12, 3.13)
    // ...

    // GL base internal format. (Table 3.11)
    const uint KTX_RGB;
    const uint KTX_RGBA;
    const uint KTX_ALPHA;
    // ...


    struct KtxHeader {
        uint8 identifier[12];
        uint32 endianness;
        uint32 glType;
        uint32 glTypeSize;
        uint32 glFormat;
        uint32 glInternalFormat;
        uint32 glBaseInternalFormat;
        uint32 pixelWidth;
        uint32 pixelHeight;
        uint32 pixelDepth;
        uint32 numberOfArrayElements;
        uint32 numberOfFaces;
        uint32 numberOfMipmapLevels;
        uint32 bytesOfKeyValueData;

        KtxHeader();

    };

    NVIMAGE_API Stream & operator<< (Stream & s, DDSHeader & header);


    struct KtxFile {
        KtxFile();
        ~KtxFile();

        void addKeyValue(const char * key, const char * value);

    private:
        KtxHeader header;

        Array<String> keyArray;
        Array<String> valueArray;

    };

    NVIMAGE_API Stream & operator<< (Stream & s, KtxFile & file);


    /*
    for each keyValuePair that fits in bytesOfKeyValueData
        UInt32   keyAndValueByteSize
        Byte     keyAndValue[keyAndValueByteSize]
        Byte     valuePadding[3 - ((keyAndValueByteSize + 3) % 4)]
    end

    for each mipmap_level in numberOfMipmapLevels*
        UInt32 imageSize;
        for each array_element in numberOfArrayElements*
           for each face in numberOfFaces
               for each z_slice in pixelDepth*
                   for each row or row_of_blocks in pixelHeight*
                       for each pixel or block_of_pixels in pixelWidth
                           Byte data[format-specific-number-of-bytes]**
                       end
                   end
               end
               Byte cubePadding[0-3]
           end
        end
        Byte mipPadding[3 - ((imageSize + 3) % 4)]
    end
    */

} // nv namespace

#endif // NV_IMAGE_KTXFILE_H
