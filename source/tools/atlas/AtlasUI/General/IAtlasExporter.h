#ifndef IATLASEXPORTER_H__
#define IATLASEXPORTER_H__

class AtObj;

class IAtlasExporter // and also importer, but I can't think of a nice concise name
{
public:
	virtual void Import(AtObj& in)=0;
	virtual void Export(AtObj& out)=0;
};

#endif // IATLASEXPORTER_H__
