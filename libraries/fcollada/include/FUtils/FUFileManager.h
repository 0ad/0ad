/*
    Copyright (C) 2005-2007 Feeling Software Inc.
    MIT License: http://www.opensource.org/licenses/mit-license.php
*/
/*
	Based on the FS Import classes:
	Copyright (C) 2005-2006 Feeling Software Inc
	Copyright (C) 2005-2006 Autodesk Media Entertainment
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/

#ifndef _FU_FILE_MANAGER_H_
#define _FU_FILE_MANAGER_H_

class FUFile;

/** Handles most file system related operations.
	Is useful mostly for platform-abstraction and to handle the relative paths
	within COLLADA documents. */
class FCOLLADA_EXPORT FUFileManager
{
private:
	FStringList pathStack;
	bool forceAbsolute;

public:
	/** Constructor.
		When creating a new file manager, the file system's current file
		path is retrieved and placed on the file path stack. */
	FUFileManager();

	/** Destructor. */
	~FUFileManager();

	/** Retrieves the current file path.
		This is the file path used when creating all relative paths.
		@return The current file path. */
	inline const fstring& GetCurrentPath() { return pathStack.back(); }

	/** Sets a new current file path.
		Files paths are placed on a stack in order to easily return
		to previous file paths.
		@param path The new file path. */
	void PushRootPath(const fstring& path);

	/** Sets a new current file path using a document filename.
		Files paths are placed on a stack in order to easily return
		to previous file paths.
		@param path The new file path. */
	void PushRootFile(const fstring& filename);

	/** Removes the current file path from the stack. */
	void PopRootPath();
	void PopRootFile(); /**< See above. */

	/** Opens a file.
		@see FUFile.
		@param filename A file path with a filename.
		@param write Whether to open the file for writing as opposed
			to opening the file for reading.
		@return The file handle. */
	FUFile* OpenFile(const fstring& filename, bool write=true);

	/** Strips the filename from the full file path.
		@param filename The full file path, including the filename.
		@return The file path without the filename.
			This variable is a static variable to correctly support DLLs.
			You should always copy its value within a fstring object. */
	static const fchar* StripFileFromPath(const fstring& filename);

	/** Retrieves the extension of a filename
		@param filename A filename, may include a file path.
		@return The extension of the filename.
			This variable is a static variable to correctly support DLLs.
			You should always copy its value within a fstring object. */
	static const fchar* GetFileExtension(const fstring& filename);

	/** Extracts the network hostname for a URI and returns it.
		@param filename A file URI.
		@return The network hostname.
			This variable is a static variable to correctly support DLLs.
			You should always copy its value within a fstring object. */
	static const fstring& ExtractNetworkHostname(fstring& filename);

	/** Converts a file path absolute.
		@param filePath A file path.
		@return The absolute file path.
			This variable is a static variable to correctly support DLLs.
			You should always copy its value within a fstring object. */
	const fstring& MakeFilePathAbsolute(const fstring& filePath) const;

	/** Converts a file path relative.
		@param filePath A file path.
		@return The file path relative to the current file path.
			This variable is a static variable to correctly support DLLs.
			You should always copy its value within a fstring object. */
	const fstring& MakeFilePathRelative(const fstring& filePath) const;

	/** Converts a file URI into a file path.
		@param fileURL A file URI.
		@return The file path associated with the URI.
			This variable is a static variable to correctly support DLLs.
			You should always copy its value within a fstring object. */
	const fstring& GetFilePath(const fstring& fileURL) const;

	/** Converts a file path into a file URI.
		@param filepath A file path.
		@param relative Whether the returned file URI should be relative to the current file path.
		@return The file URI for the given file path.
			This variable is a static variable to correctly support DLLs.
			You should always copy its value within a fstring object. */
	const fstring& GetFileURL(const fstring& filepath, bool relative) const;

	/** For a relative path, extract the list of the individual
		paths that must be traversed to get to the file.
		@param filename A file path.
		@param hostname The returned hostname of the file.
		@param driveLetter The returned letter of the drive that hosts the file [for Win32].
		@param list The returned list of paths to traverse.
		@param includeFilename Whether the filename should be pushed at the back of the returned list. */
	static void ExtractPathStack(const fstring& filename, fstring& hostname, fchar& driveLetter, FStringList& list, bool includeFilename);

	/** Sets an internal flag that will force all returned file paths
		and URIs to be absolute, rather than relative to the current file path.
		@param _forceAbsolute Whether all file paths should be absolute. */
	void SetForceAbsoluteFlag(bool _forceAbsolute) { forceAbsolute = _forceAbsolute; }

	/** Generates a URI from its components
		@param scheme The access scheme. The empty string is used for relative URIs.
			Examples: file, mailto, https, ftp.
		@param hostname The name of the host for the information.
			The empty string is used for 'localhost'.
		@param filename The full access path and filename for the document.
		@return The newly constructed URI. */
	static const fstring& GenerateURI(const fstring& scheme, const fstring& hostname, const fstring& filename);

	/** Splits a URI into its components
		@param uri The URI to split.
		@param scheme The access scheme. The empty string is used for relative URIs.
			Examples: file, mailto, https, ftp.
		@param hostname The name of the host for the information.
			The empty string is used for 'localhost'.
		@param filename The full access path and filename for the document. */
	static void SplitURI(const fstring& uri, fstring& scheme, fstring& hostname, fstring& filename);
};

#endif // _FU_FILE_MANAGER_H_

