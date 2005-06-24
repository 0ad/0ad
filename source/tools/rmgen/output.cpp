#include "stdafx.h"
#include "rmgen.h"
#include "output.h"
#include "map.h"
#include "entity.h"

using namespace std;

typedef unsigned short u16;
typedef unsigned int u32;

void OutputEntity(Entity* e, ostringstream& xml) {
	xml << "\
		<Entity>\n\
			<Template>" << e->type << "</Template>\n\
			<Player>" << e->player << "</Player>\n\
			<Position x=\"" << 4*e->x << "\" y=\"" << 4*e->y << "\" z=\"" << 4*e->z << "\" />\n\
			<Orientation angle=\"" << e->orientation << "\" />\n\
		</Entity>\n";
}

void OutputXml(Map* m, FILE* f) {
	ostringstream xml;
	xml << "\
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
	<Entities>\n";

	for(int i=0; i<m->entities.size(); i++) {
		OutputEntity(m->entities[i], xml);
	}

	for(int x=0; x<m->size; x++) {
		for(int y=0; y<m->size; y++) {
			vector<Entity*>& vec = m->terrainEntities[x][y];
			for(int i=0; i<vec.size(); i++) {
				OutputEntity(vec[i], xml);
			}
		}
	}

	xml << "\
	</Entities>\n\
	<Nonentities />\n\
</Scenario>\n";

	fprintf(f, "%s", xml.str().c_str());
}

struct Tile {
	u16 texture1; // index into texture_textures[]
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

	u32 num_texture_textures;
	String texture_textures[num_texture_textures]; // filenames (no path), e.g. "cliff1.dds"

	Tile tiles[(mapsize*16)^2];
};*/
	int size = m->size;
	int numTextures = m->idToName.size();

	// header
	fwrite("PSMP", sizeof(char), 4, f);

	// file format version
	u32 version = 4;
	fwrite(&version, sizeof(u32), 1, f);

	// data size (write 0 for now, calculate it at the end)
	int temp = 0;
	fwrite(&temp, sizeof(u32), 1, f);

	// map size in patches
	u32 sizeInPatches = size/16;
	fwrite(&sizeInPatches, sizeof(u32), 1, f);

	// heightmap
	u16* heightmap = new u16[(size+1)*(size+1)];
	for(int x=0; x<size+1; x++) {
		for(int y=0; y<size+1; y++) {
			int intHeight = (int) (m->height[x][y] * 256.0f / 0.35f);
			if(intHeight > 0xFFFF) {
				intHeight = 0xFFFF;
			}
			else if(intHeight < 0) {
				intHeight = 0;
			}
			heightmap[y*(size+1)+x] = intHeight;
		}
	}
	fwrite(heightmap, sizeof(u16), (size+1)*(size+1), f);

	// num texture textures
	fwrite(&numTextures, sizeof(u32), 1, f);

	// texture names
	for(int i=0; i<numTextures; i++) {
		string fname = m->idToName[i] + ".dds";
		int len = fname.length();
		fwrite(&len, sizeof(u32), 1, f);
		fwrite(fname.c_str(), sizeof(char), fname.length(), f);
	}

	
	// texture; note that this is an array of 16x16 patches for some reason
	Tile* tiles = new Tile[size*size];
	for(int x=0; x<size; x++) {
		for(int y=0; y<size; y++) {
			int patchX = x/16, patchY = y/16;
			int offX = x%16, offY = y%16;
			Tile& t = tiles[ (patchY*size/16 + patchX)*16*16 + (offY*16 + offX) ];
			t.texture1 = m->texture[x][y];
			t.texture2 = 0xFFFF;
			t.priority = 0;
		}
	}
	fwrite(tiles, sizeof(Tile), size*size, f);



	// data size (file size - 12)
	fseek(f, 0, SEEK_END);
	int fsize = ftell(f);
	u32 dataSize = fsize-12;
	fseek(f, 8, SEEK_SET);
	fwrite(&dataSize, sizeof(u32), 1, f);
}

void OutputMap(Map* m, const string& outputName) {
	string xmlName = outputName + ".xml";
	FILE* xmlFile = fopen(xmlName.c_str(), "w");
	if(!xmlFile) {
		cerr << "Cannot open " << xmlName << endl;
		Shutdown(1);
	}
	OutputXml(m, xmlFile);
	fclose(xmlFile);

	string pmpName = outputName + ".pmp";
	FILE* pmpFile = fopen(pmpName.c_str(), "wb");
	if(!pmpFile) {
		cerr << "Cannot open " << pmpName << endl;
		Shutdown(1);
	}
	OutputPmp(m, pmpFile);
	fclose(pmpFile);

	cout << "Map outputted to " << outputName << ".*" << endl;
}
