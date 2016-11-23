/* Copyright (C) 2013 Wildfire Games.
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

#include "graphics/ShaderManager.h"

#include "maths/Vector4D.h"

class TestShaderManager : public CxxTest::TestSuite
{
public:
	void test_defines()
	{
		CShaderDefines defines1;
		CShaderDefines defines2;
		CShaderDefines defines3;
		TS_ASSERT(defines1 == defines2);
		TS_ASSERT(defines1.GetHash() == defines2.GetHash());
		TS_ASSERT(!(defines1 < defines2 || defines2 < defines1));

		defines1.Add(CStrIntern("FOO"), CStrIntern("1"));

		TS_ASSERT_EQUALS(defines1.GetInt("FOO"), 1);
		TS_ASSERT_EQUALS(defines2.GetInt("FOO"), 0);

		TS_ASSERT(!(defines1 == defines2));
		TS_ASSERT(!(defines1.GetHash() == defines2.GetHash()));
		TS_ASSERT(defines1 < defines2 || defines2 < defines1);

		defines2.Add(CStrIntern("FOO"), CStrIntern("2"));

		TS_ASSERT_EQUALS(defines1.GetInt("FOO"), 1);
		TS_ASSERT_EQUALS(defines2.GetInt("FOO"), 2);

		TS_ASSERT(!(defines1 == defines2));
		TS_ASSERT(!(defines1.GetHash() == defines2.GetHash()));

		defines3 = defines2;
		defines2.Add(CStrIntern("FOO"), CStrIntern("1")); // override old value

		TS_ASSERT_EQUALS(defines1.GetInt("FOO"), 1);
		TS_ASSERT_EQUALS(defines2.GetInt("FOO"), 1);
		TS_ASSERT_EQUALS(defines3.GetInt("FOO"), 2);

		TS_ASSERT(defines1 == defines2);
		TS_ASSERT(defines1.GetHash() == defines2.GetHash());
	}

	void test_defines_order()
	{
		CShaderDefines defines1;
		defines1.Add(CStrIntern("A"), CStrIntern("1"));
		defines1.Add(CStrIntern("B"), CStrIntern("1"));
		defines1.Add(CStrIntern("C"), CStrIntern("1"));
		defines1.Add(CStrIntern("D"), CStrIntern("1"));

		CShaderDefines defines2;
		defines2.Add(CStrIntern("C"), CStrIntern("2"));
		defines2.Add(CStrIntern("C"), CStrIntern("1"));
		defines2.Add(CStrIntern("B"), CStrIntern("2"));
		defines2.Add(CStrIntern("B"), CStrIntern("1"));
		defines2.Add(CStrIntern("D"), CStrIntern("2"));
		defines2.Add(CStrIntern("D"), CStrIntern("1"));
		defines2.Add(CStrIntern("A"), CStrIntern("2"));
		defines2.Add(CStrIntern("A"), CStrIntern("1"));

		TS_ASSERT(defines1 == defines2);

		defines2.SetMany(defines1);
		TS_ASSERT(defines1 == defines2);

		CShaderDefines defines3;
		defines3.SetMany(defines1);
		TS_ASSERT(defines1 == defines3);
	}

	void test_uniforms()
	{
		CShaderUniforms uniforms1;
		CShaderUniforms uniforms2;
		TS_ASSERT(uniforms1 == uniforms2);

		uniforms1.Add("FOO", CVector4D(1.0f, 0.0f, 0.0f, 0.0f));

		TS_ASSERT_EQUALS(uniforms1.GetVector("FOO"), CVector4D(1.0f, 0.0f, 0.0f, 0.0f));
		TS_ASSERT_EQUALS(uniforms2.GetVector("FOO"), CVector4D(0.0f, 0.0f, 0.0f, 0.0f));

		TS_ASSERT(!(uniforms1 == uniforms2));

		uniforms2.Add("FOO", CVector4D(0.0f, 1.0f, 0.0f, 0.0f));

		TS_ASSERT(!(uniforms1 == uniforms2));

		uniforms2.Add("FOO", CVector4D(1.0f, 0.0f, 0.0f, 0.0f)); // override old value

		TS_ASSERT(uniforms1 == uniforms2);
	}
};
