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

#ifndef INCLUDED_MODIO
#define INCLUDED_MODIO

#include "lib/external_libraries/curl.h"
#include "scriptinterface/ScriptInterface.h"

#include <sodium.h>
#include <string>

// TODO: Allocate instance of the below two using sodium_malloc?
struct PKStruct
{
	unsigned char sig_alg[2] = {}; // == "Ed"
	unsigned char keynum[8] = {}; // should match the keynum in the sigstruct, else this is the wrong key
	unsigned char pk[crypto_sign_PUBLICKEYBYTES] = {};
};

struct SigStruct
{
	unsigned char sig_alg[2] = {}; // "ED" (since we only support the hashed mode)
	unsigned char keynum[8] = {}; // should match the keynum in the PKStruct
	unsigned char sig[crypto_sign_BYTES] = {};
};

struct ModIoModData
{
	std::map<std::string, std::string> properties;
	std::vector<std::string> dependencies;
	SigStruct sig;
};

enum class DownloadProgressStatus {
	NONE, // Default state
	GAMEID, // The game ID is being downloaded
	READY, // The game ID has been downloaded
	LISTING, // The mod list is being downloaded
	LISTED, // The mod list has been downloaded
	DOWNLOADING, // A mod file is being downloaded
	SUCCESS, // A mod file has been downloaded

	FAILED_GAMEID, // Game ID couldn't be retrieved
	FAILED_LISTING, // Mod list couldn't be retrieved
	FAILED_DOWNLOADING, // File couldn't be retrieved
	FAILED_FILECHECK // The file is corrupted
};

struct DownloadProgressData
{
	DownloadProgressStatus status;
	double progress;
	std::string error;
};

struct DownloadCallbackData;

/**
 * mod.io API interfacing code.
 *
 * Overview
 *
 * This class interfaces with a remote API provider that returns a list of mod files.
 * These can then be downloaded after some cursory checking of well-formedness of the returned
 * metadata.
 * Downloaded files are checked for well formedness by validating that they fit the size and hash
 * indicated by the API, then we check if the file is actually signed by a trusted key, and only
 * if all of that is success the file is actually possible to be loaded as a mod.
 *
 * Security considerations
 *
 * This both distrusts the loaded JS mods, and the API as much as possible.
 * We do not want a malicious mod to use this to download arbitrary files, nor do we want the API
 * to make us download something we have not verified.
 * Therefore we only allow mods to download one of the mods returned by this class (using indices).
 *
 * This (mostly) necessitates parsing the API responses here, as opposed to in JS.
 * One could alternatively parse the responses in a locked down JS context, but that would require
 * storing that code in here, or making sure nobody can overwrite it. Also this would possibly make
 * some of the needed accesses for downloading and verifying files a bit more complicated.
 *
 * Everything downloaded from the API has its signature verified against our public key.
 * This is a requirement, as otherwise a compromise of the API would result in users installing
 * possibly malicious files.
 * So a compromised API can just serve old files that we signed, so in that case there would need
 * to be an issue in that old file that was missed.
 *
 * To limit the extend to how old those files could be the signing key should be rotated
 * regularly (e.g. every release). To allow old versions of the engine to still use the API
 * files can be signed by both the old and the new key for some amount of time, that however
 * only makes sense in case a mod is compatible with both engine versions.
 *
 * Note that this does not prevent all possible attacks a package manager/update system should
 * defend against. This is intentionally not an update system since proper package managers already
 * exist. However there is some possible overlap in attack vectors and these should be evalutated
 * whether they apply and to what extend we can fix that on our side (or how to get the API provider
 * to help us do so). For a list of some possible issues see:
 * https://github.com/theupdateframework/specification/blob/master/tuf-spec.md
 *
 * The mod.io settings are also locked down such that only mods that have been authorized by us
 * show up in API queries. This is both done so that all required information (dependencies)
 * are stored for the files, and that only mods that have been checked for being ok are actually
 * shown to users.
 */
class ModIo
{
	NONCOPYABLE(ModIo);
public:
	ModIo();
	~ModIo();

	// Async requests
	void StartGetGameId();
	void StartListMods();
	void StartDownloadMod(size_t idx);

	/**
	 * Advance the current async request and perform final steps if the download is complete.
	 *
	 * @param scriptInterface used for parsing the data and possibly install the mod.
	 * @return true if the download is complete (successful or not), false otherwise.
	 */
	bool AdvanceRequest(const ScriptInterface& scriptInterface);

	/**
	 * Cancel the current async request and clean things up
	 */
	void CancelRequest();

	const std::vector<ModIoModData>& GetMods() const
	{
		return m_ModData;
	}
	const DownloadProgressData& GetDownloadProgress() const
	{
		return m_DownloadProgressData;
	}

private:
	static size_t ReceiveCallback(void* buffer, size_t size, size_t nmemb, void* userp);
	static size_t DownloadCallback(void* buffer, size_t size, size_t nmemb, void* userp);
	static int DownloadProgressCallback(void* clientp, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow);

	CURLMcode SetupRequest(const std::string& url, bool fileDownload);
	void TearDownRequest();

	bool ParseGameId(const ScriptInterface& scriptInterface, std::string& err);
	bool ParseMods(const ScriptInterface& scriptInterface, std::string& err);

	void DeleteDownloadedFile();
	bool VerifyDownloadedFile(std::string& err);

	// Utility methods for parsing mod.io responses and metadata
	static bool ParseGameIdResponse(const ScriptInterface& scriptInterface, const std::string& responseData, int& id, std::string& err);
	static bool ParseModsResponse(const ScriptInterface& scriptInterface, const std::string& responseData, std::vector<ModIoModData>& modData, const PKStruct& pk, std::string& err);
	static bool ParseSignature(const std::vector<std::string>& minisigs, SigStruct& sig, const PKStruct& pk, std::string& err);

	// Url parts
	std::string m_BaseUrl;
	std::string m_GamesRequest;
	std::string m_GameId;

	// Query parameters
	std::string m_ApiKey;
	std::string m_IdQuery;

	CURL* m_Curl;
	CURLM* m_CurlMulti;
	curl_slist* m_Headers;
	char m_ErrorBuffer[CURL_ERROR_SIZE];
	std::string m_ResponseData;

	// Current mod download
	int m_DownloadModID;
	OsPath m_DownloadFilePath;
	DownloadCallbackData* m_CallbackData;
	DownloadProgressData m_DownloadProgressData;

	PKStruct m_pk;

	std::vector<ModIoModData> m_ModData;

	friend class TestModIo;
};

extern ModIo* g_ModIo;

#endif // INCLUDED_MODIO
