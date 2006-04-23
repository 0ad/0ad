#include "stdafx.h"

#include "SCN.h"
#include "Stream/Stream.h"
#include "Util.h"

#include <map>

using namespace DatafileIO;

struct Chunk
{
	virtual ~Chunk() {}
	virtual void Read(InputStream& stream) = 0;
	virtual void Dump(OutputStream& stream) = 0;

	char Head[2];
};

class ChunkMap : public std::map<std::string, Chunk*>
{
public:
	~ChunkMap() { for (iterator it = begin(); it != end(); ++it) delete it->second; }
};

ChunkMap& GetChunkMap()
{
	static ChunkMap c;
	return c;
}

int RegisterChunk(char* twocc, Chunk* chunk)
{
	GetChunkMap().insert(std::make_pair(twocc, chunk));
	return 0;
}

#define CAT2(a,b) a##b
#define CAT(a,b) CAT2(a,b)
#define REGISTER(twocc, type) static int CAT(OnInit_, __LINE__) = RegisterChunk(twocc, new type)

struct Chunk_Unknown : public Chunk
{
	Chunk_Unknown() : Data(NULL) {}
	~Chunk_Unknown() { delete Data; }

	void Read(InputStream& stream)
	{
		stream.Read(&Length, 4);

		Data = new char[Length];
		stream.Read(Data, Length);
	}

	void Dump(OutputStream& stream)
	{
		stream.Write(Data, Length);
	}

	uint32_t Length;
	char* Data;
};
REGISTER("??", Chunk_Unknown);

//////////////////////////////////////////////////////////////////////////

void ReadChunk(InputStream& stream)
{
	char head[2];
	stream.Read(head, 2);

	ChunkMap::iterator it = GetChunkMap().find(head);
	if (it == GetChunkMap().end())
		it = GetChunkMap().find("??");
	it->second->Read(stream);
}

//////////////////////////////////////////////////////////////////////////

namespace DatafileIO
{
	class SCNFile
	{
		
	};
}

SCNFile* LoadFromSCN(InputStream& /*stream*/)
{
	return NULL;
}
