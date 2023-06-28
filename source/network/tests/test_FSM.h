/* Copyright (C) 2023 Wildfire Games.
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

#include "network/FSM.h"

#include <array>

class TestFSM : public CxxTest::TestSuite
{
	struct FSMGlobalState
	{
		std::array<size_t, 3> occurCount{0, 0, 0};
	};

	template<size_t N>
	static bool editGlobal(void* state, const CFsmEvent*)
	{
		++std::get<N>(reinterpret_cast<FSMGlobalState*>(state)->occurCount);
		return true;
	}

	static bool editParam(void*, CFsmEvent* event)
	{
		++*reinterpret_cast<size_t*>(event->GetParamRef());
		return true;
	}

	enum class State : unsigned int
	{
		zero,
		one,
		two
	};

	enum class Instruction : unsigned int
	{
		toZero,
		toOne,
		toTwo
	};

public:

	void test_global()
	{
		FSMGlobalState globalState;
		CFsm FSMObject;

		/*
		Corresponding pseudocode

		while (true)
		{
			// state zero
			await nextInstruction();

			// state one
			const auto cond = await nextInstruction();

			if (cond == instruction::toOne)
			{
				//state two
				await nextInstruction();
			}
		}
		*/

		FSMObject.AddTransition(static_cast<unsigned int>(State::zero),
			static_cast<unsigned int>(Instruction::toOne), static_cast<unsigned int>(State::one),
			reinterpret_cast<void*>(&editGlobal<1>), static_cast<void*>(&globalState));
		FSMObject.AddTransition(static_cast<unsigned int>(State::one),
			static_cast<unsigned int>(Instruction::toTwo), static_cast<unsigned int>(State::two),
			reinterpret_cast<void*>(&editGlobal<2>), static_cast<void*>(&globalState));
		FSMObject.AddTransition(static_cast<unsigned int>(State::one),
			static_cast<unsigned int>(Instruction::toZero), static_cast<unsigned int>(State::zero),
			reinterpret_cast<void*>(&editGlobal<0>), static_cast<void*>(&globalState));
		FSMObject.AddTransition(static_cast<unsigned int>(State::two),
			static_cast<unsigned int>(Instruction::toZero), static_cast<unsigned int>(State::zero),
			reinterpret_cast<void*>(&editGlobal<0>), static_cast<void*>(&globalState));

		FSMObject.SetFirstState(static_cast<unsigned int>(State::zero));

		TS_ASSERT(FSMObject.Update(static_cast<unsigned int>(Instruction::toOne), nullptr));
		TS_ASSERT_EQUALS(std::get<1>(globalState.occurCount), 1);
		TS_ASSERT_EQUALS(FSMObject.GetCurrState(), static_cast<unsigned int>(State::one));

		TS_ASSERT(FSMObject.Update(static_cast<unsigned int>(Instruction::toTwo), nullptr));
		TS_ASSERT(FSMObject.Update(static_cast<unsigned int>(Instruction::toZero), nullptr));
		TS_ASSERT(FSMObject.Update(static_cast<unsigned int>(Instruction::toOne), nullptr));
		TS_ASSERT(FSMObject.Update(static_cast<unsigned int>(Instruction::toZero), nullptr));
		TS_ASSERT(FSMObject.Update(static_cast<unsigned int>(Instruction::toOne), nullptr));
		TS_ASSERT(FSMObject.Update(static_cast<unsigned int>(Instruction::toZero), nullptr));
		TS_ASSERT_EQUALS(std::get<0>(globalState.occurCount), 3);
		TS_ASSERT_EQUALS(std::get<1>(globalState.occurCount), 3);
		TS_ASSERT_EQUALS(std::get<2>(globalState.occurCount), 1);

		// Some transitions do not exist.
		TS_ASSERT(!FSMObject.Update(static_cast<unsigned int>(Instruction::toZero), nullptr));
		TS_ASSERT(!FSMObject.Update(static_cast<unsigned int>(Instruction::toTwo), nullptr));
		TS_ASSERT(FSMObject.Update(static_cast<unsigned int>(Instruction::toOne), nullptr));
		TS_ASSERT(!FSMObject.Update(static_cast<unsigned int>(Instruction::toOne), nullptr));
		TS_ASSERT(FSMObject.Update(static_cast<unsigned int>(Instruction::toTwo), nullptr));
		TS_ASSERT(!FSMObject.Update(static_cast<unsigned int>(Instruction::toTwo), nullptr));
		TS_ASSERT_EQUALS(std::get<0>(globalState.occurCount), 3);
		TS_ASSERT_EQUALS(std::get<1>(globalState.occurCount), 4);
		TS_ASSERT_EQUALS(std::get<2>(globalState.occurCount), 2);
	}

	void test_param()
	{
		FSMGlobalState globalState;
		CFsm FSMObject;

		// Equal to the FSM in test_global.
		FSMObject.AddTransition(static_cast<unsigned int>(State::zero),
			static_cast<unsigned int>(Instruction::toOne), static_cast<unsigned int>(State::one),
			reinterpret_cast<void*>(&editParam), nullptr);
		FSMObject.AddTransition(static_cast<unsigned int>(State::one),
			static_cast<unsigned int>(Instruction::toTwo), static_cast<unsigned int>(State::two),
			reinterpret_cast<void*>(&editParam), nullptr);
		FSMObject.AddTransition(static_cast<unsigned int>(State::one),
			static_cast<unsigned int>(Instruction::toZero), static_cast<unsigned int>(State::zero),
			reinterpret_cast<void*>(&editParam), nullptr);
		FSMObject.AddTransition(static_cast<unsigned int>(State::two),
			static_cast<unsigned int>(Instruction::toZero), static_cast<unsigned int>(State::zero),
			reinterpret_cast<void*>(&editParam), nullptr);

		FSMObject.SetFirstState(static_cast<unsigned int>(State::zero));

		// Some transitions do not exist.
		TS_ASSERT(!FSMObject.Update(static_cast<unsigned int>(Instruction::toZero),
			static_cast<void*>(&std::get<0>(globalState.occurCount))));
		TS_ASSERT(!FSMObject.Update(static_cast<unsigned int>(Instruction::toTwo),
			static_cast<void*>(&std::get<2>(globalState.occurCount))));
		TS_ASSERT(FSMObject.Update(static_cast<unsigned int>(Instruction::toOne),
			static_cast<void*>(&std::get<1>(globalState.occurCount))));
		TS_ASSERT(!FSMObject.Update(static_cast<unsigned int>(Instruction::toOne),
			static_cast<void*>(&std::get<1>(globalState.occurCount))));
		TS_ASSERT(FSMObject.Update(static_cast<unsigned int>(Instruction::toTwo),
			static_cast<void*>(&std::get<2>(globalState.occurCount))));
		TS_ASSERT(!FSMObject.Update(static_cast<unsigned int>(Instruction::toTwo),
			static_cast<void*>(&std::get<2>(globalState.occurCount))));
		TS_ASSERT_EQUALS(std::get<0>(globalState.occurCount), 0);
		TS_ASSERT_EQUALS(std::get<1>(globalState.occurCount), 1);
		TS_ASSERT_EQUALS(std::get<2>(globalState.occurCount), 1);
	}
};
