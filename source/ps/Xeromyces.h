/* $Id: Xeromyces.h,v 1.2 2004/07/10 18:57:13 olsner Exp $

  Xeromyces file-loading interface.
  Automatically creates and caches relatively
  efficient binary representations of XML files.

  - Philip Taylor (philip@zaynar.demon.co.uk / @wildfiregames.com)

*/

#ifndef _XEROMYCES_H_
#define _XEROMYCES_H_

#include "ps/XeroXMB.h"

#include "lib/res/vfs.h"

class CXeromyces : public XMBFile
{
public:
	CXeromyces();
	~CXeromyces();

	// Load from an XML file (with invisible XMB caching).
	// Throws a const char* if stuff breaks.
	void Load(const char* filename);

	// Call once when shutting down the program.
	static void Terminate();

	// Get the XMBFile after having called Load
//	XMBFile* GetXMB() { assert(XMB); return XMB; }


private:
	bool ReadXMBFile(const char* filename, bool CheckCRC, unsigned long CRC);

	XMBFile* XMB;
	Handle XMBFileHandle; // if it's being read from disk
	char* XMBBuffer; // if it's being read from RAM

	static int XercesLoaded; // for once-only initialisation
};


#endif // _XEROMYCES_H_
