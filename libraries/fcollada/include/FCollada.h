/*
    Copyright (C) 2005-2007 Feeling Software Inc.
    MIT License: http://www.opensource.org/licenses/mit-license.php
*/

/**
 * @mainpage FCollada Documentation
 *
 * @section intro_sec Introduction
 * The FCollada classes are designed to read and write Collada files.
 *
 * @section install_sec Installation
 *
 * @subsection step1 Step 1: Download
 * You can download the FCollada libraries from our website: http://www.feelingsoftware.com
 *
 * @section copyright Copyright
 * Copyright (C) 2005-2007 Feeling Software Inc.
 * MIT License: http://www.opensource.org/licenses/mit-license.php
 */

#ifndef _FCOLLADA_H_
#define _FCOLLADA_H_

/**
	FCollada exception handling.
	Force this #define to 0 to disallow exception handling within the FCollada library.
	By default, a debug library will no handle exceptions so that your debugger can.
	In release, all exceptions should be handled so that your users receive a meaningful message,
	rather than crash your application. Force this #define to 0 only if your platform does not
	support exception handling.
*/
#ifdef _DEBUG
#define	FCOLLADA_EXCEPTION 0
#else
#define FCOLLADA_EXCEPTION 1
#endif

#define PREMIUM

#include "FUtils/FUtils.h"

/** 
	FCollada version number.
	You should verify that you have the correct version, if you use the FCollada library as a DLLs.
	For a history of version, check the Changes.txt file.

	Most significant 16 bits represents the major version number and least
	significant 16 bits represents the minor number of the major version. 
*/
#define FCOLLADA_VERSION 0x00030002 /* MMMM.NNNN */

// The main FCollada class: the document object.
class FCDocument;
typedef fm::pvector<FCDocument> FCDocumentList;

/** 
	This namespace contains FCollada global functions and member variables
*/
namespace FCollada
{
	/** Retrieves the FCollada version number.
		Used for DLL-versions of the FCollada library: verify that you have a compatible version
		of the FCollada library using this function.
		@return The FCollada version number. */
	FCOLLADA_EXPORT unsigned long GetVersion();

	/**	Creates a new top FCDocument object.
		You must used this function to create your top FCDocument object if you use a lot of external references.
		@return A new top document object. */
	FCOLLADA_EXPORT FCDocument* NewTopDocument();

	/** Retrieves the number of top documents.
		@return The number of top documents. */
	FCOLLADA_EXPORT size_t GetTopDocumentCount();

	/** Retrieves a top document.
		@param index The index of the top document.
		@return The top document at the given index. */
	FCOLLADA_EXPORT FCDocument* GetTopDocument(size_t index);

	/** Retrieves whether a document is a top document.
		@param document The document to verify.
		@return Whether the document is a top document. */
	FCOLLADA_EXPORT bool IsTopDocument(FCDocument* document);

	/** Retrieves the list of all the document currently loaded by FCollada.
		@param documents The list of documents to fill in.
			Thist list is cleared of all its content at the beginning of the function. */
	FCOLLADA_EXPORT void GetAllDocuments(FCDocumentList& documents);

	/** Retrieves the global dereferencing flag.
		Setting this flag will force all the entity instance to automatically
		attempt to open the externally-referenced documents when needed.
		Defaults: true.
		@return Whether to automatically dereference the entity instances. */
	FCOLLADA_EXPORT bool GetDereferenceFlag();

	/** Sets the global dereferencing flag.
		Setting this flag will force all the entity instance to automatically
		attempt to open the externally-referenced documents when needed.
		Defaults: true.
		@param flag Whether to automatically dereference the entity instances. */
	FCOLLADA_EXPORT void SetDereferenceFlag(bool flag);
}

#endif // _FCOLLADA_H_
