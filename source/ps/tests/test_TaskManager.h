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

#include "lib/self_test.h"

#include "ps/Future.h"
#include "ps/TaskManager.h"

#include <atomic>
#include <condition_variable>
#include <mutex>

class TestTaskManager : public CxxTest::TestSuite
{
public:
	void test_basic()
	{
		Threading::TaskManager& taskManager = Threading::TaskManager::Instance();
		// There is a minimum of 3.
		TS_ASSERT(taskManager.GetNumberOfWorkers() >= 3);

		std::atomic<int> tasks_run = 0;
		auto increment_run = [&tasks_run]() { tasks_run++; };
		Future future = taskManager.PushTask(increment_run);
		future.Wait();
		TS_ASSERT_EQUALS(tasks_run.load(), 1);

		// Test Execute.
		std::condition_variable cv;
		std::mutex mutex;
		std::atomic<bool> go = false;
		future = taskManager.PushTask([&]() {
			std::unique_lock<std::mutex> lock(mutex);
			cv.wait(lock, [&go]() -> bool { return go; });
			lock.unlock();
			increment_run();
			lock.lock();
			go = false;
			lock.unlock();
			cv.notify_all();
		});
		TS_ASSERT_EQUALS(tasks_run.load(), 1);
		std::unique_lock<std::mutex> lock(mutex);
		go = true;
		lock.unlock();
		cv.notify_all();
		lock.lock();
		cv.wait(lock, [&go]() -> bool { return !go; });
		TS_ASSERT_EQUALS(tasks_run.load(), 2);
		// Wait on the future before the mutex/cv go out of scope.
		future.Wait();
	}

	void test_Priority()
	{
		Threading::TaskManager& taskManager = Threading::TaskManager::Instance();
		std::atomic<int> tasks_run = 0;
		// Push general tasks
		auto increment_run = [&tasks_run]() { tasks_run++; };
		Future future = taskManager.PushTask(increment_run);
		Future futureLow = taskManager.PushTask(increment_run, Threading::TaskPriority::LOW);
		future.Wait();
		futureLow.Wait();
		TS_ASSERT_EQUALS(tasks_run.load(), 2);
		// Also check with no waiting expected.
		taskManager.PushTask(increment_run).Wait();
		TS_ASSERT_EQUALS(tasks_run.load(), 3);
		taskManager.PushTask(increment_run, Threading::TaskPriority::LOW).Wait();
		TS_ASSERT_EQUALS(tasks_run.load(), 4);
	}

	void test_Load()
	{
		Threading::TaskManager& taskManager = Threading::TaskManager::Instance();

#define ITERATIONS 100000
		std::vector<Future<int>> futures;
		futures.resize(ITERATIONS);
		std::vector<u32> values(ITERATIONS);

		auto f1 = taskManager.PushTask([&taskManager, &futures]() {
			for (u32 i = 0; i < ITERATIONS; i+=3)
				futures[i] = taskManager.PushTask([]() { return 5; });
		});

		auto f2 = taskManager.PushTask([&taskManager, &futures]() {
			for (u32 i = 1; i < ITERATIONS; i+=3)
				futures[i] = taskManager.PushTask([]() { return 5; }, Threading::TaskPriority::LOW);
		});

		auto f3 = taskManager.PushTask([&taskManager, &futures]() {
			for (u32 i = 2; i < ITERATIONS; i+=3)
				futures[i] = taskManager.PushTask([]() { return 5; });
		});

		f1.Wait();
		f2.Wait();
		f3.Wait();

		for (size_t i = 0; i < ITERATIONS; ++i)
			TS_ASSERT_EQUALS(futures[i].Get(), 5);
#undef ITERATIONS
	}
};
