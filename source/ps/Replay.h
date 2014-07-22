/* Copyright (C) 2014 Wildfire Games.
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

#ifndef INCLUDED_REPLAY
#define INCLUDED_REPLAY

class CScriptValRooted;
struct SimulationCommand;
class ScriptInterface;

/**
 * Replay log recorder interface.
 * Call its methods at appropriate times during the game.
 */
class IReplayLogger
{
public:
	IReplayLogger() { }
	virtual ~IReplayLogger() { }

	/**
	 * Started the game with the given game attributes.
	 */
	virtual void StartGame(const CScriptValRooted& attribs) = 0;

	/**
	 * Run the given turn with the given collection of player commands.
	 */
	virtual void Turn(u32 n, u32 turnLength, const std::vector<SimulationCommand>& commands) = 0;

	/**
	 * Optional hash of simulation state (for sync checking).
	 */
	virtual void Hash(const std::string& hash, bool quick) = 0;
};

/**
 * Implementation of IReplayLogger that simply throws away all data.
 */
class CDummyReplayLogger : public IReplayLogger
{
public:
	virtual void StartGame(const CScriptValRooted& UNUSED(attribs)) { }
	virtual void Turn(u32 UNUSED(n), u32 UNUSED(turnLength), const std::vector<SimulationCommand>& UNUSED(commands)) { }
	virtual void Hash(const std::string& UNUSED(hash), bool UNUSED(quick)) { }
};

/**
 * Implementation of IReplayLogger that saves data to a file in the logs directory.
 */
class CReplayLogger : public IReplayLogger
{
	NONCOPYABLE(CReplayLogger);
public:
	CReplayLogger(ScriptInterface& scriptInterface);
	~CReplayLogger();

	virtual void StartGame(const CScriptValRooted& attribs);
	virtual void Turn(u32 n, u32 turnLength, const std::vector<SimulationCommand>& commands);
	virtual void Hash(const std::string& hash, bool quick);

private:
	ScriptInterface& m_ScriptInterface;
	std::ostream* m_Stream;
};

/**
 * Replay log replayer. Runs the log with no graphics and dumps some info to stdout.
 */
class CReplayPlayer
{
public:
	CReplayPlayer();
	~CReplayPlayer();

	void Load(const std::string& path);
	void Replay(bool serializationtest);

private:
	std::istream* m_Stream;
};

#endif // INCLUDED_REPLAY
