/* $Id$

  Xeromyces file-loading interface.
  Automatically creates and caches relatively
  efficient binary representations of XML files.

  - Philip Taylor (philip@zaynar.demon.co.uk / @wildfiregames.com)

*/

#ifndef _XEROMYCES_H_
#define _XEROMYCES_H_

#include "ps/Errors.h"
ERROR_GROUP(Xeromyces);
ERROR_TYPE(Xeromyces, XMLOpenFailed);
ERROR_TYPE(Xeromyces, XMLParseError);

#include "ps/XeroXMB.h"
#include "CVFSFile.h"

class CXeromyces : public XMBFile
{
public:
	CXeromyces();
	~CXeromyces();

	// Load from an XML file (with invisible XMB caching).
	PSRETURN Load(const char* filename);

	// Call once when shutting down the program, to unload Xerces.
	static void Terminate();

private:
	bool ReadXMBFile(const char* filename);

	XMBFile* XMB;
	CVFSFile* XMBFileHandle; // if it's being read from disk
	char* XMBBuffer; // if it's being read from RAM

	static int XercesLoaded; // for once-only initialisation
};


#endif // _XEROMYCES_H_
