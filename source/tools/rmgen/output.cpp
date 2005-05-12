#include "stdafx.h"
#include "rmgen.h"
#include "output.h"
#include "map.h"

using namespace std;

const char* OUTPUT_PATH = "../../../binaries/data/mods/official/maps/scenarios/";

typedef unsigned short u16;
typedef unsigned int u32;


void OutputXml(Map* m, FILE* f) {
	const char* xml = "\
<?xml version=\"1.0\" encoding=\"utf-8\" standalone=\"no\"?>\n\
<!DOCTYPE Scenario SYSTEM \"/maps/scenario_v4.dtd\">\n\
<Scenario>\n\
	<Environment>\n\
			<SunColour r=\"1\" g=\"1\" b=\"1\" />\n\
			<SunElevation angle=\"0.785398\" />\n\
			<SunRotation angle=\"4.712389\" />\n\
			<TerrainAmbientColour r=\"0\" g=\"0\" b=\"0\" />\n\
			<UnitsAmbientColour r=\"0.4\" g=\"0.4\" b=\"0.4\" />\n\
	</Environment>\n\
	<Entities />\n\
	<Nonentities />\n\
</Scenario>\n";
	fprintf(f, "%s", xml);
}

struct Tile {
	u16 texture1; // index into terrain_textures[]
	u16 texture2; // index, or 0xFFFF for 'none'
	u32 priority; // ???
};

void OutputPmp(Map* m, FILE* f) {
/*
struct String {
    u32 length;
    char data[length]; // not NUL-terminated
};
struct PMP {
    char header[4]; // == "PSMP"
    u32 version; // == 4
    u32 data_size; // == filesize-12
    

    u32 map_size; // number of patches (16x16 tiles) per side

    u16 heightmap[(mapsize*16 + 1)^2]; // (squared, not xor) - vertex heights

    u32 num_terrain_textures;
    String terrain_textures[num_terrain_textures]; // filenames (no path), e.g. "cliff1.dds"

    Tile tiles[(mapsize*16)^2];
};*/
	int size = m->size;
	int numTerrains = m->idToName.size();

	// header
	fwrite("PSMP", sizeof(char), 4, f);

	// file format version
	u32 version = 4;
	fwrite(&version, sizeof(u32), 1, f);

	// data size
	/*int numTextureChars = 0;
	for(int i=0; i<numTerrains; i++) {
		numTextureChars += (m->idToName[i]+".dds").length();
	}
	u32 dataSize = sizeof(u32)					// size in patches
		+ (size+1)*(size+1)*sizeof(u16)			// heightmap
		+ sizeof(u32)							// num textures
		+ m->idToName.size()*sizeof(u32)		// texture string lengths
		+ numTextureChars*sizeof(char)			// texture string chars
		+ size*size*sizeof(Tile);				// tiles*/
	int temp = 0;
	fwrite(&temp, sizeof(u32), 1, f);

	// map size in patches
	u32 sizeInPatches = size/16;
	fwrite(&sizeInPatches, sizeof(u32), 1, f);

	// heightmap
	u16* heightmap = new u16[(size+1)*(size+1)];
	memset(heightmap, 0, (size+1)*(size+1)*sizeof(u16));
	fwrite(heightmap, sizeof(u16), (size+1)*(size+1), f);

	// num terrain textures
	fwrite(&numTerrains, sizeof(u32), 1, f);

	// terrain names
	for(int i=0; i<numTerrains; i++) {
		string fname = m->idToName[i] + ".dds";
		int len = fname.length();
		fwrite(&len, sizeof(u32), 1, f);
		fwrite(fname.c_str(), sizeof(char), fname.length(), f);
	}

	// terrains
	Tile* tiles = new Tile[size*size];
	for(int i=0; i<size; i++) {
		for(int j=0; j<size; j++) {
			Tile& t = tiles[i*size+j];
			t.texture1 = m->terrain[i][j];
			t.texture2 = 0xFFFF;
			t.priority = 0;
		}
	}
	fwrite(tiles, sizeof(Tile), size*size, f);

	// data size
	fseek(f, 0, SEEK_END);
	int fsize = ftell(f);
	u32 dataSize = fsize-12;
	fseek(f, 8, SEEK_SET);
	fwrite(&dataSize, sizeof(u32), 1, f);
}

void OutputMap(Map* m, string outputName) {
	string pmpName = OUTPUT_PATH + outputName + ".pmp";
	string xmlName = OUTPUT_PATH + outputName + ".xml";

	FILE* xmlFile = fopen(xmlName.c_str(), "w");
	if(!xmlFile) {
		cerr << "Cannot open " << xmlName << endl;
		Shutdown(1);
	}
	OutputXml(m, xmlFile);
	fclose(xmlFile);

	FILE* pmpFile = fopen(pmpName.c_str(), "wb");
	if(!pmpFile) {
		cerr << "Cannot open " << xmlName << endl;
		Shutdown(1);
	}
	OutputPmp(m, pmpFile);
	fclose(pmpFile);

	cout << "Map outputted to " << OUTPUT_PATH << outputName << ".*" << endl;
}
