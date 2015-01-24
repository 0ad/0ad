/* Copyright (C) 2010 Wildfire Games.
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

#include "simulation2/system/ComponentTest.h"

#include "simulation2/components/ICmpCommandQueue.h"

class TestCmpCommandQueue : public CxxTest::TestSuite
{
public:
	void setUp()
	{
		CXeromyces::Startup();
	}

	void tearDown()
	{
		CXeromyces::Terminate();
	}

	void test_basic()
	{
		ComponentTestHelper test(g_ScriptRuntime);
		JSContext* cx = test.GetScriptInterface().GetContext();
		JSAutoRequest rq(cx);

		std::vector<SimulationCommand> empty;

		ICmpCommandQueue* cmp = test.Add<ICmpCommandQueue>(CID_CommandQueue, "");

		TS_ASSERT(test.GetScriptInterface().Eval("var cmds = []; function ProcessCommand(player, cmd) { cmds.push([player, cmd]); }"));

		JS::RootedValue cmd(cx);

		TS_ASSERT(test.GetScriptInterface().Eval("([1,2,3])", &cmd));
		cmp->PushLocalCommand(1, cmd);

		TS_ASSERT(test.GetScriptInterface().Eval("({x:4})", &cmd));
		cmp->PushLocalCommand(-1, cmd);

		test.Roundtrip();

		// Process the first two commands
		cmp->FlushTurn(empty);

		TS_ASSERT(test.GetScriptInterface().Eval("({y:5})", &cmd));
		cmp->PushLocalCommand(10, cmd);

		// Process the next command
		cmp->FlushTurn(empty);

		// Process no commands
		cmp->FlushTurn(empty);

		test.Roundtrip();

		std::string output;
		TS_ASSERT(test.GetScriptInterface().Eval("uneval(cmds)", output));
		TS_ASSERT_STR_EQUALS(output, "[[1, [1, 2, 3]], [-1, {x:4}], [10, {y:5}]]");
	}
};
