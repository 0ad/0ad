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

#include "precompiled.h"

#include "ModIo.h"

#include "i18n/L10n.h"
#include "lib/file/file_system.h"
#include "lib/sysdep/filesystem.h"
#include "lib/sysdep/sysdep.h"
#include "maths/MD5.h"
#include "ps/CLogger.h"
#include "ps/ConfigDB.h"
#include "ps/GameSetup/Paths.h"
#include "ps/Mod.h"
#include "ps/ModInstaller.h"
#include "ps/Util.h"
#include "scriptinterface/ScriptConversions.h"
#include "scriptinterface/ScriptInterface.h"

#include <boost/algorithm/string.hpp>
#include <iomanip>

ModIo* g_ModIo = nullptr;

struct DownloadCallbackData
{
	DownloadCallbackData()
		: fp(nullptr), md5()
	{
	}
	DownloadCallbackData(FILE* _fp)
		: fp(_fp), md5()
	{
		crypto_generichash_init(&hash_state, NULL, 0U, crypto_generichash_BYTES_MAX);
	}
	FILE* fp;
	MD5 md5;
	crypto_generichash_state hash_state;
};

ModIo::ModIo()
	: m_GamesRequest("/games"), m_CallbackData(nullptr)
{
	// Get config values from the sytem namespace, or below (default).
	// This can be overridden on the command line.
	//
	// We do this so a malicious mod cannot change the base url and
	// get the user to make connections to someone else's endpoint.
	// If another user of the engine wants to provide different values
	// here, while still using the same engine version, they can just
	// provide some shortcut/script that sets these using command line
	// parameters.
	std::string pk_str;
	g_ConfigDB.GetValue(CFG_SYSTEM, "modio.public_key", pk_str);
	g_ConfigDB.GetValue(CFG_SYSTEM, "modio.v1.baseurl", m_BaseUrl);
	{
		std::string api_key;
		g_ConfigDB.GetValue(CFG_SYSTEM, "modio.v1.api_key", api_key);
		m_ApiKey = "api_key=" + api_key;
	}
	{
		std::string nameid;
		g_ConfigDB.GetValue(CFG_SYSTEM, "modio.v1.name_id", nameid);
		m_IdQuery = "name_id="+nameid;
	}

	m_CurlMulti = curl_multi_init();
	ENSURE(m_CurlMulti);

	m_Curl = curl_easy_init();
	ENSURE(m_Curl);

	// Capture error messages
	curl_easy_setopt(m_Curl, CURLOPT_ERRORBUFFER, m_ErrorBuffer);

	// Fail if the server did
	curl_easy_setopt(m_Curl, CURLOPT_FAILONERROR, 1L);

	// Disable signal handlers (required for multithreaded applications)
	curl_easy_setopt(m_Curl, CURLOPT_NOSIGNAL, 1L);

	// To minimise security risks, don't support redirects (except for file
	// downloads, for which this setting will be enabled).
	curl_easy_setopt(m_Curl, CURLOPT_FOLLOWLOCATION, 0L);

	// For file downloads, one redirect seems plenty for a CDN serving the files.
	curl_easy_setopt(m_Curl, CURLOPT_MAXREDIRS, 1L);

	m_Headers = NULL;
	std::string ua = "User-Agent: pyrogenesis ";
	ua += curl_version();
	ua += " (https://play0ad.com/)";
	m_Headers = curl_slist_append(m_Headers, ua.c_str());
	curl_easy_setopt(m_Curl, CURLOPT_HTTPHEADER, m_Headers);

	if (sodium_init() < 0)
		ENSURE(0 && "Failed to initialize libsodium.");

	size_t bin_len = 0;
	if (sodium_base642bin((unsigned char*)&m_pk, sizeof m_pk, pk_str.c_str(), pk_str.size(), NULL, &bin_len, NULL, sodium_base64_VARIANT_ORIGINAL) != 0 || bin_len != sizeof m_pk)
		ENSURE(0 && "Failed to decode base64 public key. Please fix your configuration or mod.io will be unusable.");
}

ModIo::~ModIo()
{
	// Clean things up to avoid unpleasant surprises,
	// and delete the temporary file if any.
	TearDownRequest();
	if (m_DownloadProgressData.status == DownloadProgressStatus::DOWNLOADING)
		DeleteDownloadedFile();

	curl_slist_free_all(m_Headers);
	curl_easy_cleanup(m_Curl);
	curl_multi_cleanup(m_CurlMulti);

	delete m_CallbackData;
}

size_t ModIo::ReceiveCallback(void* buffer, size_t size, size_t nmemb, void* userp)
{
	ModIo* self = static_cast<ModIo*>(userp);

	self->m_ResponseData += std::string((char*)buffer, (char*)buffer+size*nmemb);

	return size*nmemb;
}

size_t ModIo::DownloadCallback(void* buffer, size_t size, size_t nmemb, void* userp)
{
	DownloadCallbackData* data = static_cast<DownloadCallbackData*>(userp);
	if (!data->fp)
		return 0;

	size_t len = fwrite(buffer, size, nmemb, data->fp);

	// Only update the hash with data we actually managed to write.
	// In case we did not write all of it we will fail the download,
	// but we do not want to have a possibly valid hash in that case.
	size_t written = len*size;

	data->md5.Update((const u8*)buffer, written);
	crypto_generichash_update(&data->hash_state, (const u8*)buffer, written);

	return written;
}

int ModIo::DownloadProgressCallback(void* clientp, curl_off_t dltotal, curl_off_t dlnow, curl_off_t UNUSED(ultotal), curl_off_t UNUSED(ulnow))
{
	DownloadProgressData* data = static_cast<DownloadProgressData*>(clientp);

	// If we got more data than curl expected, something is very wrong, abort.
	if (dltotal != 0 && dlnow > dltotal)
		return 1;

	data->progress = dltotal == 0 ? 0 : static_cast<double>(dlnow) / static_cast<double>(dltotal);

	return 0;
}

CURLMcode ModIo::SetupRequest(const std::string& url, bool fileDownload)
{
	if (fileDownload)
	{
		// The download link will most likely redirect elsewhere, so allow that.
		// We verify the validity of the file later.
		curl_easy_setopt(m_Curl, CURLOPT_FOLLOWLOCATION, 1L);
		// Enable the progress meter
		curl_easy_setopt(m_Curl, CURLOPT_NOPROGRESS, 0L);

		// Set IO callbacks
		curl_easy_setopt(m_Curl, CURLOPT_WRITEFUNCTION, DownloadCallback);
		curl_easy_setopt(m_Curl, CURLOPT_WRITEDATA, (void*)m_CallbackData);
		curl_easy_setopt(m_Curl, CURLOPT_XFERINFOFUNCTION, DownloadProgressCallback);
		curl_easy_setopt(m_Curl, CURLOPT_XFERINFODATA, (void*)&m_DownloadProgressData);

		// Initialize the progress counter
		m_DownloadProgressData.progress = 0;
	}
	else
	{
		// To minimise security risks, don't support redirects
		curl_easy_setopt(m_Curl, CURLOPT_FOLLOWLOCATION, 0L);
		// Disable the progress meter
		curl_easy_setopt(m_Curl, CURLOPT_NOPROGRESS, 1L);

		// Set IO callbacks
		curl_easy_setopt(m_Curl, CURLOPT_WRITEFUNCTION, ReceiveCallback);
		curl_easy_setopt(m_Curl, CURLOPT_WRITEDATA, this);
	}

	m_ErrorBuffer[0] = '\0';
	curl_easy_setopt(m_Curl, CURLOPT_URL, url.c_str());
	return curl_multi_add_handle(m_CurlMulti, m_Curl);
}

void ModIo::TearDownRequest()
{
	ENSURE(curl_multi_remove_handle(m_CurlMulti, m_Curl) == CURLM_OK);

	if (m_CallbackData)
	{
		if (m_CallbackData->fp)
			fclose(m_CallbackData->fp);
		m_CallbackData->fp = nullptr;
	}
}

void ModIo::StartGetGameId()
{
	// Don't start such a request during active downloads.
	if (m_DownloadProgressData.status == DownloadProgressStatus::GAMEID ||
	    m_DownloadProgressData.status == DownloadProgressStatus::LISTING ||
	    m_DownloadProgressData.status == DownloadProgressStatus::DOWNLOADING)
		return;

	m_GameId.clear();

	CURLMcode err = SetupRequest(m_BaseUrl+m_GamesRequest+"?"+m_ApiKey+"&"+m_IdQuery, false);
	if (err != CURLM_OK)
	{
		TearDownRequest();
		m_DownloadProgressData.status = DownloadProgressStatus::FAILED_GAMEID;
		m_DownloadProgressData.error = fmt::sprintf(
			g_L10n.Translate("Failure while starting querying for game id. Error: %s; %s."),
			curl_multi_strerror(err), m_ErrorBuffer);
		return;
	}

	m_DownloadProgressData.status = DownloadProgressStatus::GAMEID;
}

void ModIo::StartListMods()
{
	// Don't start such a request during active downloads.
	if (m_DownloadProgressData.status == DownloadProgressStatus::GAMEID ||
	    m_DownloadProgressData.status == DownloadProgressStatus::LISTING ||
	    m_DownloadProgressData.status == DownloadProgressStatus::DOWNLOADING)
		return;

	m_ModData.clear();

	if (m_GameId.empty())
	{
		LOGERROR("Game ID not fetched from mod.io. Call StartGetGameId first and wait for it to finish.");
		return;
	}

	CURLMcode err = SetupRequest(m_BaseUrl+m_GamesRequest+m_GameId+"/mods?"+m_ApiKey, false);
	if (err != CURLM_OK)
	{
		TearDownRequest();
		m_DownloadProgressData.status = DownloadProgressStatus::FAILED_LISTING;
		m_DownloadProgressData.error = fmt::sprintf(
			g_L10n.Translate("Failure while starting querying for mods. Error: %s; %s."),
			curl_multi_strerror(err), m_ErrorBuffer);
		return;
	}

	m_DownloadProgressData.status = DownloadProgressStatus::LISTING;
}

void ModIo::StartDownloadMod(size_t idx)
{
	// Don't start such a request during active downloads.
	if (m_DownloadProgressData.status == DownloadProgressStatus::GAMEID ||
	    m_DownloadProgressData.status == DownloadProgressStatus::LISTING ||
	    m_DownloadProgressData.status == DownloadProgressStatus::DOWNLOADING)
		return;

	if (idx >= m_ModData.size())
		return;

	const Paths paths(g_args);
	const OsPath modUserPath = paths.UserData()/"mods";
	const OsPath modPath = modUserPath/m_ModData[idx].properties["name_id"];
	if (!DirectoryExists(modPath) && INFO::OK != CreateDirectories(modPath, 0700, false))
	{
		m_DownloadProgressData.status = DownloadProgressStatus::FAILED_DOWNLOADING;
		m_DownloadProgressData.error = fmt::sprintf(
			g_L10n.Translate("Could not create mod directory: %s."), modPath.string8());
		return;
	}

	// Name the file after the name_id, since using the filename would mean that
	// we could end up with multiple zip files in the folder that might not work
	// as expected for a user (since a later version might remove some files
	// that aren't compatible anymore with the engine version).
	// So we ignore the filename provided by the API and assume that we do not
	// care about handling update.zip files. If that is the case we would need
	// a way to find out what files are required by the current one and which
	// should be removed for everything to work. This seems to be too complicated
	// so we just do not support that usage.
	// NOTE: We do save the file under a slightly different name from the final
	//       one, to ensure that in case a download aborts and the file stays
	//       around, the game will not attempt to open the file which has not
	//       been verified.
	m_DownloadFilePath = modPath/(m_ModData[idx].properties["name_id"]+".zip.temp");

	delete m_CallbackData;
	m_CallbackData = new DownloadCallbackData(sys_OpenFile(m_DownloadFilePath, "wb"));
	if (!m_CallbackData->fp)
	{
		m_DownloadProgressData.status = DownloadProgressStatus::FAILED_DOWNLOADING;
		m_DownloadProgressData.error = fmt::sprintf(
			g_L10n.Translate("Could not open temporary file for mod download: %s."), m_DownloadFilePath.string8());
		return;
	}

	CURLMcode err = SetupRequest(m_ModData[idx].properties["binary_url"], true);
	if (err != CURLM_OK)
	{
		TearDownRequest();
		m_DownloadProgressData.status = DownloadProgressStatus::FAILED_DOWNLOADING;
		m_DownloadProgressData.error = fmt::sprintf(
			g_L10n.Translate("Failed to start the download. Error: %s; %s."), curl_multi_strerror(err), m_ErrorBuffer);
		return;
	}

	m_DownloadModID = idx;
	m_DownloadProgressData.status = DownloadProgressStatus::DOWNLOADING;
}

void ModIo::CancelRequest()
{
	TearDownRequest();

	switch (m_DownloadProgressData.status)
	{
	case DownloadProgressStatus::GAMEID:
	case DownloadProgressStatus::FAILED_GAMEID:
		m_DownloadProgressData.status = DownloadProgressStatus::NONE;
		break;
	case DownloadProgressStatus::LISTING:
	case DownloadProgressStatus::FAILED_LISTING:
		m_DownloadProgressData.status = DownloadProgressStatus::READY;
		break;
	case DownloadProgressStatus::DOWNLOADING:
	case DownloadProgressStatus::FAILED_DOWNLOADING:
		m_DownloadProgressData.status = DownloadProgressStatus::LISTED;
		DeleteDownloadedFile();
		break;
	default:
		break;
	}
}

bool ModIo::AdvanceRequest(const ScriptInterface& scriptInterface)
{
	// If the request was cancelled, stop trying to advance it
	if (m_DownloadProgressData.status != DownloadProgressStatus::GAMEID &&
	    m_DownloadProgressData.status != DownloadProgressStatus::LISTING &&
	    m_DownloadProgressData.status != DownloadProgressStatus::DOWNLOADING)
		return true;

	int stillRunning;
	CURLMcode err = curl_multi_perform(m_CurlMulti, &stillRunning);
	if (err != CURLM_OK)
	{
		std::string error = fmt::sprintf(
			g_L10n.Translate("Asynchronous download failure: %s, %s."), curl_multi_strerror(err), m_ErrorBuffer);
		TearDownRequest();
		if (m_DownloadProgressData.status == DownloadProgressStatus::GAMEID)
			m_DownloadProgressData.status = DownloadProgressStatus::FAILED_GAMEID;
		else if (m_DownloadProgressData.status == DownloadProgressStatus::LISTING)
			m_DownloadProgressData.status = DownloadProgressStatus::FAILED_LISTING;
		else if (m_DownloadProgressData.status == DownloadProgressStatus::DOWNLOADING)
		{
			m_DownloadProgressData.status = DownloadProgressStatus::FAILED_DOWNLOADING;
			DeleteDownloadedFile();
		}
		m_DownloadProgressData.error = error;
		return true;
	}

	CURLMsg* message;
	do
	{
		int in_queue;
		message = curl_multi_info_read(m_CurlMulti, &in_queue);
		if (!message || message->msg == CURLMSG_DONE || message->easy_handle == m_Curl)
			continue;

		CURLcode err = message->data.result;
		if (err == CURLE_OK)
			continue;

		std::string error = fmt::sprintf(
			g_L10n.Translate("Download failure. Server response: %s; %s."), curl_easy_strerror(err), m_ErrorBuffer);
		TearDownRequest();
		if (m_DownloadProgressData.status == DownloadProgressStatus::GAMEID)
			m_DownloadProgressData.status = DownloadProgressStatus::FAILED_GAMEID;
		else if (m_DownloadProgressData.status == DownloadProgressStatus::LISTING)
			m_DownloadProgressData.status = DownloadProgressStatus::FAILED_LISTING;
		else if (m_DownloadProgressData.status == DownloadProgressStatus::DOWNLOADING)
		{
			m_DownloadProgressData.status = DownloadProgressStatus::FAILED_DOWNLOADING;
			DeleteDownloadedFile();
		}
		m_DownloadProgressData.error = error;
		return true;
	} while (message);

	if (stillRunning)
		return false;

	// Download finished.
	TearDownRequest();

	// Perform parsing and/or checks
	std::string error;
	switch (m_DownloadProgressData.status)
	{
	case DownloadProgressStatus::GAMEID:
		if (!ParseGameId(scriptInterface, error))
		{
			m_DownloadProgressData.status = DownloadProgressStatus::FAILED_GAMEID;
			m_DownloadProgressData.error = error;
			break;
		}

		m_DownloadProgressData.status = DownloadProgressStatus::READY;
		break;
	case DownloadProgressStatus::LISTING:
		if (!ParseMods(scriptInterface, error))
		{
			m_ModData.clear(); // Failed during parsing, make sure we don't provide partial data
			m_DownloadProgressData.status = DownloadProgressStatus::FAILED_LISTING;
			m_DownloadProgressData.error = error;
			break;
		}

		m_DownloadProgressData.status = DownloadProgressStatus::LISTED;
		break;
	case DownloadProgressStatus::DOWNLOADING:
		if (!VerifyDownloadedFile(error))
		{
			m_DownloadProgressData.status = DownloadProgressStatus::FAILED_FILECHECK;
			m_DownloadProgressData.error = error;
			DeleteDownloadedFile();
			break;
		}

		m_DownloadProgressData.status = DownloadProgressStatus::SUCCESS;

		{
			Paths paths(g_args);
			CModInstaller installer(paths.UserData() / "mods", paths.Cache());
			installer.Install(m_DownloadFilePath, g_ScriptRuntime, false);
		}
		break;
	default:
		break;
	}

	return true;
}

bool ModIo::ParseGameId(const ScriptInterface& scriptInterface, std::string& err)
{
	int id = -1;
	bool ret = ParseGameIdResponse(scriptInterface, m_ResponseData, id, err);
	m_ResponseData.clear();
	if (!ret)
		return false;

	m_GameId = "/" + std::to_string(id);
	return true;
}

bool ModIo::ParseMods(const ScriptInterface& scriptInterface, std::string& err)
{
	bool ret = ParseModsResponse(scriptInterface, m_ResponseData, m_ModData, m_pk, err);
	m_ResponseData.clear();
	return ret;
}

void ModIo::DeleteDownloadedFile()
{
	if (wunlink(m_DownloadFilePath) != 0)
		LOGERROR("Failed to delete temporary file.");
	m_DownloadFilePath = OsPath();
}

bool ModIo::VerifyDownloadedFile(std::string& err)
{
	// Verify filesize, as a first basic download check.
	{
		u64 filesize = std::stoull(m_ModData[m_DownloadModID].properties.at("filesize"));
		if (filesize != FileSize(m_DownloadFilePath))
		{
			err = g_L10n.Translate("Mismatched filesize.");
			return false;
		}
	}

	ENSURE(m_CallbackData);

	// MD5 (because upstream provides it)
	// Just used to make sure there was no obvious corruption during transfer.
	{
		u8 digest[MD5::DIGESTSIZE];
		m_CallbackData->md5.Final(digest);
		std::string md5digest = Hexify(digest, MD5::DIGESTSIZE);


		if (m_ModData[m_DownloadModID].properties.at("filehash_md5") != md5digest)
		{
			err = fmt::sprintf(
				g_L10n.Translate("Invalid file. Expected md5 %s, got %s."),
				m_ModData[m_DownloadModID].properties.at("filehash_md5").c_str(),
				md5digest);
			return false;
		}
	}

	// Verify file signature.
	// Used to make sure that the downloaded file was actually checked and signed
	// by Wildfire Games. And has not been tampered with by the API provider, or the CDN.

	unsigned char hash_fin[crypto_generichash_BYTES_MAX] = {};
	if (crypto_generichash_final(&m_CallbackData->hash_state, hash_fin, sizeof hash_fin) != 0)
	{
		err = g_L10n.Translate("Failed to compute final hash.");
		return false;
	}

	if (crypto_sign_verify_detached(m_ModData[m_DownloadModID].sig.sig, hash_fin, sizeof hash_fin, m_pk.pk) != 0)
	{
		err = g_L10n.Translate("Failed to verify signature.");
		return false;
	}

	return true;
}

#define FAIL(...) STMT(err = fmt::sprintf(__VA_ARGS__); CLEANUP(); return false;)

/**
 * Parses the current content of m_ResponseData to extract m_GameId.
 *
 * The JSON data is expected to look like
 * { "data": [{"id": 42, ...}, ...], ... }
 * where we are only interested in the value of the id property.
 *
 * @returns true iff it successfully parsed the id.
 */
bool ModIo::ParseGameIdResponse(const ScriptInterface& scriptInterface, const std::string& responseData, int& id, std::string& err)
{
#define CLEANUP() id = -1;
	JSContext* cx = scriptInterface.GetContext();
	JSAutoRequest rq(cx);

	JS::RootedValue gameResponse(cx);

	if (!scriptInterface.ParseJSON(responseData, &gameResponse))
		FAIL("Failed to parse response as JSON.");

	if (!gameResponse.isObject())
		FAIL("response not an object.");

	JS::RootedObject gameResponseObj(cx, gameResponse.toObjectOrNull());
	JS::RootedValue dataVal(cx);
	if (!JS_GetProperty(cx, gameResponseObj, "data", &dataVal))
		FAIL("data property not in response.");

	// [{"id": 42, ...}, ...]
	if (!dataVal.isObject())
		FAIL("data property not an object.");

	JS::RootedObject data(cx, dataVal.toObjectOrNull());
	u32 length;
	if (!JS_IsArrayObject(cx, data) || !JS_GetArrayLength(cx, data, &length) || !length)
		FAIL("data property not an array with at least one element.");

	// {"id": 42, ...}
	JS::RootedValue first(cx);
	if (!JS_GetElement(cx, data, 0, &first))
		FAIL("Couldn't get first element.");
	if (!first.isObject())
		FAIL("First element not an object.");

	JS::RootedObject firstObj(cx, &first.toObject());
	bool hasIdProperty;
	if (!JS_HasProperty(cx, firstObj, "id", &hasIdProperty) || !hasIdProperty)
		FAIL("No id property in first element.");

	JS::RootedValue idProperty(cx);
	ENSURE(JS_GetProperty(cx, firstObj, "id", &idProperty));

	// Make sure the property is not set to something that could be converted to a bogus value
	// TODO: We should be able to convert JS::Values to C++ variables in a way that actually
	// fails when types do not match (see https://trac.wildfiregames.com/ticket/5128).
	if (!idProperty.isNumber())
		FAIL("id property not a number.");

	id = -1;
	if (!ScriptInterface::FromJSVal(cx, idProperty, id) || id <= 0)
		FAIL("Invalid id.");

	return true;
#undef CLEANUP
}

/**
 * Parses the current content of m_ResponseData into m_ModData.
 *
 * The JSON data is expected to look like
 * { data: [modobj1, modobj2, ...], ... (including result_count) }
 * where modobjN has the following structure
 * { homepage_url: "url", name: "displayname", nameid: "short-non-whitespace-name",
 *   summary: "short desc.", modfile: { version: "1.2.4", filename: "asdf.zip",
 *   filehash: { md5: "deadbeef" }, filesize: 1234, download: { binary_url: "someurl", ... } }, ... }.
 * Only the listed properties are of interest to consumers, and we flatten
 * the modfile structure as that simplifies handling and there are no conflicts.
 */
bool ModIo::ParseModsResponse(const ScriptInterface& scriptInterface, const std::string& responseData, std::vector<ModIoModData>& modData, const PKStruct& pk, std::string& err)
{
// Make sure we don't end up passing partial results back
#define CLEANUP() modData.clear();

	JSContext* cx = scriptInterface.GetContext();
	JSAutoRequest rq(cx);

	JS::RootedValue modResponse(cx);

	if (!scriptInterface.ParseJSON(responseData, &modResponse))
		FAIL("Failed to parse response as JSON.");

	if (!modResponse.isObject())
		FAIL("response not an object.");

	JS::RootedObject modResponseObj(cx, modResponse.toObjectOrNull());
	JS::RootedValue dataVal(cx);
	if (!JS_GetProperty(cx, modResponseObj, "data", &dataVal))
		FAIL("data property not in response.");

	// [modobj1, modobj2, ... ]
	if (!dataVal.isObject())
		FAIL("data property not an object.");

	JS::RootedObject data(cx, dataVal.toObjectOrNull());
	u32 length;
	if (!JS_IsArrayObject(cx, data) || !JS_GetArrayLength(cx, data, &length) || !length)
		FAIL("data property not an array with at least one element.");

	modData.clear();
	modData.reserve(length);

	for (u32 i = 0; i < length; ++i)
	{
		JS::RootedValue el(cx);
		if (!JS_GetElement(cx, data, i, &el) || !el.isObject())
			FAIL("Failed to get array element object.");

		modData.emplace_back();

#define COPY_STRINGS(prefix, obj, ...) \
	for (const std::string& prop : { __VA_ARGS__ }) \
	{ \
		std::string val; \
		if (!ScriptInterface::FromJSProperty(cx, obj, prop.c_str(), val)) \
			FAIL("Failed to get %s from %s.", prop, #obj);\
		modData.back().properties.emplace(prefix+prop, val); \
	}

		// TODO: Currently the homepage_url field does not contain a non-null value for any entry.
		COPY_STRINGS("", el, "name", "name_id", "summary");

		// Now copy over the modfile part, but without the pointless substructure
		JS::RootedObject elObj(cx, el.toObjectOrNull());
		JS::RootedValue modFile(cx);
		if (!JS_GetProperty(cx, elObj, "modfile", &modFile))
			FAIL("Failed to get modfile data.");

		if (!modFile.isObject())
			FAIL("modfile not an object.");

		COPY_STRINGS("", modFile, "version", "filesize");

		JS::RootedObject modFileObj(cx, modFile.toObjectOrNull());
		JS::RootedValue filehash(cx);
		if (!JS_GetProperty(cx, modFileObj, "filehash", &filehash))
			FAIL("Failed to get filehash data.");

		COPY_STRINGS("filehash_", filehash, "md5");

		JS::RootedValue download(cx);
		if (!JS_GetProperty(cx, modFileObj, "download", &download))
			FAIL("Failed to get download data.");

		COPY_STRINGS("", download, "binary_url");

		// Parse metadata_blob (sig+deps)
		std::string metadata_blob;
		if (!ScriptInterface::FromJSProperty(cx, modFile, "metadata_blob", metadata_blob))
			FAIL("Failed to get metadata_blob from modFile.");

		JS::RootedValue metadata(cx);
		if (!scriptInterface.ParseJSON(metadata_blob, &metadata))
			FAIL("Failed to parse metadata_blob as JSON.");

		if (!metadata.isObject())
			FAIL("metadata_blob not decoded as an object.");

		if (!ScriptInterface::FromJSProperty(cx, metadata, "dependencies", modData.back().dependencies))
			FAIL("Failed to get dependencies from metadata_blob.");

		std::vector<std::string> minisigs;
		if (!ScriptInterface::FromJSProperty(cx, metadata, "minisigs", minisigs))
			FAIL("Failed to get minisigs from metadata_blob.");

		// Remove this entry if we did not find a valid matching signature.
		std::string signatureParsingErr;
		if (!ParseSignature(minisigs, modData.back().sig, pk, signatureParsingErr))
			modData.pop_back();

#undef COPY_STRINGS
	}

	return true;
#undef CLEANUP
}

/**
 * Parse signatures to find one that matches the public key, and has a valid global signature.
 * Returns true and sets @param sig to the valid matching signature.
 */
bool ModIo::ParseSignature(const std::vector<std::string>& minisigs, SigStruct& sig, const PKStruct& pk, std::string& err)
{
#define CLEANUP() sig = {};
	for (const std::string& file_sig : minisigs)
	{
		// Format of a .minisig file (created using minisign(1) with -SHm file.zip)
		// untrusted comment: .*\nb64sign_of_file\ntrusted comment: .*\nb64sign_of_sign_of_file_and_trusted_comment

		std::vector<std::string> sig_lines;
		boost::split(sig_lines, file_sig, boost::is_any_of("\n"));
		if (sig_lines.size() < 4)
			FAIL("Invalid (too short) sig.");

		// Verify that both the untrusted comment and the trusted comment start with the correct prefix
		// because that is easy.
		const std::string untrusted_comment_prefix = "untrusted comment: ";
		const std::string trusted_comment_prefix = "trusted comment: ";
		if (!boost::algorithm::starts_with(sig_lines[0], untrusted_comment_prefix))
			FAIL("Malformed untrusted comment.");
		if (!boost::algorithm::starts_with(sig_lines[2], trusted_comment_prefix))
			FAIL("Malformed trusted comment.");

		// We only _really_ care about the second line which is the signature of the file (b64-encoded)
		// Also handling the other signature is nice, but not really required.
		const std::string& msg_sig = sig_lines[1];

		size_t bin_len = 0;
		if (sodium_base642bin((unsigned char*)&sig, sizeof sig, msg_sig.c_str(), msg_sig.size(), NULL, &bin_len, NULL, sodium_base64_VARIANT_ORIGINAL) != 0 || bin_len != sizeof sig)
			FAIL("Failed to decode base64 sig.");

		cassert(sizeof pk.keynum == sizeof sig.keynum);

		if (memcmp(&pk.keynum, &sig.keynum, sizeof sig.keynum) != 0)
			continue; // mismatched key, try another one

		if (memcmp(&sig.sig_alg, "ED", 2) != 0)
			FAIL("Only hashed minisign signatures are supported.");

		// Signature matches our public key

		// Now verify the global signature (sig || trusted_comment)

		unsigned char global_sig[crypto_sign_BYTES];
		if (sodium_base642bin(global_sig, sizeof global_sig, sig_lines[3].c_str(), sig_lines[3].size(), NULL, &bin_len, NULL, sodium_base64_VARIANT_ORIGINAL) != 0 || bin_len != sizeof global_sig)
			FAIL("Failed to decode base64 global_sig.");

		const std::string trusted_comment = sig_lines[2].substr(trusted_comment_prefix.size());

		unsigned char* sig_and_trusted_comment = (unsigned char*)sodium_malloc((sizeof sig.sig) + trusted_comment.size());
		if (!sig_and_trusted_comment)
			FAIL("sodium_malloc failed.");

		memcpy(sig_and_trusted_comment, sig.sig, sizeof sig.sig);
		memcpy(sig_and_trusted_comment + sizeof sig.sig, trusted_comment.data(), trusted_comment.size());

		if (crypto_sign_verify_detached(global_sig, sig_and_trusted_comment, (sizeof sig.sig) + trusted_comment.size(), pk.pk) != 0)
		{
			err = "Failed to verify global signature.";
			sodium_free(sig_and_trusted_comment);
			return false;
		}

		sodium_free(sig_and_trusted_comment);

		// Valid global sig, and the keynum matches the real one
		return true;
	}

	return false;
#undef CLEANUP
}

#undef FAIL
