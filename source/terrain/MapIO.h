#ifndef _MAPIO_H
#define _MAPIO_H

class CMapIO 
{
public:
	// current file version given to saved maps 
	enum { FILE_VERSION = 2 };
	// supported file read version - file with version less than this will be reject
	enum { FILE_READ_VERSION = 1 };

#pragma pack(push, 1)
	// description of a tile for I/O purposes
	struct STileDesc {
		// index into the texture array of first texture on tile
		u16 m_Tex1Index;
		// index into the texture array of second texture; (0xffff) if none
		u16 m_Tex2Index;
		// priority
		u32 m_Priority;
	};

	// description of an object for I/O purposes
	struct SObjectDesc {
		// index into the object array 
		u16 m_ObjectIndex;
		// transformation matrix
		float m_Transform[16];
	};
#pragma pack(pop)
};

#endif


