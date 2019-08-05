/* Copyright (C) 2019 Wildfire Games.
 * This file is part of 0 A.D.
 *
 * 0 A.D. is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * 0 A.D. is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 0 A.D.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "precompiled.h"

#include "UserReport.h"

#include "lib/timer.h"
#include "lib/utf8.h"
#include "lib/external_libraries/curl.h"
#include "lib/external_libraries/libsdl.h"
#include "lib/external_libraries/zlib.h"
#include "lib/file/archive/stream.h"
#include "lib/os_path.h"
#include "lib/sysdep/sysdep.h"
#include "ps/ConfigDB.h"
#include "ps/Filesystem.h"
#include "ps/Profiler2.h"
#include "ps/ThreadUtil.h"

#include <fstream>
#include <mutex>
#include <string>

#define DEBUG_UPLOADS 0

/*
 * The basic idea is that the game submits reports to us, which we send over
 * HTTP to a server for storage and analysis.
 *
 * We can't use libcurl's asynchronous 'multi' API, because DNS resolution can
 * be synchronous and slow (which would make the game pause).
 * So we use the 'easy' API in a background thread.
 * The main thread submits reports, toggles whether uploading is enabled,
 * and polls for the current status (typically to display in the GUI);
 * the worker thread does all of the uploading.
 *
 * It'd be nice to extend this in the future to handle things like crash reports.
 * The game should store the crashlogs (suitably anonymised) in a directory, and
 * we should detect those files and upload them when we're restarted and online.
 */


/**
 * Version number stored in config file when the user agrees to the reporting.
 * Reporting will be disabled if the config value is missing or is less than
 * this value. If we start reporting a lot more data, we should increase this
 * value and get the user to re-confirm.
 */
static const int REPORTER_VERSION = 1;

/**
 * Time interval (seconds) at which the worker thread will check its reconnection
 * timers. (This should be relatively high so the thread doesn't waste much time
 * continually waking up.)
 */
static const double TIMER_CHECK_INTERVAL = 10.0;

/**
 * Seconds we should wait before reconnecting to the server after a failure.
 */
static const double RECONNECT_INVERVAL = 60.0;

CUserReporter g_UserReporter;

struct CUserReport
{
	time_t m_Time;
	std::string m_Type;
	int m_Version;
	std::string m_Data;
};

class CUserReporterWorker
{
public:
	CUserReporterWorker(const std::string& userID, const std::string& url) :
		m_URL(url), m_UserID(userID), m_Enabled(false), m_Shutdown(false), m_Status("disabled"),
		m_PauseUntilTime(timer_Time()), m_LastUpdateTime(timer_Time())
	{
		// Set up libcurl:

		m_Curl = curl_easy_init();
		ENSURE(m_Curl);

#if DEBUG_UPLOADS
		curl_easy_setopt(m_Curl, CURLOPT_VERBOSE, 1L);
#endif

		// Capture error messages
		curl_easy_setopt(m_Curl, CURLOPT_ERRORBUFFER, m_ErrorBuffer);

		// Disable signal handlers (required for multithreaded applications)
		curl_easy_setopt(m_Curl, CURLOPT_NOSIGNAL, 1L);

		// To minimise security risks, don't support redirects
		curl_easy_setopt(m_Curl, CURLOPT_FOLLOWLOCATION, 0L);

		// Prevent this thread from blocking the engine shutdown for 5 minutes in case the server is unavailable
		curl_easy_setopt(m_Curl, CURLOPT_CONNECTTIMEOUT, 10L);

		// Set IO callbacks
		curl_easy_setopt(m_Curl, CURLOPT_WRITEFUNCTION, ReceiveCallback);
		curl_easy_setopt(m_Curl, CURLOPT_WRITEDATA, this);
		curl_easy_setopt(m_Curl, CURLOPT_READFUNCTION, SendCallback);
		curl_easy_setopt(m_Curl, CURLOPT_READDATA, this);

		// Set URL to POST to
		curl_easy_setopt(m_Curl, CURLOPT_URL, url.c_str());
		curl_easy_setopt(m_Curl, CURLOPT_POST, 1L);

		// Set up HTTP headers
		m_Headers = NULL;
		// Set the UA string
		std::string ua = "User-Agent: 0ad ";
		ua += curl_version();
		ua += " (http://play0ad.com/)";
		m_Headers = curl_slist_append(m_Headers, ua.c_str());
		// Override the default application/x-www-form-urlencoded type since we're not using that type
		m_Headers = curl_slist_append(m_Headers, "Content-Type: application/octet-stream");
		// Disable the Accept header because it's a waste of a dozen bytes
		m_Headers = curl_slist_append(m_Headers, "Accept: ");
		curl_easy_setopt(m_Curl, CURLOPT_HTTPHEADER, m_Headers);


		// Set up the worker thread:

		// Use SDL semaphores since OS X doesn't implement sem_init
		m_WorkerSem = SDL_CreateSemaphore(0);
		ENSURE(m_WorkerSem);

		int ret = pthread_create(&m_WorkerThread, NULL, &RunThread, this);
		ENSURE(ret == 0);
	}

	~CUserReporterWorker()
	{
		// Clean up resources

		SDL_DestroySemaphore(m_WorkerSem);

		curl_slist_free_all(m_Headers);
		curl_easy_cleanup(m_Curl);
	}

	/**
	 * Called by main thread, when the online reporting is enabled/disabled.
	 */
	void SetEnabled(bool enabled)
	{
		std::lock_guard<std::mutex> lock(m_WorkerMutex);
		if (enabled != m_Enabled)
		{
			m_Enabled = enabled;

			// Wake up the worker thread
			SDL_SemPost(m_WorkerSem);
		}
	}

	/**
	 * Called by main thread to request shutdown.
	 * Returns true if we've shut down successfully.
	 * Returns false if shutdown is taking too long (we might be blocked on a
	 * sync network operation) - you mustn't destroy this object, just leak it
	 * and terminate.
	 */
	bool Shutdown()
	{
		{
			std::lock_guard<std::mutex> lock(m_WorkerMutex);
			m_Shutdown = true;
		}

		// Wake up the worker thread
		SDL_SemPost(m_WorkerSem);

		// Wait for it to shut down cleanly
		// TODO: should have a timeout in case of network hangs
		pthread_join(m_WorkerThread, NULL);

		return true;
	}

	/**
	 * Called by main thread to determine the current status of the uploader.
	 */
	std::string GetStatus()
	{
		std::lock_guard<std::mutex> lock(m_WorkerMutex);
		return m_Status;
	}

	/**
	 * Called by main thread to add a new report to the queue.
	 */
	void Submit(const shared_ptr<CUserReport>& report)
	{
		{
			std::lock_guard<std::mutex> lock(m_WorkerMutex);
			m_ReportQueue.push_back(report);
		}

		// Wake up the worker thread
		SDL_SemPost(m_WorkerSem);
	}

	/**
	 * Called by the main thread every frame, so we can check
	 * retransmission timers.
	 */
	void Update()
	{
		double now = timer_Time();
		if (now > m_LastUpdateTime + TIMER_CHECK_INTERVAL)
		{
			// Wake up the worker thread
			SDL_SemPost(m_WorkerSem);

			m_LastUpdateTime = now;
		}
	}

private:
	static void* RunThread(void* data)
	{
		debug_SetThreadName("CUserReportWorker");
		g_Profiler2.RegisterCurrentThread("userreport");

		static_cast<CUserReporterWorker*>(data)->Run();

		return NULL;
	}

	void Run()
	{
		// Set libcurl's proxy configuration
		// (This has to be done in the thread because it's potentially very slow)
		SetStatus("proxy");
		std::wstring proxy;

		{
			PROFILE2("get proxy config");
			if (sys_get_proxy_config(wstring_from_utf8(m_URL), proxy) == INFO::OK)
				curl_easy_setopt(m_Curl, CURLOPT_PROXY, utf8_from_wstring(proxy).c_str());
		}

		SetStatus("waiting");

		/*
		 * We use a semaphore to let the thread be woken up when it has
		 * work to do. Various actions from the main thread can wake it:
		 *   * SetEnabled()
		 *   * Shutdown()
		 *   * Submit()
		 *   * Retransmission timeouts, once every several seconds
		 *
		 * If multiple actions have triggered wakeups, we might respond to
		 * all of those actions after the first wakeup, which is okay (we'll do
		 * nothing during the subsequent wakeups). We should never hang due to
		 * processing fewer actions than wakeups.
		 *
		 * Retransmission timeouts are triggered via the main thread - we can't simply
		 * use SDL_SemWaitTimeout because on Linux it's implemented as an inefficient
		 * busy-wait loop, and we can't use a manual busy-wait with a long delay time
		 * because we'd lose responsiveness. So the main thread pings the worker
		 * occasionally so it can check its timer.
		 */

		// Wait until the main thread wakes us up
		while (true)
		{
			g_Profiler2.RecordRegionEnter("semaphore wait");

			ENSURE(SDL_SemWait(m_WorkerSem) == 0);

			g_Profiler2.RecordRegionLeave();

			// Handle shutdown requests as soon as possible
			if (GetShutdown())
				return;

			// If we're not enabled, ignore this wakeup
			if (!GetEnabled())
				continue;

			// If we're still pausing due to a failed connection,
			// go back to sleep again
			if (timer_Time() < m_PauseUntilTime)
				continue;

			// We're enabled, so process as many reports as possible
			while (ProcessReport())
			{
				// Handle shutdowns while we were sending the report
				if (GetShutdown())
					return;
			}
		}
	}

	bool GetEnabled()
	{
		std::lock_guard<std::mutex> lock(m_WorkerMutex);
		return m_Enabled;
	}

	bool GetShutdown()
	{
		std::lock_guard<std::mutex> lock(m_WorkerMutex);
		return m_Shutdown;
	}

	void SetStatus(const std::string& status)
	{
		std::lock_guard<std::mutex> lock(m_WorkerMutex);
		m_Status = status;
#if DEBUG_UPLOADS
		debug_printf(">>> CUserReporterWorker status: %s\n", status.c_str());
#endif
	}

	bool ProcessReport()
	{
		PROFILE2("process report");

		shared_ptr<CUserReport> report;

		{
			std::lock_guard<std::mutex> lock(m_WorkerMutex);
			if (m_ReportQueue.empty())
				return false;
			report = m_ReportQueue.front();
			m_ReportQueue.pop_front();
		}

		ConstructRequestData(*report);
		m_RequestDataOffset = 0;
		m_ResponseData.clear();
		m_ErrorBuffer[0] = '\0';

		curl_easy_setopt(m_Curl, CURLOPT_POSTFIELDSIZE_LARGE, (curl_off_t)m_RequestData.size());

		SetStatus("connecting");

#if DEBUG_UPLOADS
		TIMER(L"CUserReporterWorker request");
#endif

		CURLcode err = curl_easy_perform(m_Curl);

#if DEBUG_UPLOADS
		printf(">>>\n%s\n<<<\n", m_ResponseData.c_str());
#endif

		if (err == CURLE_OK)
		{
			long code = -1;
			curl_easy_getinfo(m_Curl, CURLINFO_RESPONSE_CODE, &code);
			SetStatus("completed:" + CStr::FromInt(code));

			// Check for success code
			if (code == 200)
				return true;

			// If the server returns the 410 Gone status, interpret that as meaning
			// it no longer supports uploads (at least from this version of the game),
			// so shut down and stop talking to it (to avoid wasting bandwidth)
			if (code == 410)
			{
				std::lock_guard<std::mutex> lock(m_WorkerMutex);
				m_Shutdown = true;
				return false;
			}
		}
		else
		{
			std::string errorString(m_ErrorBuffer);

			if (errorString.empty())
				errorString = curl_easy_strerror(err);

			SetStatus("failed:" + CStr::FromInt(err) + ":" + errorString);
		}

		// We got an unhandled return code or a connection failure;
		// push this report back onto the queue and try again after
		// a long interval

		{
			std::lock_guard<std::mutex> lock(m_WorkerMutex);
			m_ReportQueue.push_front(report);
		}

		m_PauseUntilTime = timer_Time() + RECONNECT_INVERVAL;
		return false;
	}

	void ConstructRequestData(const CUserReport& report)
	{
		// Construct the POST request data in the application/x-www-form-urlencoded format

		std::string r;

		r += "user_id=";
		AppendEscaped(r, m_UserID);

		r += "&time=" + CStr::FromInt64(report.m_Time);

		r += "&type=";
		AppendEscaped(r, report.m_Type);

		r += "&version=" + CStr::FromInt(report.m_Version);

		r += "&data=";
		AppendEscaped(r, report.m_Data);

		// Compress the content with zlib to save bandwidth.
		// (Note that we send a request with unlabelled compressed data instead
		// of using Content-Encoding, because Content-Encoding is a mess and causes
		// problems with servers and breaks Content-Length and this is much easier.)
		std::string compressed;
		compressed.resize(compressBound(r.size()));
		uLongf destLen = compressed.size();
		int ok = compress((Bytef*)compressed.c_str(), &destLen, (const Bytef*)r.c_str(), r.size());
		ENSURE(ok == Z_OK);
		compressed.resize(destLen);

		m_RequestData.swap(compressed);
	}

	void AppendEscaped(std::string& buffer, const std::string& str)
	{
		char* escaped = curl_easy_escape(m_Curl, str.c_str(), str.size());
		buffer += escaped;
		curl_free(escaped);
	}

	static size_t ReceiveCallback(void* buffer, size_t size, size_t nmemb, void* userp)
	{
		CUserReporterWorker* self = static_cast<CUserReporterWorker*>(userp);

		if (self->GetShutdown())
			return 0; // signals an error

		self->m_ResponseData += std::string((char*)buffer, (char*)buffer+size*nmemb);

		return size*nmemb;
	}

	static size_t SendCallback(char* bufptr, size_t size, size_t nmemb, void* userp)
	{
		CUserReporterWorker* self = static_cast<CUserReporterWorker*>(userp);

		if (self->GetShutdown())
			return CURL_READFUNC_ABORT; // signals an error

		// We can return as much data as available, up to the buffer size
		size_t amount = std::min(self->m_RequestData.size() - self->m_RequestDataOffset, size*nmemb);

		// ...But restrict to sending a small amount at once, so that we remain
		// responsive to shutdown requests even if the network is pretty slow
		amount = std::min((size_t)1024, amount);

		if(amount != 0)	// (avoids invalid operator[] call where index=size)
		{
			memcpy(bufptr, &self->m_RequestData[self->m_RequestDataOffset], amount);
			self->m_RequestDataOffset += amount;
		}

		self->SetStatus("sending:" + CStr::FromDouble((double)self->m_RequestDataOffset / self->m_RequestData.size()));

		return amount;
	}

private:
	// Thread-related members:
	pthread_t m_WorkerThread;
	std::mutex m_WorkerMutex;
	SDL_sem* m_WorkerSem;

	// Shared by main thread and worker thread:
	// These variables are all protected by m_WorkerMutex
	std::deque<shared_ptr<CUserReport> > m_ReportQueue;
	bool m_Enabled;
	bool m_Shutdown;
	std::string m_Status;

	// Initialised in constructor by main thread; otherwise used only by worker thread:
	std::string m_URL;
	std::string m_UserID;
	CURL* m_Curl;
	curl_slist* m_Headers;
	double m_PauseUntilTime;

	// Only used by worker thread:
	std::string m_ResponseData;
	std::string m_RequestData;
	size_t m_RequestDataOffset;
	char m_ErrorBuffer[CURL_ERROR_SIZE];

	// Only used by main thread:
	double m_LastUpdateTime;
};



CUserReporter::CUserReporter() :
	m_Worker(NULL)
{
}

CUserReporter::~CUserReporter()
{
	ENSURE(!m_Worker); // Deinitialize should have been called before shutdown
}

std::string CUserReporter::LoadUserID()
{
	std::string userID;

	// Read the user ID from user.cfg (if there is one)
	CFG_GET_VAL("userreport.id", userID);

	// If we don't have a validly-formatted user ID, generate a new one
	if (userID.length() != 16)
	{
		u8 bytes[8] = {0};
		sys_generate_random_bytes(bytes, ARRAY_SIZE(bytes));
		// ignore failures - there's not much we can do about it

		userID = "";
		for (size_t i = 0; i < ARRAY_SIZE(bytes); ++i)
		{
			char hex[3];
			sprintf_s(hex, ARRAY_SIZE(hex), "%02x", (unsigned int)bytes[i]);
			userID += hex;
		}

		g_ConfigDB.SetValueString(CFG_USER, "userreport.id", userID);
		g_ConfigDB.WriteValueToFile(CFG_USER, "userreport.id", userID);
	}

	return userID;
}

bool CUserReporter::IsReportingEnabled()
{
	int version = -1;
	CFG_GET_VAL("userreport.enabledversion", version);
	return (version >= REPORTER_VERSION);
}

void CUserReporter::SetReportingEnabled(bool enabled)
{
	CStr val = CStr::FromInt(enabled ? REPORTER_VERSION : 0);
	g_ConfigDB.SetValueString(CFG_USER, "userreport.enabledversion", val);
	g_ConfigDB.WriteValueToFile(CFG_USER, "userreport.enabledversion", val);

	if (m_Worker)
		m_Worker->SetEnabled(enabled);
}

std::string CUserReporter::GetStatus()
{
	if (!m_Worker)
		return "disabled";

	return m_Worker->GetStatus();
}

void CUserReporter::Initialize()
{
	ENSURE(!m_Worker); // must only be called once

	std::string userID = LoadUserID();
	std::string url;
	CFG_GET_VAL("userreport.url_upload", url);

	m_Worker = new CUserReporterWorker(userID, url);

	m_Worker->SetEnabled(IsReportingEnabled());
}

void CUserReporter::Deinitialize()
{
	if (!m_Worker)
		return;

	if (m_Worker->Shutdown())
	{
		// Worker was shut down cleanly
		SAFE_DELETE(m_Worker);
	}
	else
	{
		// Worker failed to shut down in a reasonable time
		// Leak the resources (since that's better than hanging or crashing)
		m_Worker = NULL;
	}
}

void CUserReporter::Update()
{
	if (m_Worker)
		m_Worker->Update();
}

void CUserReporter::SubmitReport(const std::string& type, int version, const std::string& data, const std::string& dataHumanReadable)
{
	// Write to logfile, enabling users to assess privacy concerns before the data is submitted
	if (!dataHumanReadable.empty())
	{
		OsPath path = psLogDir() / OsPath("userreport_" + type + ".txt");
		std::ofstream stream(OsString(path), std::ofstream::trunc);
		if (stream)
		{
			debug_printf("UserReport written to %s\n", path.string8().c_str());
			stream << dataHumanReadable << std::endl;
			stream.close();
		}
		else
			debug_printf("Failed to write UserReport to %s\n", path.string8().c_str());
	}

	// If not initialised, discard the report
	if (!m_Worker)
		return;

	// Actual submit
	shared_ptr<CUserReport> report(new CUserReport);
	report->m_Time = time(NULL);
	report->m_Type = type;
	report->m_Version = version;
	report->m_Data = data;

	m_Worker->Submit(report);
}
