#include "lib/self_test.h"

#include "lib/res/file/path.h"
#include "lib/res/file/file.h"
#include "lib/res/file/file_cache.h"
#include "lib/res/file/vfs.h"
#include "lib/res/file/archive.h"
#include "lib/res/file/archive_builder.h"

class TestArchiveBuilder : public CxxTest::TestSuite 
{
	const char* const archive_fn;
	static const size_t NUM_FILES = 100;
	static const size_t MAX_FILE_SIZE = 20000;

	std::set<const char*> existing_names;
	const char* gen_random_name()
	{
		// 10 chars is enough for (10-1)*5 bits = 45 bits > u32
		char name_tmp[10];

		for(;;)
		{
			u32 rand_num = rand(0, 100000);
			base32(4, (const u8*)&rand_num, (u8*)name_tmp);

			// store filename in atom pool
			const char* atom_fn = file_make_unique_fn_copy(name_tmp);
			// done if the filename is unique (not been generated yet)
			if(existing_names.find(atom_fn) == existing_names.end())
			{
				existing_names.insert(atom_fn);
				return atom_fn;
			}
		}
	}

	struct TestFile
	{
		off_t size;
		u8* data;	// must be delete[]-ed after comparing
	};
	// (must be separate array and end with NULL entry (see Filenames))
	const char* filenames[NUM_FILES+1];
	TestFile files[NUM_FILES];

	void generate_random_files()
	{
		for(size_t i = 0; i < NUM_FILES; i++)
		{
			const off_t size = rand(0, MAX_FILE_SIZE);
			u8* data = new u8[size];

			// random data won't compress at all, and we want to exercise
			// the uncompressed codepath as well => make some of the files
			// easily compressible (much less values).
			const bool make_easily_compressible = (rand(0, 100) > 50);
			if(make_easily_compressible)
			{
				for(off_t i = 0; i < size; i++)
					data[i] = rand() & 0x0F;
			}
			else
			{
				for(off_t i = 0; i < size; i++)
					data[i] = rand() & 0xFF;
			}

			filenames[i] = gen_random_name();
			files[i].size = size;
			files[i].data = data;

			ssize_t bytes_written = vfs_store(filenames[i], data, size, FILE_NO_AIO);
			TS_ASSERT_EQUALS(bytes_written, size);
		}

		// 0-terminate the list - see Filenames decl.
		filenames[NUM_FILES] = NULL;
	}

public:
	TestArchiveBuilder()
		: archive_fn("test_archive_random_data.zip") {}

	void setUp()
	{
		(void)file_init();
		path_init();	// required for file_make_unique_fn_copy
		(void)file_set_root_dir(0, ".");
		vfs_init();
		TS_ASSERT_OK(dir_create("archivetest", S_IRWXU|S_IRWXG|S_IRWXO));
		TS_ASSERT_OK(vfs_mount("", "archivetest"));
	}

	void tearDown()
	{
		vfs_shutdown();
		dir_delete("archivetest");
	}

	void test_create_archive_with_random_files()
	{
		generate_random_files();
		TS_ASSERT_OK(archive_build(archive_fn, filenames));

		// wipe out file cache, otherwise we're just going to get back
		// the file contents read during archive_build .
		file_cache_reset();

		// read in each file and compare file contents
		Handle ha = archive_open(archive_fn);
		TS_ASSERT(ha > 0);
		for(size_t i = 0; i < NUM_FILES; i++)
		{
			File f;
			TS_ASSERT_OK(afile_open(ha, filenames[i], 0, 0, &f));
			FileIOBuf buf = FILE_BUF_ALLOC;
			ssize_t bytes_read = afile_read(&f, 0, files[i].size, &buf);
			TS_ASSERT_EQUALS(bytes_read, files[i].size);

			TS_ASSERT_SAME_DATA(buf, files[i].data, files[i].size);

			TS_ASSERT_OK(file_buf_free(buf));
			SAFE_ARRAY_DELETE(files[i].data);
		}
		TS_ASSERT_OK(archive_close(ha));
	}
};
