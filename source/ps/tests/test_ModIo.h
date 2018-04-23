/* Copyright (C) 2018 Wildfire Games.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include "lib/self_test.h"

#include "ps/CLogger.h"
#include "ps/ModIo.h"
#include "scriptinterface/ScriptInterface.h"

#include <sodium.h>

class TestModIo : public CxxTest::TestSuite
{
public:
	void setUp()
	{
		if (sodium_init() < 0)
			LOGERROR("failed to initialize libsodium");
	}

	// TODO: One could probably fuzz these parsing functions to
	//       make sure they handle malformed input nicely.

	void test_id_parsing()
	{
		ScriptInterface script("Test", "Test", g_ScriptRuntime);

#define TS_ASSERT_PARSE(input, expected_error, expected_id) \
	{ \
		TestLogger logger; \
		int id = -1; \
		std::string err; \
		TS_ASSERT(!ModIo::ParseGameIdResponse(script, input, id, err)); \
		TS_ASSERT_STR_EQUALS(err, expected_error); \
		TS_ASSERT_EQUALS(id, expected_id); \
	}

		// Various malformed inputs
		TS_ASSERT_PARSE("", "Failed to parse response as JSON.", -1);
		TS_ASSERT_PARSE("()", "Failed to parse response as JSON.", -1);
		TS_ASSERT_PARSE("[]", "data property not an object.", -1);
		TS_ASSERT_PARSE("null", "response not an object.", -1);
		TS_ASSERT_PARSE("{}", "data property not an object.", -1);
		TS_ASSERT_PARSE("{\"data\": null}", "data property not an object.", -1);
		TS_ASSERT_PARSE("{\"data\": {}}", "data property not an array with at least one element.", -1);
		TS_ASSERT_PARSE("{\"data\": []}", "data property not an array with at least one element.", -1);
		TS_ASSERT_PARSE("{\"data\": [null]}", "First element not an object.", -1);
		TS_ASSERT_PARSE("{\"data\": [false]}", "First element not an object.", -1);
		TS_ASSERT_PARSE("{\"data\": [{}]}", "No id property in first element.", -1);
		TS_ASSERT_PARSE("{\"data\": [[]]}", "No id property in first element.", -1);

		// Various invalid IDs
		TS_ASSERT_PARSE("{\"data\": [{\"id\": null}]}", "id property not a number.", -1);
		TS_ASSERT_PARSE("{\"data\": [{\"id\": {}}]}", "id property not a number.", -1);
		TS_ASSERT_PARSE("{\"data\": [{\"id\": true}]}", "id property not a number.", -1);
		TS_ASSERT_PARSE("{\"data\": [{\"id\": -12}]}", "Invalid id.", -1);
		TS_ASSERT_PARSE("{\"data\": [{\"id\": 0}]}", "Invalid id.", -1);

#undef TS_ASSERT_PARSE

		// Correctly formed input
		{
			TestLogger logger;
			int id = -1;
			std::string err;
			TS_ASSERT(ModIo::ParseGameIdResponse(script, "{\"data\": [{\"id\": 42}]}", id, err));
			TS_ASSERT(err.empty());
			TS_ASSERT_EQUALS(id, 42);
		}
	}

	void test_mods_parsing()
	{
		ScriptInterface script("Test", "Test", g_ScriptRuntime);

		PKStruct pk;

		const std::string pk_str = "RWTA6VIoth2Q1PFLsRILr3G7NB+mwwO8BSGoXs63X6TQgNGM4cE8Pvd6";

		size_t bin_len = 0;
		if (sodium_base642bin((unsigned char*)&pk, sizeof pk, pk_str.c_str(), pk_str.size(), NULL, &bin_len, NULL, sodium_base64_VARIANT_ORIGINAL) != 0 || bin_len != sizeof pk)
			LOGERROR("failed to decode base64 public key");

#define TS_ASSERT_PARSE(input, expected_error) \
	{ \
		TestLogger logger; \
		std::vector<ModIoModData> mods; \
		std::string err; \
		TS_ASSERT(!ModIo::ParseModsResponse(script, input, mods, pk, err)); \
		TS_ASSERT_STR_EQUALS(err, expected_error); \
		TS_ASSERT_EQUALS(mods.size(), 0); \
	}

		TS_ASSERT_PARSE("", "Failed to parse response as JSON.");
		TS_ASSERT_PARSE("()", "Failed to parse response as JSON.");
		TS_ASSERT_PARSE("null", "response not an object.");
		TS_ASSERT_PARSE("[]", "data property not an object.");
		TS_ASSERT_PARSE("{}", "data property not an object.");
		TS_ASSERT_PARSE("{\"data\": null}", "data property not an object.");
		TS_ASSERT_PARSE("{\"data\": {}}", "data property not an array with at least one element.");
		TS_ASSERT_PARSE("{\"data\": []}", "data property not an array with at least one element.");
		TS_ASSERT_PARSE("{\"data\": [null]}", "Failed to get array element object.");
		TS_ASSERT_PARSE("{\"data\": [false]}", "Failed to get array element object.");
		TS_ASSERT_PARSE("{\"data\": [true]}", "Failed to get array element object.");
		TS_ASSERT_PARSE("{\"data\": [{}]}", "Failed to get name from el.");
		TS_ASSERT_PARSE("{\"data\": [[]]}", "Failed to get name from el.");
		TS_ASSERT_PARSE("{\"data\": [{\"foo\":\"bar\"}]}", "Failed to get name from el.");

		TS_ASSERT_PARSE("{\"data\": [{\"name\":null}]}", "Failed to get name_id from el."); // also some script value conversion check warning
		TS_ASSERT_PARSE("{\"data\": [{\"name\":42}]}", "Failed to get name_id from el."); // no conversion warning, but converting numbers to strings and vice-versa seems ok
		TS_ASSERT_PARSE("{\"data\": [{\"name\":false}]}", "Failed to get name_id from el."); // also some script value conversion check warning
		TS_ASSERT_PARSE("{\"data\": [{\"name\":{}}]}", "Failed to get name_id from el."); // also some script value conversion check warning
		TS_ASSERT_PARSE("{\"data\": [{\"name\":[]}]}", "Failed to get name_id from el."); // also some script value conversion check warning
		TS_ASSERT_PARSE("{\"data\": [{\"name\":\"foobar\"}]}", "Failed to get name_id from el.");

		TS_ASSERT_PARSE("{\"data\": [{\"name\":\"\",\"name_id\":\"\",\"summary\":\"\"}]}", "modfile not an object.");
		TS_ASSERT_PARSE("{\"data\": [{\"name\":\"\",\"name_id\":\"\",\"summary\":\"\",\"modfile\":null}]}", "modfile not an object.");
		TS_ASSERT_PARSE("{\"data\": [{\"name\":\"\",\"name_id\":\"\",\"summary\":\"\",\"modfile\":[]}]}", "Failed to get version from modFile.");
		TS_ASSERT_PARSE("{\"data\": [{\"name\":\"\",\"name_id\":\"\",\"summary\":\"\",\"modfile\":{}}]}", "Failed to get version from modFile.");

		TS_ASSERT_PARSE("{\"data\": [{\"name\":\"\",\"name_id\":\"\",\"summary\":\"\",\"modfile\":{\"version\":null}}]}", "Failed to get filesize from modFile."); // also some script value conversion check warning
		TS_ASSERT_PARSE("{\"data\": [{\"name\":\"\",\"name_id\":\"\",\"summary\":\"\",\"modfile\":{\"version\":\"\",\"filesize\":1234}}]}", "Failed to get md5 from filehash.");

		TS_ASSERT_PARSE("{\"data\": [{\"name\":\"\",\"name_id\":\"\",\"summary\":\"\",\"modfile\":{\"version\":\"\",\"filesize\":1234, \"filehash\":null}}]}", "Failed to get md5 from filehash.");
		TS_ASSERT_PARSE("{\"data\": [{\"name\":\"\",\"name_id\":\"\",\"summary\":\"\",\"modfile\":{\"version\":\"\",\"filesize\":1234, \"filehash\":{}}}]}", "Failed to get md5 from filehash.");
		TS_ASSERT_PARSE("{\"data\": [{\"name\":\"\",\"name_id\":\"\",\"summary\":\"\",\"modfile\":{\"version\":\"\",\"filesize\":1234, \"filehash\":{\"md5\":null}}}]}", "Failed to get binary_url from download."); // also some script value conversion check warning
		TS_ASSERT_PARSE("{\"data\": [{\"name\":\"\",\"name_id\":\"\",\"summary\":\"\",\"modfile\":{\"version\":\"\",\"filesize\":1234, \"filehash\":{\"md5\":\"abc\"}}}]}", "Failed to get binary_url from download.");

		TS_ASSERT_PARSE("{\"data\": [{\"name\":\"\",\"name_id\":\"\",\"summary\":\"\",\"modfile\":{\"version\":\"\",\"filesize\":1234, \"filehash\":{\"md5\":\"abc\"}, \"download\":null}}]}", "Failed to get binary_url from download."); // also some script value conversion check warning
		TS_ASSERT_PARSE("{\"data\": [{\"name\":\"\",\"name_id\":\"\",\"summary\":\"\",\"modfile\":{\"version\":\"\",\"filesize\":1234, \"filehash\":{\"md5\":\"abc\"}, \"download\":{\"binary_url\":null}}}]}", "Failed to get metadata_blob from modFile."); // also some script value conversion check warning
		TS_ASSERT_PARSE("{\"data\": [{\"name\":\"\",\"name_id\":\"\",\"summary\":\"\",\"modfile\":{\"version\":\"\",\"filesize\":1234, \"filehash\":{\"md5\":\"abc\"}, \"download\":{\"binary_url\":\"\"}}}]}", "Failed to get metadata_blob from modFile.");

		TS_ASSERT_PARSE("{\"data\": [{\"name\":\"\",\"name_id\":\"\",\"summary\":\"\",\"modfile\":{\"version\":\"\",\"filesize\":1234, \"filehash\":{\"md5\":\"abc\"}, \"download\":{\"binary_url\":\"\"},\"metadata_blob\":null}}]}", "metadata_blob not decoded as an object.");
		TS_ASSERT_PARSE("{\"data\": [{\"name\":\"\",\"name_id\":\"\",\"summary\":\"\",\"modfile\":{\"version\":\"\",\"filesize\":1234, \"filehash\":{\"md5\":\"abc\"}, \"download\":{\"binary_url\":\"\"},\"metadata_blob\":\"\"}}]}", "Failed to parse metadata_blob as JSON.");

		TS_ASSERT_PARSE("{\"data\": [{\"name\":\"\",\"name_id\":\"\",\"summary\":\"\",\"modfile\":{\"version\":\"\",\"filesize\":1234, \"filehash\":{\"md5\":\"abc\"}, \"download\":{\"binary_url\":\"\"},\"metadata_blob\":\"{}\"}}]}", "Failed to get dependencies from metadata_blob.");
		TS_ASSERT_PARSE("{\"data\": [{\"name\":\"\",\"name_id\":\"\",\"summary\":\"\",\"modfile\":{\"version\":\"\",\"filesize\":1234, \"filehash\":{\"md5\":\"abc\"}, \"download\":{\"binary_url\":\"\"},\"metadata_blob\":\"{\\\"dependencies\\\":null}\"}}]}", "Failed to get dependencies from metadata_blob.");
		TS_ASSERT_PARSE("{\"data\": [{\"name\":\"\",\"name_id\":\"\",\"summary\":\"\",\"modfile\":{\"version\":\"\",\"filesize\":1234, \"filehash\":{\"md5\":\"abc\"}, \"download\":{\"binary_url\":\"\"},\"metadata_blob\":\"{\\\"dependencies\\\":[]}\"}}]}", "Failed to get minisigs from metadata_blob.");
		TS_ASSERT_PARSE("{\"data\": [{\"name\":\"\",\"name_id\":\"\",\"summary\":\"\",\"modfile\":{\"version\":\"\",\"filesize\":1234, \"filehash\":{\"md5\":\"abc\"}, \"download\":{\"binary_url\":\"\"},\"metadata_blob\":\"{\\\"dependencies\\\":[],\\\"minisigs\\\":null}\"}}]}", "Failed to get minisigs from metadata_blob.");

#undef TS_ASSERT_PARSE

		// Correctly formed input, but no signature matching the public key
		// Thus all such mods/modfiles are not added, thus we get 0 parsed mods.
		{
			TestLogger logger;
			std::vector<ModIoModData> mods;
			std::string err;
			TS_ASSERT(ModIo::ParseModsResponse(script, "{\"data\": [{\"name\":\"\",\"name_id\":\"\",\"summary\":\"\",\"modfile\":{\"version\":\"\",\"filesize\":1234, \"filehash\":{\"md5\":\"abc\"}, \"download\":{\"binary_url\":\"\"},\"metadata_blob\":\"{\\\"dependencies\\\":[],\\\"minisigs\\\":[]}\"}}]}", mods, pk, err));
			TS_ASSERT(err.empty());
			TS_ASSERT_EQUALS(mods.size(), 0);
		}

		// Correctly formed input (with a signature matching the public key above, and a valid global signature)
		{
			TestLogger logger;
			std::vector<ModIoModData> mods;
			std::string err;
			TS_ASSERT(ModIo::ParseModsResponse(script, "{\"data\": [{\"name\":\"\",\"name_id\":\"\",\"summary\":\"\",\"modfile\":{\"version\":\"\",\"filesize\":1234, \"filehash\":{\"md5\":\"abc\"}, \"download\":{\"binary_url\":\"\"},\"metadata_blob\":\"{\\\"dependencies\\\":[],\\\"minisigs\\\":[\\\"untrusted comment: signature from minisign secret key\\\\nRUTA6VIoth2Q1HUg5bwwbCUZPcqbQ/reLXqxiaWARH5PNcwxX5vBv/mLPLgdxGsIrOyK90763+rCVTmjeYx5BDz8C0CIbGZTNQs=\\\\ntrusted comment: timestamp:1517285433\\\\tfile:tm.zip\\\\nTHwNMhK4Ogj6XA4305p1K9/ouP/DrxPcDFrPaiu+Ke6/WGlHIzBZHvmHWUedvsK6dzL31Gk8YNzscKWnZqWNCw==\\\"]}\"}}]}", mods, pk, err));
			TS_ASSERT(err.empty());
			TS_ASSERT_EQUALS(mods.size(), 1);
		}
	}

	void test_signature_parsing()
	{
		PKStruct pk;

		const std::string pk_str = "RWTA6VIoth2Q1PFLsRILr3G7NB+mwwO8BSGoXs63X6TQgNGM4cE8Pvd6";

		size_t bin_len = 0;
		if (sodium_base642bin((unsigned char*)&pk, sizeof pk, pk_str.c_str(), pk_str.size(), NULL, &bin_len, NULL, sodium_base64_VARIANT_ORIGINAL) != 0 || bin_len != sizeof pk)
			LOGERROR("failed to decode base64 public key");


		// No invalid signature at all (silent failure)
#define TS_ASSERT_PARSE_SILENT_FAILURE(input) \
	{ \
		TestLogger logger; \
		SigStruct sig; \
		std::string err; \
		TS_ASSERT(!ModIo::ParseSignature(input, sig, pk, err)); \
		TS_ASSERT(err.empty()); \
	}

#define TS_ASSERT_PARSE(input, expected_error) \
	{ \
		TestLogger logger; \
		SigStruct sig; \
		std::string err; \
		TS_ASSERT(!ModIo::ParseSignature({input}, sig, pk, err)); \
		TS_ASSERT_STR_EQUALS(err, expected_error); \
	}

		TS_ASSERT_PARSE_SILENT_FAILURE({});

		TS_ASSERT_PARSE("", "Invalid (too short) sig.");

		TS_ASSERT_PARSE("\nRUTA6VIoth2Q1HUg5bwwbCUZPcqbQ/reLXqxiaWARH5PNcwxX5vBv/mLPLgdxGsIrOyK90763+rCVTmjeYx5BDz8C0CIbGZTNQs=\n\nTHwNMhK4Ogj6XA4305p1K9/ouP/DrxPcDFrPaiu+Ke6/WGlHIzBZHvmHWUedvsK6dzL31Gk8YNzscKWnZqWNCw==", "Malformed untrusted comment.");
		TS_ASSERT_PARSE("unturusted comment: \nRUTA6VIoth2Q1HUg5bwwbCUZPcqbQ/reLXqxiaWARH5PNcwxX5vBv/mLPLgdxGsIrOyK90763+rCVTmjeYx5BDz8C0CIbGZTNQs=\n\nTHwNMhK4Ogj6XA4305p1K9/ouP/DrxPcDFrPaiu+Ke6/WGlHIzBZHvmHWUedvsK6dzL31Gk8YNzscKWnZqWNCw==", "Malformed untrusted comment.");
		TS_ASSERT_PARSE("untrusted comment: \nRUTA6VIoth2Q1HUg5bwwbCUZPcqbQ/reLXqxiaWARH5PNcwxX5vBv/mLPLgdxGsIrOyK90763+rCVTmjeYx5BDz8C0CIbGZTNQs=\n\nTHwNMhK4Ogj6XA4305p1K9/ouP/DrxPcDFrPaiu+Ke6/WGlHIzBZHvmHWUedvsK6dzL31Gk8YNzscKWnZqWNCw==", "Malformed trusted comment.");
		TS_ASSERT_PARSE("untrusted comment: \nRUTA6VIoth2Q1HUg5bwwbCUZPcqbQ/reLXqxiaWARH5PNcwxX5vBv/mLPLgdxGsIrOyK90763+rCVTmjeYx5BDz8C0CIbGZTNQs=\ntrusted comment:\nTHwNMhK4Ogj6XA4305p1K9/ouP/DrxPcDFrPaiu+Ke6/WGlHIzBZHvmHWUedvsK6dzL31Gk8YNzscKWnZqWNCw==", "Malformed trusted comment.");

		TS_ASSERT_PARSE("untrusted comment: \n\ntrusted comment: \n", "Failed to decode base64 sig.");
		TS_ASSERT_PARSE("untrusted comment: \nZm9vYmFyCg==\ntrusted comment: \n", "Failed to decode base64 sig.");
		TS_ASSERT_PARSE("untrusted comment: \nRWTA6VIoth2Q1HUg5bwwbCUZPcqbQ/reLXqxiaWARH5PNcwxX5vBv/mLPLgdxGsIrOyK90763+rCVTmjeYx5BDz8C0CIbGZTNQs=\ntrusted comment: \n", "Only hashed minisign signatures are supported.");

		// Silent failure again this one has the wrong keynum
		TS_ASSERT_PARSE_SILENT_FAILURE({"untrusted comment: \nRUTA5VIoth2Q1HUg5bwwbCUZPcqbQ/reLXqxiaWARH5PNcwxX5vBv/mLPLgdxGsIrOyK90763+rCVTmjeYx5BDz8C0CIbGZTNQs=\ntrusted comment: \n"});

		TS_ASSERT_PARSE("untrusted comment: \nRUTA6VIoth2Q1HUg5bwwbCUZPcqbQ/reLXqxiaWARH5PNcwxX5vBv/mLPLgdxGsIrOyK90763+rCVTmjeYx5BDz8C0CIbGZTNQs=\ntrusted comment: \n", "Failed to decode base64 global_sig.");

		TS_ASSERT_PARSE("untrusted comment: \nRUTA6VIoth2Q1HUg5bwwbCUZPcqbQ/reLXqxiaWARH5PNcwxX5vBv/mLPLgdxGsIrOyK90763+rCVTmjeYx5BDz8C0CIbGZTNQs=\ntrusted comment: timestamp:1517285433\tfile:tm.zip\nAHwNMhK4Ogj6XA4305p1K9/ouP/DrxPcDFrPaiu+Ke6/WGlHIzBZHvmHWUedvsK6dzL31Gk8YNzscKWnZqWNCw==", "Failed to verify global signature.");

		// Valid signature
		{
			TestLogger logger;
			SigStruct sig;
			std::string err;
			TS_ASSERT(ModIo::ParseSignature({"untrusted comment: \nRUTA6VIoth2Q1HUg5bwwbCUZPcqbQ/reLXqxiaWARH5PNcwxX5vBv/mLPLgdxGsIrOyK90763+rCVTmjeYx5BDz8C0CIbGZTNQs=\ntrusted comment: timestamp:1517285433\tfile:tm.zip\nTHwNMhK4Ogj6XA4305p1K9/ouP/DrxPcDFrPaiu+Ke6/WGlHIzBZHvmHWUedvsK6dzL31Gk8YNzscKWnZqWNCw=="}, sig, pk, err));
			TS_ASSERT(err.empty());
		}

#undef TS_ASSERT_PARSE_SILENT_FAILURE
#undef TS_ASSERT_PARSE
	}
};
