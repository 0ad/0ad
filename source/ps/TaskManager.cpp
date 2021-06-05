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

#include "precompiled.h"

#include "TaskManager.h"

#include "lib/debug.h"
#include "maths/MathUtil.h"
#include "ps/CLogger.h"
#include "ps/ConfigDB.h"
#include "ps/Threading.h"
#include "ps/ThreadUtil.h"
#include "ps/Profiler2.h"

#include <condition_variable>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>

namespace Threading
{
/**
 * Minimum number of TaskManager workers.
 */
static constexpr size_t MIN_THREADS = 3;

/**
 * Maximum number of TaskManager workers.
 */
static constexpr size_t MAX_THREADS = 32;

std::unique_ptr<TaskManager> g_TaskManager;

class Thread;

using QueueItem = std::function<void()>;

/**
 * Light wrapper around std::thread. Ensures Join has been called.
 */
class Thread
{
public:
	Thread() = default;
	Thread(const Thread&) = delete;
	Thread(Thread&&) = delete;

	template<typename T, void(T::* callable)()>
	void Start(T* object)
	{
		m_Thread = std::thread(HandleExceptions<DoStart<T, callable>>::Wrapper, object);
	}
	template<typename T, void(T::* callable)()>
	static void DoStart(T* object);

protected:
	~Thread()
	{
		ENSURE(!m_Thread.joinable());
	}

	std::thread m_Thread;
	std::atomic<bool> m_Kill = false;
};

/**
 * Worker thread: process the taskManager queues until killed.
 */
class WorkerThread : public Thread
{
public:
	WorkerThread(TaskManager::Impl& taskManager);
	~WorkerThread();

	/**
	 * Wake the worker.
	 */
	void Wake();

protected:
	void RunUntilDeath();

	std::mutex m_Mutex;
	std::condition_variable m_ConditionVariable;

	TaskManager::Impl& m_TaskManager;
};

/**
 * PImpl-ed implementation of the Task manager.
 *
 * The normal priority queue is processed first, the low priority only if there are no higher-priority tasks
 */
class TaskManager::Impl
{
	friend class TaskManager;
	friend class WorkerThread;
public:
	Impl(TaskManager& backref);
	~Impl()
	{
		ClearQueue();
		m_Workers.clear();
	}

	/**
	 * 2-phase init to avoid having to think too hard about the order of class members.
	 */
	void SetupWorkers(size_t numberOfWorkers);

	/**
	 * Push a task on the global queue.
	 * Takes ownership of @a task.
	 * May be called from any thread.
	 */
	void PushTask(std::function<void()>&& task, TaskPriority priority);

protected:
	void ClearQueue();

	template<TaskPriority Priority>
	bool PopTask(std::function<void()>& taskOut);

	// Back reference (keep this first).
	TaskManager& m_TaskManager;

	std::atomic<bool> m_HasWork = false;
	std::atomic<bool> m_HasLowPriorityWork = false;
	std::mutex m_GlobalMutex;
	std::mutex m_GlobalLowPriorityMutex;
	std::deque<QueueItem> m_GlobalQueue;
	std::deque<QueueItem> m_GlobalLowPriorityQueue;

	// Ideally this would be a vector, since it does get iterated, but that requires movable types.
	std::deque<WorkerThread> m_Workers;

	// Round-robin counter for GetWorker.
	mutable size_t m_RoundRobinIdx = 0;
};

TaskManager::TaskManager() : TaskManager(std::thread::hardware_concurrency() - 1)
{
}

TaskManager::TaskManager(size_t numberOfWorkers)
{
	m = std::make_unique<Impl>(*this);
	numberOfWorkers = Clamp<size_t>(numberOfWorkers, MIN_THREADS, MAX_THREADS);
	m->SetupWorkers(numberOfWorkers);
}

TaskManager::~TaskManager() {}

TaskManager::Impl::Impl(TaskManager& backref)
	: m_TaskManager(backref)
{
}

void TaskManager::Impl::SetupWorkers(size_t numberOfWorkers)
{
	for (size_t i = 0; i < numberOfWorkers; ++i)
		m_Workers.emplace_back(*this);
}

void TaskManager::ClearQueue() { m->ClearQueue(); }
void TaskManager::Impl::ClearQueue()
{
	{
		std::lock_guard<std::mutex> lock(m_GlobalMutex);
		m_GlobalQueue.clear();
	}
	{
		std::lock_guard<std::mutex> lock(m_GlobalLowPriorityMutex);
		m_GlobalLowPriorityQueue.clear();
	}
}

size_t TaskManager::GetNumberOfWorkers() const
{
	return m->m_Workers.size();
}

void TaskManager::DoPushTask(std::function<void()>&& task, TaskPriority priority)
{
	m->PushTask(std::move(task), priority);
}

void TaskManager::Impl::PushTask(std::function<void()>&& task, TaskPriority priority)
{
	std::mutex& mutex = priority == TaskPriority::NORMAL ? m_GlobalMutex : m_GlobalLowPriorityMutex;
	std::deque<QueueItem>& queue = priority == TaskPriority::NORMAL ? m_GlobalQueue : m_GlobalLowPriorityQueue;
	std::atomic<bool>& hasWork = priority == TaskPriority::NORMAL ? m_HasWork : m_HasLowPriorityWork;
	{
		std::lock_guard<std::mutex> lock(mutex);
		queue.emplace_back(std::move(task));
		hasWork = true;
	}

	for (WorkerThread& worker : m_Workers)
		worker.Wake();
}

template<TaskPriority Priority>
bool TaskManager::Impl::PopTask(std::function<void()>& taskOut)
{
	std::mutex& mutex = Priority == TaskPriority::NORMAL ? m_GlobalMutex : m_GlobalLowPriorityMutex;
	std::deque<QueueItem>& queue = Priority == TaskPriority::NORMAL ? m_GlobalQueue : m_GlobalLowPriorityQueue;
	std::atomic<bool>& hasWork = Priority == TaskPriority::NORMAL ? m_HasWork : m_HasLowPriorityWork;

	// Particularly critical section since we're locking the global queue.
	std::lock_guard<std::mutex> globalLock(mutex);
	if (!queue.empty())
	{
		taskOut = std::move(queue.front());
		queue.pop_front();
		hasWork = !queue.empty();
		return true;
	}
	return false;
}

void TaskManager::Initialise()
{
	if (!g_TaskManager)
		g_TaskManager = std::make_unique<TaskManager>();
}

TaskManager& TaskManager::Instance()
{
	ENSURE(g_TaskManager);
	return *g_TaskManager;
}

// Thread definition

WorkerThread::WorkerThread(TaskManager::Impl& taskManager)
	: m_TaskManager(taskManager)
{
	Start<WorkerThread, &WorkerThread::RunUntilDeath>(this);
}

WorkerThread::~WorkerThread()
{
	m_Kill = true;
	m_ConditionVariable.notify_one();
	if (m_Thread.joinable())
		m_Thread.join();
}

void WorkerThread::Wake()
{
	m_ConditionVariable.notify_one();
}

void WorkerThread::RunUntilDeath()
{
	// The profiler does better if the names are unique.
	static std::atomic<int> n = 0;
	std::string name = "Task Mgr #" + std::to_string(n++);
	debug_SetThreadName(name.c_str());
	g_Profiler2.RegisterCurrentThread(name);


	std::function<void()> task;
	bool hasTask = false;
	std::unique_lock<std::mutex> lock(m_Mutex, std::defer_lock);
	while (!m_Kill)
	{
		lock.lock();
		m_ConditionVariable.wait(lock, [this](){
			return m_Kill || m_TaskManager.m_HasWork || m_TaskManager.m_HasLowPriorityWork;
		});
		lock.unlock();

		if (m_Kill)
			break;

		// Fetch work from the global queues.
		hasTask = m_TaskManager.PopTask<TaskPriority::NORMAL>(task);
		if (!hasTask)
			hasTask = m_TaskManager.PopTask<TaskPriority::LOW>(task);
		if (hasTask)
			task();
	}
}

// Defined here - needs access to derived types.
template<typename T, void(T::* callable)()>
void Thread::DoStart(T* object)
{
	std::invoke(callable, object);
}

} // namespace Threading
