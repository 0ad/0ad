/*
  Xeromyces file-loading interface.
  Automatically creates and caches relatively
  efficient binary representations of XML files.
*/

#ifndef INCLUDED_XEROMYCES
#define INCLUDED_XEROMYCES

#include "ps/Errors.h"
ERROR_GROUP(Xeromyces);
ERROR_TYPE(Xeromyces, XMLOpenFailed);
ERROR_TYPE(Xeromyces, XMLParseError);

#include "XeroXMB.h"
#include "ps/Filesystem.h"

class CXeromyces : public XMBFile
{
	friend class TestXeromyces;
public:
	CXeromyces();
	~CXeromyces();

	// Load from an XML file (with invisible XMB caching).
	PSRETURN Load(const VfsPath& filename);

	// Call once when shutting down the program, to unload Xerces.
	static void Terminate();

private:

	// Find out write location of the XMB file corresponding to xmlFilename
	static void GetXMBPath(const PIVFS& vfs, const VfsPath& xmlFilename, const VfsPath& xmbFilename, VfsPath& xmbActualPath);

	bool ReadXMBFile(const VfsPath& filename);

	shared_ptr<u8> XMBBuffer;

	static int XercesLoaded; // for once-only initialisation
};


#define _XERO_MAKE_UID2__(p,l) p ## l
#define _XERO_MAKE_UID1__(p,l) _XERO_MAKE_UID2__(p,l)

#define _XERO_CHILDREN _XERO_MAKE_UID1__(_children_, __LINE__)
#define _XERO_I _XERO_MAKE_UID1__(_i_, __LINE__)

#define XERO_ITER_EL(parent_element, child_element)					\
	XMBElementList _XERO_CHILDREN = parent_element.GetChildNodes();	\
	XMBElement child_element (0);									\
	for (int _XERO_I = 0;											\
		 _XERO_I < _XERO_CHILDREN.Count								\
			&& (child_element = _XERO_CHILDREN.Item(_XERO_I), 1);	\
		 ++_XERO_I)

#define XERO_ITER_ATTR(parent_element, attribute)						\
	XMBAttributeList _XERO_CHILDREN = parent_element.GetAttributes();	\
	XMBAttribute attribute;												\
	for (int _XERO_I = 0;												\
		 _XERO_I < _XERO_CHILDREN.Count									\
			&& (attribute = _XERO_CHILDREN.Item(_XERO_I), 1);			\
		 ++_XERO_I)

#endif // INCLUDED_XEROMYCES
