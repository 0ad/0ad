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

#include "maths/Brush.h"
#include "maths/BoundingBoxAligned.h"
#include "graphics/Frustum.h"

class TestBrush : public CxxTest::TestSuite
{
public:
	void setUp()
	{
		CxxTest::setAbortTestOnFail(true);
	}

	void tearDown()
	{

	}

	void test_slice_empty_brush()
	{
		// verifies that the result of slicing an empty bound with a plane yields an empty bound
		CBrush brush;
		CPlane plane(CVector4D(0, 0, -1, 0.5f)); // can be anything, really

		CBrush result;
		brush.Slice(plane, result);
		TS_ASSERT(brush.IsEmpty());
	}

	void test_slice_plane_simple()
	{
		// slice a 1x1x1 cube vertically down the middle at z = 0.5, with the plane normal pointing towards the negative
		// end of the Z axis (i.e., anything with Z lower than 0.5 is considered 'in front of' the plane and is kept)
		CPlane plane(CVector4D(0, 0, -1, 0.5f));
		CBrush brush(CBoundingBoxAligned(CVector3D(0,0,0), CVector3D(1,1,1)));

		CBrush result;
		brush.Slice(plane, result);

		// verify that the resulting brush consists of exactly our 8 expected, unique vertices
		TS_ASSERT_EQUALS((size_t)8, result.GetVertices().size());
		size_t LBF = GetUniqueVertexIndex(result, CVector3D(0, 0, 0)); // left-bottom-front <=> XYZ
		size_t RBF = GetUniqueVertexIndex(result, CVector3D(1, 0, 0)); // right-bottom-front
		size_t RBB = GetUniqueVertexIndex(result, CVector3D(1, 0, 0.5f)); // right-bottom-back
		size_t LBB = GetUniqueVertexIndex(result, CVector3D(0, 0, 0.5f)); // etc.
		size_t LTF = GetUniqueVertexIndex(result, CVector3D(0, 1, 0));
		size_t RTF = GetUniqueVertexIndex(result, CVector3D(1, 1, 0));
		size_t RTB = GetUniqueVertexIndex(result, CVector3D(1, 1, 0.5f));
		size_t LTB = GetUniqueVertexIndex(result, CVector3D(0, 1, 0.5f));

		// verify that the brush contains the six expected planes (one of which is the slicing plane)
		VerifyFacePresent(result, 5, LBF, RBF, RBB, LBB, LBF); // bottom face
		VerifyFacePresent(result, 5, LTF, RTF, RTB, LTB, LTF); // top face
		VerifyFacePresent(result, 5, LBF, LBB, LTB, LTF, LBF); // left face
		VerifyFacePresent(result, 5, RBF, RBB, RTB, RTF, RBF); // right face
		VerifyFacePresent(result, 5, LBF, RBF, RTF, LTF, LBF); // front face
		VerifyFacePresent(result, 5, LBB, RBB, RTB, LTB, LBB); // back face
	}

	void test_slice_plane_behind_brush()
	{
		// slice the (0,0,0) to (1,1,1) cube by the plane z = 1.5, with the plane normal pointing towards the negative
		// end of the Z axis (i.e. the entire cube is 'in front of' the plane and should be kept)
		CPlane plane(CVector4D(0, 0, -1, 1.5f));
		CBrush brush(CBoundingBoxAligned(CVector3D(0,0,0), CVector3D(1,1,1)));

		CBrush result;
		brush.Slice(plane, result);

		// verify that the resulting brush consists of exactly our 8 expected, unique vertices
		TS_ASSERT_EQUALS((size_t)8, result.GetVertices().size());
		size_t LBF = GetUniqueVertexIndex(result, CVector3D(0, 0, 0)); // left-bottom-front <=> XYZ
		size_t RBF = GetUniqueVertexIndex(result, CVector3D(1, 0, 0)); // right-bottom-front
		size_t RBB = GetUniqueVertexIndex(result, CVector3D(1, 0, 1)); // right-bottom-back
		size_t LBB = GetUniqueVertexIndex(result, CVector3D(0, 0, 1)); // etc.
		size_t LTF = GetUniqueVertexIndex(result, CVector3D(0, 1, 0));
		size_t RTF = GetUniqueVertexIndex(result, CVector3D(1, 1, 0));
		size_t RTB = GetUniqueVertexIndex(result, CVector3D(1, 1, 1));
		size_t LTB = GetUniqueVertexIndex(result, CVector3D(0, 1, 1));

		// verify that the brush contains the six expected planes (one of which is the slicing plane)
		VerifyFacePresent(result, 5, LBF, RBF, RBB, LBB, LBF); // bottom face
		VerifyFacePresent(result, 5, LTF, RTF, RTB, LTB, LTF); // top face
		VerifyFacePresent(result, 5, LBF, LBB, LTB, LTF, LBF); // left face
		VerifyFacePresent(result, 5, RBF, RBB, RTB, RTF, RBF); // right face
		VerifyFacePresent(result, 5, LBF, RBF, RTF, LTF, LBF); // front face
		VerifyFacePresent(result, 5, LBB, RBB, RTB, LTB, LBB); // back face
	}

	void test_slice_plane_in_front_of_brush()
	{
		// slices the (0,0,0) to (1,1,1) cube by the plane z = -0.5, with the plane normal pointing towards the negative
		// end of the Z axis (i.e. the entire cube is 'behind' the plane and should be cut away)
		CPlane plane(CVector4D(0, 0, -1, -0.5f));
		CBrush brush(CBoundingBoxAligned(CVector3D(0,0,0), CVector3D(1,1,1)));

		CBrush result;
		brush.Slice(plane, result);

		TS_ASSERT_EQUALS((size_t)0, result.GetVertices().size());

		std::vector<std::vector<size_t> > faces;
		result.GetFaces(faces);

		TS_ASSERT_EQUALS((size_t)0, faces.size());
	}

private:
	size_t GetUniqueVertexIndex(const CBrush& brush, const CVector3D& vertex, float eps = 1e-6f)
	{
		std::vector<CVector3D> vertices = brush.GetVertices();

		for (size_t i = 0; i < vertices.size(); ++i)
		{
			const CVector3D& v = vertices[i];
			if (fabs(v.X - vertex.X) < eps
				&& fabs(v.Y - vertex.Y) < eps
				&& fabs(v.Z - vertex.Z) < eps)
				return i;
		}

		TS_FAIL("Vertex not found in brush");
		return ~0u;
	}

	void VerifyFacePresent(const CBrush& brush, int count, ...)
	{
		std::vector<size_t> face;

		va_list args;
		va_start(args, count);
		for (int x = 0; x < count; ++x)
			face.push_back(va_arg(args, size_t));
		va_end(args);

		if (face.size() == 0)
			return;

		std::vector<std::vector<size_t> > faces;
		brush.GetFaces(faces);

		// the brush is free to use any starting vertex along the face, and to use any winding order, so have 'face'
		// cycle through various starting values and see if any of them (or their reverse) matches one found in the brush.

		for (size_t c = 0; c < face.size() - 1; ++c)
		{
			std::vector<std::vector<size_t> >::iterator it1 = std::find(faces.begin(), faces.end(), face);
			if (it1 != faces.end())
				return;

			// no match, try the reverse
			std::vector<size_t> faceReverse = face;
			std::reverse(faceReverse.begin(), faceReverse.end());
			std::vector<std::vector<size_t> >::iterator it2 = std::find(faces.begin(), faces.end(), faceReverse);
			if (it2 != faces.end())
				return;

			// no match, cycle it
			face.erase(face.begin());
			face.push_back(face[0]);
		}

		TS_FAIL("Face not found in brush");
	}
};
