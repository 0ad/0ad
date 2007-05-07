#ifndef INCLUDED_SCOLOR
#define INCLUDED_SCOLOR

// SColor3ub: structure for packed RGB colors
struct SColor3ub
{
	u8 R;
	u8 G;
	u8 B;
};

// SColor4ub: structure for packed RGBA colors
struct SColor4ub
{
	u8 R;
	u8 G;
	u8 B;
	u8 A;

	SColor4ub() { }
	SColor4ub(u8 _r, u8 _g, u8 _b, u8 _a) : R(_r), G(_g), B(_b), A(_a) { }
};

#endif
