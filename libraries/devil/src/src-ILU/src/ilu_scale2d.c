//-----------------------------------------------------------------------------
//
// ImageLib Utility Sources
// Copyright (C) 2000-2002 by Denton Woods
// Last modified: 05/25/2001 <--Y2K Compliant! =]
//
// Filename: src-ILU/src/ilu_scale2d.c
//
// Description: Scales an image.
//
//-----------------------------------------------------------------------------


// NOTE:  Don't look at this file if you wish to preserve your sanity!


#include "ilu_internal.h"
#include "ilu_states.h"


ILimage *iluScale2DNear_(ILimage *Image, ILimage *Scaled, ILuint Width, ILuint Height);
ILimage *iluScale2DLinear_(ILimage *Image, ILimage *Scaled, ILuint Width, ILuint Height);
ILimage *iluScale2DBilinear_(ILimage *Image, ILimage *Scaled, ILuint Width, ILuint Height);

static ILuint		x1, x2;
static ILuint		NewY1, NewY2, NewX1, NewX2, Size, x, y, c;
static ILdouble	ScaleX, ScaleY, t1, t2, t3, t4, f, ft, NewX;
static ILdouble	Table[2][4];  // Assumes we don't have larger than 32-bit images.
static ILuint		ImgBps, SclBps;
static ILushort	*ShortPtr, *SShortPtr;
static ILuint		*IntPtr, *SIntPtr;
static ILfloat		*FloatPtr, *SFloatPtr;


ILimage *iluScale2D_(ILimage *Image, ILimage *Scaled, ILuint Width, ILuint Height)
{
	if (Image == NULL) {
		ilSetError(ILU_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	ScaleX = (ILfloat)Width / Image->Width;
	ScaleY = (ILfloat)Height / Image->Height;

	if (iluFilter == ILU_NEAREST)
		return iluScale2DNear_(Image, Scaled, Width, Height);
	else if (iluFilter == ILU_LINEAR)
		return iluScale2DLinear_(Image, Scaled, Width, Height);
	// iluFilter == ILU_BILINEAR
	return iluScale2DBilinear_(Image, Scaled, Width, Height);
}


ILimage *iluScale2DNear_(ILimage *Image, ILimage *Scaled, ILuint Width, ILuint Height)
{
	ImgBps = Image->Bps / Image->Bpc;
	SclBps = Scaled->Bps / Scaled->Bpc;

	switch (Image->Bpc)
	{
		case 1:
			for (y = 0; y < Height; y++) {
				NewY1 = y * SclBps;
				NewY2 = (ILuint)(y / ScaleY) * ImgBps;
				for (x = 0; x < Width; x++) {
					NewX1 = x * Scaled->Bpp;
					NewX2 = (ILuint)(x / ScaleX) * Image->Bpp;
					for (c = 0; c < Scaled->Bpp; c++) {
						Scaled->Data[NewY1 + NewX1 + c] = Image->Data[NewY2 + NewX2 + c];
						x1 = 0;
					}
				}
			}
			break;

		case 2:
			ShortPtr = (ILushort*)Image->Data;
			SShortPtr = (ILushort*)Scaled->Data;
			for (y = 0; y < Height; y++) {
				NewY1 = y * SclBps;
				NewY2 = (ILuint)(y / ScaleY) * ImgBps;
				for (x = 0; x < Width; x++) {
					NewX1 = x * Scaled->Bpp;
					NewX2 = (ILuint)(x / ScaleX) * Image->Bpp;
					for (c = 0; c < Scaled->Bpp; c++) {
						SShortPtr[NewY1 + NewX1 + c] = ShortPtr[NewY2 + NewX2 + c];
						x1 = 0;
					}
				}
			}
			break;

		case 4:
			IntPtr = (ILuint*)Image->Data;
			SIntPtr = (ILuint*)Scaled->Data;
			for (y = 0; y < Height; y++) {
				NewY1 = y * SclBps;
				NewY2 = (ILuint)(y / ScaleY) * ImgBps;
				for (x = 0; x < Width; x++) {
					NewX1 = x * Scaled->Bpp;
					NewX2 = (ILuint)(x / ScaleX) * Image->Bpp;
					for (c = 0; c < Scaled->Bpp; c++) {
						SIntPtr[NewY1 + NewX1 + c] = IntPtr[NewY2 + NewX2 + c];
						x1 = 0;
					}
				}
			}
			break;
	}

	return Scaled;
}


ILimage *iluScale2DLinear_(ILimage *Image, ILimage *Scaled, ILuint Width, ILuint Height)
{
	ImgBps = Image->Bps / Image->Bpc;
	SclBps = Scaled->Bps / Scaled->Bpc;

	switch (Image->Bpc)
	{
		case 1:
			for (y = 0; y < Height; y++) {
				NewY1 = (ILuint)(y / ScaleY) * ImgBps;
				for (x = 0; x < Width; x++) {
					t1 = x / (ILdouble)Width;
					t4 = t1 * Width;
					t2 = t4 - (ILuint)(t4);
					ft = t2 * IL_PI;
					f = (1.0 - cos(ft)) * .5;
					NewX1 = ((ILuint)(t4 / ScaleX)) * Image->Bpp;
					NewX2 = ((ILuint)(t4 / ScaleX) + 1) * Image->Bpp;

					Size = y * SclBps + x * Scaled->Bpp;
					for (c = 0; c < Scaled->Bpp; c++) {
						x1 = Image->Data[NewY1 + NewX1 + c];
						x2 = Image->Data[NewY1 + NewX2 + c];
						Scaled->Data[Size + c] = (ILubyte)((1.0 - f) * x1 + f * x2);
					}
				}
			}
			break;

		case 2:
			ShortPtr = (ILushort*)Image->Data;
			SShortPtr = (ILushort*)Scaled->Data;
			for (y = 0; y < Height; y++) {
				NewY1 = (ILuint)(y / ScaleY) * ImgBps;
				for (x = 0; x < Width; x++) {
					t1 = x / (ILdouble)Width;
					t4 = t1 * Width;
					t2 = t4 - (ILuint)(t4);
					ft = t2 * IL_PI;
					f = (1.0 - cos(ft)) * .5;
					NewX1 = ((ILuint)(t4 / ScaleX)) * Image->Bpp;
					NewX2 = ((ILuint)(t4 / ScaleX) + 1) * Image->Bpp;

					Size = y * SclBps + x * Scaled->Bpp;
					for (c = 0; c < Scaled->Bpp; c++) {
						x1 = ShortPtr[NewY1 + NewX1 + c];
						x2 = ShortPtr[NewY1 + NewX2 + c];
						SShortPtr[Size + c] = (ILushort)((1.0 - f) * x1 + f * x2);
					}
				}
			}
			break;

		case 4:
			IntPtr = (ILuint*)Image->Data;
			SIntPtr = (ILuint*)Scaled->Data;
			for (y = 0; y < Height; y++) {
				NewY1 = (ILuint)(y / ScaleY) * ImgBps;
				for (x = 0; x < Width; x++) {
					t1 = x / (ILdouble)Width;
					t4 = t1 * Width;
					t2 = t4 - (ILuint)(t4);
					ft = t2 * IL_PI;
					f = (1.0 - cos(ft)) * .5;
					NewX1 = ((ILuint)(t4 / ScaleX)) * Image->Bpp;
					NewX2 = ((ILuint)(t4 / ScaleX) + 1) * Image->Bpp;

					Size = y * SclBps + x * Scaled->Bpp;
					for (c = 0; c < Scaled->Bpp; c++) {
						x1 = IntPtr[NewY1 + NewX1 + c];
						x2 = IntPtr[NewY1 + NewX2 + c];
						SIntPtr[Size + c] = (ILuint)((1.0 - f) * x1 + f * x2);
					}
				}
			}
			break;
	}

	return Scaled;
}


ILimage *iluScale2DBilinear_(ILimage *Image, ILimage *Scaled, ILuint Width, ILuint Height)
{
	ImgBps = Image->Bps / Image->Bpc;
	SclBps = Scaled->Bps / Scaled->Bpc;

	switch (Image->Bpc)
	{
		case 1:
			Height--;  // Only use regular Height once in the following loop.
			for (y = 0; y < Height; y++) {
				NewY1 = (ILuint)(y / ScaleY) * ImgBps;
				NewY2 = (ILuint)((y+1) / ScaleY) * ImgBps;
				for (x = 0; x < Width; x++) {
					NewX = Width / ScaleX;
					t1 = x / (ILdouble)Width;
					t4 = t1 * Width;
					t2 = t4 - (ILuint)(t4);
					t3 = (1.0 - t2);
					t4 = t1 * NewX;
					NewX1 = (ILuint)(t4) * Image->Bpp;
					NewX2 = (ILuint)(t4 + 1) * Image->Bpp;

					for (c = 0; c < Scaled->Bpp; c++) {
						Table[0][c] = t3 * Image->Data[NewY1 + NewX1 + c] +
							t2 * Image->Data[NewY1 + NewX2 + c];

						Table[1][c] = t3 * Image->Data[NewY2 + NewX1 + c] +
							t2 * Image->Data[NewY2 + NewX2 + c];
					}

					// Linearly interpolate between the table values.
					t1 = y / (ILdouble)(Height + 1);  // Height+1 is the real height now.
					t3 = (1.0 - t1);
					Size = y * SclBps + x * Scaled->Bpp;
					for (c = 0; c < Scaled->Bpp; c++) {
						Scaled->Data[Size + c] =
							(ILubyte)(t3 * Table[0][c] + t1 * Table[1][c]);
					}
				}
			}

			// Calculate the last row.
			NewY1 = (ILuint)(Height / ScaleY) * ImgBps;
			for (x = 0; x < Width; x++) {
				NewX = Width / ScaleX;
				t1 = x / (ILdouble)Width;
				t4 = t1 * Width;
				ft = (t4 - (ILuint)(t4)) * IL_PI;
				f = (1.0 - cos(ft)) * .5;  // Cosine interpolation
				NewX1 = (ILuint)(t1 * NewX) * Image->Bpp;
				NewX2 = (ILuint)(t1 * NewX + 1) * Image->Bpp;

				Size = Height * SclBps + x * Image->Bpp;
				for (c = 0; c < Scaled->Bpp; c++) {
					Scaled->Data[Size + c] = (ILubyte)((1.0 - f) * Image->Data[NewY1 + NewX1 + c] +
						f * Image->Data[NewY1 + NewX2 + c]);
				}
			}
			break;

		case 2:
			ShortPtr = (ILushort*)Image->Data;
			SShortPtr = (ILushort*)Scaled->Data;
			Height--;  // Only use regular Height once in the following loop.
			for (y = 0; y < Height; y++) {
				NewY1 = (ILuint)(y / ScaleY) * ImgBps;
				NewY2 = (ILuint)((y+1) / ScaleY) * ImgBps;
				for (x = 0; x < Width; x++) {
					NewX = Width / ScaleX;
					t1 = x / (ILdouble)Width;
					t4 = t1 * Width;
					t2 = t4 - (ILuint)(t4);
					t3 = (1.0 - t2);
					t4 = t1 * NewX;
					NewX1 = (ILuint)(t4) * Image->Bpp;
					NewX2 = (ILuint)(t4 + 1) * Image->Bpp;

					for (c = 0; c < Scaled->Bpp; c++) {
						Table[0][c] = t3 * ShortPtr[NewY1 + NewX1 + c] +
							t2 * ShortPtr[NewY1 + NewX2 + c];

						Table[1][c] = t3 * ShortPtr[NewY2 + NewX1 + c] +
							t2 * ShortPtr[NewY2 + NewX2 + c];
					}

					// Linearly interpolate between the table values.
					t1 = y / (ILdouble)(Height + 1);  // Height+1 is the real height now.
					t3 = (1.0 - t1);
					Size = y * SclBps + x * Scaled->Bpp;
					for (c = 0; c < Scaled->Bpp; c++) {
						SShortPtr[Size + c] =
							(ILushort)(t3 * Table[0][c] + t1 * Table[1][c]);
					}
				}
			}

			// Calculate the last row.
			NewY1 = (ILuint)(Height / ScaleY) * ImgBps;
			for (x = 0; x < Width; x++) {
				NewX = Width / ScaleX;
				t1 = x / (ILdouble)Width;
				t4 = t1 * Width;
				ft = (t4 - (ILuint)(t4)) * IL_PI;
				f = (1.0 - cos(ft)) * .5;  // Cosine interpolation
				NewX1 = (ILuint)(t1 * NewX) * Image->Bpp;
				NewX2 = (ILuint)(t1 * NewX + 1) * Image->Bpp;

				Size = Height * SclBps + x * Image->Bpp;
				for (c = 0; c < Scaled->Bpp; c++) {
					SShortPtr[Size + c] = (ILushort)((1.0 - f) * ShortPtr[NewY1 + NewX1 + c] +
						f * ShortPtr[NewY1 + NewX2 + c]);
				}
			}
			break;

		case 4:
			if (Image->Type != IL_FLOAT) {
				IntPtr = (ILuint*)Image->Data;
				SIntPtr = (ILuint*)Scaled->Data;
				Height--;  // Only use regular Height once in the following loop.
				for (y = 0; y < Height; y++) {
					NewY1 = (ILuint)(y / ScaleY) * ImgBps;
					NewY2 = (ILuint)((y+1) / ScaleY) * ImgBps;
					for (x = 0; x < Width; x++) {
						NewX = Width / ScaleX;
						t1 = x / (ILdouble)Width;
						t4 = t1 * Width;
						t2 = t4 - (ILuint)(t4);
						t3 = (1.0 - t2);
						t4 = t1 * NewX;
						NewX1 = (ILuint)(t4) * Image->Bpp;
						NewX2 = (ILuint)(t4 + 1) * Image->Bpp;

						for (c = 0; c < Scaled->Bpp; c++) {
							Table[0][c] = t3 * IntPtr[NewY1 + NewX1 + c] +
								t2 * IntPtr[NewY1 + NewX2 + c];

							Table[1][c] = t3 * IntPtr[NewY2 + NewX1 + c] +
								t2 * IntPtr[NewY2 + NewX2 + c];
						}

						// Linearly interpolate between the table values.
						t1 = y / (ILdouble)(Height + 1);  // Height+1 is the real height now.
						t3 = (1.0 - t1);
						Size = y * SclBps + x * Scaled->Bpp;
						for (c = 0; c < Scaled->Bpp; c++) {
							SIntPtr[Size + c] =
								(ILuint)(t3 * Table[0][c] + t1 * Table[1][c]);
						}
					}
				}

				// Calculate the last row.
				NewY1 = (ILuint)(Height / ScaleY) * ImgBps;
				for (x = 0; x < Width; x++) {
					NewX = Width / ScaleX;
					t1 = x / (ILdouble)Width;
					t4 = t1 * Width;
					ft = (t4 - (ILuint)(t4)) * IL_PI;
					f = (1.0 - cos(ft)) * .5;  // Cosine interpolation
					NewX1 = (ILuint)(t1 * NewX) * Image->Bpp;
					NewX2 = (ILuint)(t1 * NewX + 1) * Image->Bpp;

					Size = Height * SclBps + x * Image->Bpp;
					for (c = 0; c < Scaled->Bpp; c++) {
						SIntPtr[Size + c] = (ILuint)((1.0 - f) * IntPtr[NewY1 + NewX1 + c] +
							f * IntPtr[NewY1 + NewX2 + c]);
					}
				}
			}
			else {  // IL_FLOAT
				FloatPtr = (ILfloat*)Image->Data;
				SFloatPtr = (ILfloat*)Scaled->Data;
				Height--;  // Only use regular Height once in the following loop.
				for (y = 0; y < Height; y++) {
					NewY1 = (ILuint)(y / ScaleY) * ImgBps;
					NewY2 = (ILuint)((y+1) / ScaleY) * ImgBps;
					for (x = 0; x < Width; x++) {
						NewX = Width / ScaleX;
						t1 = x / (ILdouble)Width;
						t4 = t1 * Width;
						t2 = t4 - (ILuint)(t4);
						t3 = (1.0 - t2);
						t4 = t1 * NewX;
						NewX1 = (ILuint)(t4) * Image->Bpp;
						NewX2 = (ILuint)(t4 + 1) * Image->Bpp;

						for (c = 0; c < Scaled->Bpp; c++) {
							Table[0][c] = t3 * FloatPtr[NewY1 + NewX1 + c] +
								t2 * FloatPtr[NewY1 + NewX2 + c];

							Table[1][c] = t3 * FloatPtr[NewY2 + NewX1 + c] +
								t2 * FloatPtr[NewY2 + NewX2 + c];
						}

						// Linearly interpolate between the table values.
						t1 = y / (ILdouble)(Height + 1);  // Height+1 is the real height now.
						t3 = (1.0 - t1);
						Size = y * SclBps + x * Scaled->Bpp;
						for (c = 0; c < Scaled->Bpp; c++) {
							SFloatPtr[Size + c] =
								(ILfloat)(t3 * Table[0][c] + t1 * Table[1][c]);
						}
					}
				}

				// Calculate the last row.
				NewY1 = (ILuint)(Height / ScaleY) * ImgBps;
				for (x = 0; x < Width; x++) {
					NewX = Width / ScaleX;
					t1 = x / (ILdouble)Width;
					t4 = t1 * Width;
					ft = (t4 - (ILuint)(t4)) * IL_PI;
					f = (1.0 - cos(ft)) * .5;  // Cosine interpolation
					NewX1 = (ILuint)(t1 * NewX) * Image->Bpp;
					NewX2 = (ILuint)(t1 * NewX + 1) * Image->Bpp;

					Size = Height * SclBps + x * Image->Bpp;
					for (c = 0; c < Scaled->Bpp; c++) {
						SFloatPtr[Size + c] = (ILfloat)((1.0 - f) * FloatPtr[NewY1 + NewX1 + c] +
							f * FloatPtr[NewY1 + NewX2 + c]);
					}
				}
			}
			break;
	}

	return Scaled;
}
