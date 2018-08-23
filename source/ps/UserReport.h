/* Copyright (C) 2018 Wildfire Games.
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

#ifndef INCLUDED_USERREPORT
#define INCLUDED_USERREPORT

#include <string>

class CUserReporterWorker;

class CUserReporter
{
public:
	CUserReporter();
	~CUserReporter();

	void Initialize();
	void Deinitialize();

	/**
	 * Must be called frequently (preferably every frame), to update some
	 * internal reconnection timers.
	 */
	void Update();

	// Functions for the GUI to control the reporting:
	bool IsReportingEnabled();
	void SetReportingEnabled(bool enabled);
	std::string GetStatus();

	/**
	 * Submit a report to be transmitted to the online server.
	 * Nothing will be transmitted until reporting is enabled by the user, so
	 * you don't need to check for that first.
	 * @param type short string identifying the type of data ("hwdetect", "message", etc)
	 * @param version positive integer that should be incremented if the data is changed in
	 * a non-compatible way and the server will have to distinguish old and new formats
	 * @param data the actual data (typically UTF-8-encoded text, or JSON, but could be binary)
	 * @param dataHumanReadable an optional, readable representation of the same data, allowing users to assess for privacy concerns
	 */
	void SubmitReport(const std::string& type, int version, const std::string& data, const std::string& dataHumanReadable);

private:
	std::string LoadUserID();

	CUserReporterWorker* m_Worker;
};

extern CUserReporter g_UserReporter;

#endif // INCLUDED_USERREPORT
