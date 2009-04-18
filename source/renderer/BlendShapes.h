/* Copyright (C) 2009 Wildfire Games.
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

/*

 */

#ifndef INCLUDED_BLENDSHAPES
#define INCLUDED_BLENDSHAPES

struct BlendShape4
{
public:
	BlendShape4() {}
	BlendShape4(int a,int b,int c,int d) {
		m_Data[0]=a; m_Data[1]=b; m_Data[2]=c; m_Data[3]=d;
	}

	int& operator[](int index) { return m_Data[index]; }
	const int& operator[](int index) const { return m_Data[index]; }

	bool operator==(const BlendShape4& lhs) const {
		return memcmp(m_Data,lhs.m_Data,sizeof(BlendShape4))==0;
	}

	void Rotate90(BlendShape4& dst) const {
		dst[0]=m_Data[3];
		dst[1]=m_Data[0];
		dst[2]=m_Data[1];
		dst[3]=m_Data[2];
	}

	void Rotate180(BlendShape4& dst) const {
		dst[0]=m_Data[2];
		dst[1]=m_Data[3];
		dst[2]=m_Data[0];
		dst[3]=m_Data[1];
	}

	void Rotate270(BlendShape4& dst) const {
		dst[0]=m_Data[1];
		dst[1]=m_Data[2];
		dst[2]=m_Data[3];
		dst[3]=m_Data[0];
	}

	void FlipU(BlendShape4& dst) const {
		dst[0]=m_Data[2];
		dst[1]=m_Data[1];
		dst[2]=m_Data[0];
		dst[3]=m_Data[3];
	}

	void FlipV(BlendShape4& dst) const {
		dst[0]=m_Data[0];
		dst[1]=m_Data[3];
		dst[2]=m_Data[2];
		dst[3]=m_Data[1];
	}

private:
	int m_Data[4];
};


struct BlendShape8
{
public:
	BlendShape8() {}
	BlendShape8(int a,int b,int c,int d,int e,int f,int g,int h) {
		m_Data[0]=a; m_Data[1]=b; m_Data[2]=c; m_Data[3]=d;
		m_Data[4]=e; m_Data[5]=f; m_Data[6]=g; m_Data[7]=h;
	}

	int& operator[](size_t index) { return m_Data[index]; }
	const int& operator[](size_t index) const { return m_Data[index]; }

	bool operator==(const BlendShape8& lhs) const {
		return memcmp(m_Data,lhs.m_Data,sizeof(BlendShape8))==0;
	}

	void Rotate90(BlendShape8& dst) const {
		dst[0]=m_Data[6];
		dst[1]=m_Data[7];
		dst[2]=m_Data[0];
		dst[3]=m_Data[1];
		dst[4]=m_Data[2];
		dst[5]=m_Data[3];
		dst[6]=m_Data[4];
		dst[7]=m_Data[5];
	}

	void Rotate180(BlendShape8& dst) const {
		dst[0]=m_Data[4];
		dst[1]=m_Data[5];
		dst[2]=m_Data[6];
		dst[3]=m_Data[7];
		dst[4]=m_Data[0];
		dst[5]=m_Data[1];
		dst[6]=m_Data[2];
		dst[7]=m_Data[3];
	}

	void Rotate270(BlendShape8& dst) const {
		dst[0]=m_Data[2];
		dst[1]=m_Data[3];
		dst[2]=m_Data[4];
		dst[3]=m_Data[5];
		dst[4]=m_Data[6];
		dst[5]=m_Data[7];
		dst[6]=m_Data[0];
		dst[7]=m_Data[1];
	}

	void FlipU(BlendShape8& dst) const {
		dst[0]=m_Data[4];
		dst[1]=m_Data[3];
		dst[2]=m_Data[2];
		dst[3]=m_Data[1];
		dst[4]=m_Data[0];
		dst[5]=m_Data[7];
		dst[6]=m_Data[6];
		dst[7]=m_Data[5];
	}

	void FlipV(BlendShape8& dst) const {
		dst[0]=m_Data[0];
		dst[1]=m_Data[7];
		dst[2]=m_Data[6];
		dst[3]=m_Data[5];
		dst[4]=m_Data[4];
		dst[5]=m_Data[3];
		dst[6]=m_Data[2];
		dst[7]=m_Data[1];
	}

private:
	int m_Data[8];
};

#endif
