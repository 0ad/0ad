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

#include <functional>
#include <type_traits>

class TestFuture : public CxxTest::TestSuite
{
public:
	void test_future_basic()
	{
		int counter = 0;
		{
			Future<void> noret;
			std::function<void()> task = noret.Wrap([&counter]() mutable { counter++; });
			task();
			TS_ASSERT_EQUALS(counter, 1);
		}

		{
			Future<void> noret;
			{
				std::function<void()> task = noret.Wrap([&counter]() mutable { counter++; });
				// Auto-cancels the task.
			}
		}
		TS_ASSERT_EQUALS(counter, 1);
	}

	void test_future_return()
	{
		{
			Future<int> future;
			std::function<void()> task = future.Wrap([]() { return 1; });
			task();
			TS_ASSERT_EQUALS(future.Get(), 1);
		}

		// Convertible type.
		{
			Future<int> future;
			std::function<void()> task = future.Wrap([]() -> u8 { return 1; });
			task();
			TS_ASSERT_EQUALS(future.Get(), 1);
		}

		static int destroyed = 0;
		// No trivial constructor or destructor. Also not copiable.
		struct NonDef
		{
			NonDef() = delete;
			NonDef(int input) : value(input) {};
			NonDef(const NonDef&) = delete;
			NonDef(NonDef&& o)
			{
				value = o.value;
				o.value = 0;
			}
			~NonDef() { if (value != 0) destroyed++; }
			int value = 0;
		};
		TS_ASSERT_EQUALS(destroyed, 0);
		{
			Future<NonDef> future;
			std::function<void()> task = future.Wrap([]() { return 1; });
			task();
			TS_ASSERT_EQUALS(future.Get().value, 1);
		}
		TS_ASSERT_EQUALS(destroyed, 1);
		{
			Future<NonDef> future;
			std::function<void()> task = future.Wrap([]() { return 1; });
		}
		TS_ASSERT_EQUALS(destroyed, 1);
		/**
		 * TODO: find a way to test this
		{
			Future<NonDef> future;
			std::function<void()> task = future.Wrap([]() { return 1; });
			future.CancelOrWait();
			TS_ASSERT_THROWS(future.Get(), const Future<NonDef>::BadFutureAccess&);
		}
		 */
		TS_ASSERT_EQUALS(destroyed, 1);
	}

	void test_future_moving()
	{
		Future<int> future;
		std::function<int()> function;

		// Set things up so all temporaries passed into the futures will be reset to obviously invalid memory.
		std::aligned_storage_t<sizeof(Future<int>), alignof(Future<int>)> futureStorage;
		std::aligned_storage_t<sizeof(std::function<int()>), alignof(std::function<int()>)> functionStorage;
		Future<int>* f = &future; // CppCheck doesn't read placement new correctly, do this to silence errors.
		std::function<int()>* c = &function;
		f = new (&futureStorage) Future<int>{};
		c = new (&functionStorage) std::function<int()>{};

		*c = []() { return 7; };
		std::function<void()> task = f->Wrap(std::move(*c));

		future = std::move(*f);
		function = std::move(*c);

		// Destroy and clear the memory
		f->~Future();
		c->~function();
		memset(&futureStorage, 0xFF, sizeof(decltype(futureStorage)));
		memset(&functionStorage, 0xFF, sizeof(decltype(functionStorage)));

		// Let's move the packaged task while at it.
		std::function<void()> task2 = std::move(task);
		task2();
		TS_ASSERT_EQUALS(future.Get(), 7);
	}
};
