/* $Id: Xeromyces.h,v 1.5 2004/07/18 11:50:34 philip Exp $

  Xeromyces file-loading interface.
  Automatically creates and caches relatively
  efficient binary representations of XML files.

  - Philip Taylor (philip@zaynar.demon.co.uk / @wildfiregames.com)

*/

#ifndef _XEROMYCES_H_
#define _XEROMYCES_H_

ERROR_GROUP(Xeromyces);
ERROR_TYPE(Xeromyces, XMLOpenFailed);
ERROR_TYPE(Xeromyces, XMLParseError);

#include "ps/XeroXMB.h"
#include "lib/res/h_mgr.h"

class CXeromyces : public XMBFile
{
public:
	CXeromyces();
	~CXeromyces();

	// Load from an XML file (with invisible XMB caching).
	// Throws a PSERROR_Xeromyces upon failure.
	void Load(const char* filename);

	// Call once when shutting down the program, to unload Xerces.
	static void Terminate();

private:
	bool ReadXMBFile(const char* filename, bool CheckCRC, unsigned long CRC);

	XMBFile* XMB;
	Handle XMBFileHandle; // if it's being read from disk
	char* XMBBuffer; // if it's being read from RAM

	static int XercesLoaded; // for once-only initialisation
};


#endif // _XEROMYCES_H_
