
/*
For more info:
http://developer.valvesoftware.com/wiki/VTF

File Layout:
    VTF Header
    VTF Low Resolution Image Data
    For Each Mipmap (Smallest to Largest)
      For Each Frame (First to Last)
        For Each Face (First to Last)
          For Each Z Slice (Min to Max; Varies with Mipmap)
            VTF High Resolution Image Data


*/


enum
{
    IMAGE_FORMAT_NONE = -1,
    IMAGE_FORMAT_RGBA8888 = 0,
    IMAGE_FORMAT_ABGR8888,
    IMAGE_FORMAT_RGB888,
    IMAGE_FORMAT_BGR888,
    IMAGE_FORMAT_RGB565,
    IMAGE_FORMAT_I8,
    IMAGE_FORMAT_IA88,
    IMAGE_FORMAT_P8,
    IMAGE_FORMAT_A8,
    IMAGE_FORMAT_RGB888_BLUESCREEN,
    IMAGE_FORMAT_BGR888_BLUESCREEN,
    IMAGE_FORMAT_ARGB8888,
    IMAGE_FORMAT_BGRA8888,
    IMAGE_FORMAT_DXT1,
    IMAGE_FORMAT_DXT3,
    IMAGE_FORMAT_DXT5,
    IMAGE_FORMAT_BGRX8888,
    IMAGE_FORMAT_BGR565,
    IMAGE_FORMAT_BGRX5551,
    IMAGE_FORMAT_BGRA4444,
    IMAGE_FORMAT_DXT1_ONEBITALPHA,
    IMAGE_FORMAT_BGRA5551,
    IMAGE_FORMAT_UV88,
    IMAGE_FORMAT_UVWQ8888,
    IMAGE_FORMAT_RGBA16161616F,
    IMAGE_FORMAT_RGBA16161616,
    IMAGE_FORMAT_UVLX8888,
	IMAGE_FORMAT_R32F,						//!<  = Luminance - 32 bpp
	IMAGE_FORMAT_RGB323232F,				//!<  = Red, Green, Blue - 96 bpp
	IMAGE_FORMAT_RGBA32323232F,				//!<  = Red, Green, Blue, Alpha - 128 bpp
	IMAGE_FORMAT_NV_DST16,
	IMAGE_FORMAT_NV_DST24,					
	IMAGE_FORMAT_NV_INTZ,
	IMAGE_FORMAT_NV_RAWZ,
	IMAGE_FORMAT_ATI_DST16,
	IMAGE_FORMAT_ATI_DST24,
	IMAGE_FORMAT_NV_NULL,
	IMAGE_FORMAT_ATI2N,						
	IMAGE_FORMAT_ATI1N,
};


enum
{
    TEXTUREFLAGS_POINTSAMPLE = 0x00000001,
    TEXTUREFLAGS_TRILINEAR = 0x00000002,
    TEXTUREFLAGS_CLAMPS = 0x00000004,
    TEXTUREFLAGS_CLAMPT = 0x00000008,
    TEXTUREFLAGS_ANISOTROPIC = 0x00000010,
    TEXTUREFLAGS_HINT_DXT5 = 0x00000020,
    TEXTUREFLAGS_NOCOMPRESS = 0x00000040,
    TEXTUREFLAGS_NORMAL = 0x00000080,
    TEXTUREFLAGS_NOMIP = 0x00000100,
    TEXTUREFLAGS_NOLOD = 0x00000200,
    TEXTUREFLAGS_MINMIP = 0x00000400,
    TEXTUREFLAGS_PROCEDURAL = 0x00000800,
    TEXTUREFLAGS_ONEBITALPHA = 0x00001000,
    TEXTUREFLAGS_EIGHTBITALPHA = 0x00002000,
    TEXTUREFLAGS_ENVMAP = 0x00004000,
    TEXTUREFLAGS_RENDERTARGET = 0x00008000,
    TEXTUREFLAGS_DEPTHRENDERTARGET = 0x00010000,
    TEXTUREFLAGS_NODEBUGOVERRIDE = 0x00020000,
    TEXTUREFLAGS_SINGLECOPY = 0x00040000,
    TEXTUREFLAGS_ONEOVERMIPLEVELINALPHA = 0x00080000,
    TEXTUREFLAGS_PREMULTCOLORBYONEOVERMIPLEVEL = 0x00100000,
    TEXTUREFLAGS_NORMALTODUDV = 0x00200000,
    TEXTUREFLAGS_ALPHATESTMIPGENERATION = 0x00400000,
    TEXTUREFLAGS_NODEPTHBUFFER = 0x00800000,
    TEXTUREFLAGS_NICEFILTERED = 0x01000000,
    TEXTUREFLAGS_CLAMPU = 0x02000000
};


struct VtfHeader
{
    char signature[4];          // File signature ("VTF\0").
    uint32 version[2];           // version[0].version[1] (currently 7.2).
    uint32 headerSize;          // Size of the header struct (16 byte aligned; currently 80 bytes).
    
    // 7.0
    uint16 width;                 // Width of the largest mipmap in pixels. Must be a power of 2.
    uint16 height;            // Height of the largest mipmap in pixels. Must be a power of 2.
    uint32 flags;            // VTF flags.
    uint16 frames;            // Number of frames, if animated (1 for no animation).
    uint16 firstFrame;        // First frame in animation (0 based).
    uint8 padding0[4];      // reflectivity padding (16 byte alignment).
    float reflectivity[3];      // reflectivity vector.
    uint8 padding1[4];        // reflectivity padding (8 byte packing).
    float bumpmapScale;           // Bumpmap scale.
    uint32 highResImageFormat;  // High resolution image format.
    uint8 mipmapCount;              // Number of mipmaps.
    uint32 lowResImageFormat;    // Low resolution image format (always DXT1).
    uint8 lowResImageWidth;        // Low resolution image width.
    uint8 lowResImageHeight;        // Low resolution image height.
    
    // 7.2
    uint16 depth;                        // Depth of the largest mipmap in pixels.
                                            // Must be a power of 2. Can be 0 or 1 for a 2D texture (v7.2 only).
};


