/* Copyright (C) 2012 Wildfire Games.
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

#include "graphics/LOSTexture.h"
#include "lib/timer.h"
#include "scriptinterface/ScriptInterface.h"
#include "simulation2/Simulation2.h"

class TestLOSTexture : public CxxTest::TestSuite
{
public:
	void test_basic()
	{
		CSimulation2 sim(NULL, g_ScriptRuntime, NULL);
		CLOSTexture tex(sim);

		const ssize_t size = 8;
		u32 inputData[size*size] = {
			2, 2, 2, 0, 0, 0, 0, 0,
			2, 2, 2, 0, 0, 0, 0, 0,
			2, 2, 2, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 2
		};
		std::vector<u32> inputDataVec(inputData, inputData+size*size);

		// LOS_MASK should be cmpRanageManager->GetSharedLosMask(1),
		// but that would mean adding a huge mock component for this and it
		// should always be LOS_MASK for player 1 (as the other players are bit-shifted).
		ICmpRangeManager::CLosQuerier los(ICmpRangeManager::LOS_MASK, inputDataVec, size);

		std::vector<u8> losData;
		size_t pitch;
		losData.resize(tex.GetBitmapSize(size, size, &pitch));

		tex.GenerateBitmap(los, &losData[0], size, size, pitch);

//		for (size_t i = 0; i < losData.size(); ++i)
//			printf("%s %3d", i % (size_t)sqrt(losData.size()) ? "" : "\n", losData[i]);

		TS_ASSERT_EQUALS(losData[0], 104);
	}

	void test_perf_DISABLED()
	{
		CSimulation2 sim(NULL, ScriptInterface::CreateRuntime(), NULL);
		CLOSTexture tex(sim);

		const ssize_t size = 257;
		std::vector<u32> inputDataVec;
		inputDataVec.resize(size*size);

		// LOS_MASK should be cmpRanageManager->GetSharedLosMask(1),
		// but that would mean adding a huge mock component for this and it
		// should always be LOS_MASK for player 1 (as the other players are bit-shifted).
		ICmpRangeManager::CLosQuerier los(ICmpRangeManager::LOS_MASK, inputDataVec, size);

		size_t reps = 128;
		double t = timer_Time();
		for (size_t i = 0; i < reps; ++i)
		{
			std::vector<u8> losData;
			size_t pitch;
			losData.resize(tex.GetBitmapSize(size, size, &pitch));

			tex.GenerateBitmap(los, &losData[0], size, size, pitch);
		}
		double dt = timer_Time() - t;
		printf("\n# %f secs\n", dt/reps);
	}
};
