#ifndef INCLUDED_VFS_PATH
#define INCLUDED_VFS_PATH

struct VfsPathTraits;
typedef fs::basic_path<std::string, VfsPathTraits> VfsPath;

typedef std::vector<VfsPath> VfsPaths;

struct VfsPathTraits
{
	typedef std::string internal_string_type;
	typedef std::string external_string_type;

	static external_string_type to_external(const VfsPath&, const internal_string_type& src)
	{
		return src;
	}

	static internal_string_type to_internal(const external_string_type& src)
	{
		return src;
	}
};

extern bool IsDirectory(const VfsPath& pathname);
extern std::string Basename(const VfsPath& pathname);
extern std::string Extension(const VfsPath& pathname);

#endif	//	#ifndef INCLUDED_VFS_PATH
