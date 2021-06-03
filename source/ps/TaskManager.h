/* Copyright (C) 2021 Wildfire Games.
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

#ifndef INCLUDED_THREADING_TASKMANAGER
#define INCLUDED_THREADING_TASKMANAGER

#include "ps/Future.h"

#include <memory>
#include <vector>

class TestTaskManager;
class CConfigDB;

namespace Threading
{
class TaskManager;
class WorkerThread;

enum class TaskPriority
{
	NORMAL,
	LOW
};

/**
 * The task manager creates all worker threads on initialisation,
 * and manages the task queues.
 * See implementation for additional comments.
 */
class TaskManager
{
	friend class WorkerThread;
public:
	TaskManager();
	~TaskManager();
	TaskManager(const TaskManager&) = delete;
	TaskManager(TaskManager&&) = delete;
	TaskManager& operator=(const TaskManager&) = delete;
	TaskManager& operator=(TaskManager&&) = delete;

	static void Initialise();
	static TaskManager& Instance();

	/**
	 * Clears all tasks from the queue. This blocks on started tasks.
	 */
	void ClearQueue();

	/**
	 * @return the number of threaded workers.
	 */
	size_t GetNumberOfWorkers() const;

	/**
	 * Push a task to be executed.
	 */
	template<typename T>
	Future<std::invoke_result_t<T>> PushTask(T&& func, TaskPriority priority = TaskPriority::NORMAL)
	{
		Future<std::invoke_result_t<T>> ret;
		DoPushTask(std::move(ret.Wrap(std::move(func))), priority);
		return ret;
	}

private:
	TaskManager(size_t numberOfWorkers);

	void DoPushTask(std::function<void()>&& task, TaskPriority priority);

	class Impl;
	std::unique_ptr<Impl> m;
};
} // namespace Threading

#endif // INCLUDED_THREADING_TASKMANAGER
