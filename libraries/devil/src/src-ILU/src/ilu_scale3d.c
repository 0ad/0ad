//-----------------------------------------------------------------------------
//
// ImageLib Utility Sources
// Copyright (C) 2000-2002 by Denton Woods
// Last modified: 05/25/2001 <--Y2K Compliant! =]
//
// Filename: src-ILU/src/ilu_scale.c
//
// Description: Scales an image.
//
//-----------------------------------------------------------------------------


// NOTE:  Don't look at this file if you wish to preserve your sanity!


#include "ilu_internal.h"
#include "ilu_states.h"


ILimage *iluScale3DNear_(ILimage *Image, ILimage *Scaled, ILuint Width, ILuint Height, ILuint Depth);
ILimage *iluScale3DLinear_(ILimage *Image, ILimage *Scaled, ILuint Width, ILuint Height, ILuint Depth);
ILimage *iluScale3DBilinear_(ILimage *Image, ILimage *Scaled, ILuint Width, ILuint Height, ILuint Depth);

static ILuint		Size, NewX1, NewX2, NewY1, NewY2, NewZ1, NewZ2, x, y, z, c;
static ILdouble	ScaleX, ScaleY, ScaleZ, x1, x2, t1, t2, t4, f, ft;
//ILdouble	Table[2][2][4];  // Assumes we don't have larger than 32-bit images.
static ILuint		ImgBps, SclBps, ImgPlane, SclPlane;
static ILushort	*ShortPtr, *SShortPtr;
static ILuint		*IntPtr, *SIntPtr;


ILimage *iluScale3D_(ILimage *Image, ILimage *Scaled, ILuint Width, ILuint Height, ILuint Depth)
{
	if (Image == NULL) {
		ilSetError(ILU_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	ScaleX = (ILfloat)Width / Image->Width;
	ScaleY = (ILfloat)Height / Image->Height;
	ScaleZ = (ILfloat)Depth / Image->Depth;

	//if (iluFilter == ILU_NEAREST)
		return iluScale3DNear_(Image, Scaled, Width, Height, Depth);
	//else if (iluFilter == ILU_LINEAR)
		//return iluScale3DLinear_(Image, Scaled, Width, Height, Depth);
	
	// iluFilter == ILU_BILINEAR
	//return iluScale3DBilinear_(Image, Scaled, Width, Height, Depth);
}


ILimage *iluScale3DNear_(ILimage *Image, ILimage *Scaled, ILuint Width, ILuint Height, ILuint Depth)
{
	ImgBps = Image->Bps / Image->Bpc;
	SclBps = Scaled->Bps / Scaled->Bpc;
	ImgPlane = Image->SizeOfPlane / Image->Bpc;
	SclPlane = Scaled->SizeOfPlane / Scaled->Bpc;

	switch (Image->Bpc)
	{
		case 1:
			for (z = 0; z < Depth; z++) {
				NewZ1 = z * SclPlane;
				NewZ2 = (ILuint)(z / ScaleZ) * ImgPlane;
				for (y = 0; y < Height; y++) {
					NewY1 = y * SclBps;
					NewY2 = (ILuint)(y / ScaleY) * ImgBps;
					for (x = 0; x < Width; x++) {
						NewX1 = x * Scaled->Bpp;
						NewX2 = (ILuint)(x / ScaleX) * Image->Bpp;
						for (c = 0; c < Scaled->Bpp; c++) {
							Scaled->Data[NewZ1 + NewY1 + NewX1 + c] =
								Image->Data[NewZ2 + NewY2 + NewX2 + c];
						}
					}
				}
			}
			break;

		case 2:
			ShortPtr = (ILushort*)Image->Data;
			SShortPtr = (ILushort*)Scaled->Data;
			for (z = 0; z < Depth; z++) {
				NewZ1 = z * SclPlane;
				NewZ2 = (ILuint)(z / ScaleZ) * ImgPlane;
				for (y = 0; y < Height; y++) {
					NewY1 = y * SclBps;
					NewY2 = (ILuint)(y / ScaleY) * ImgBps;
					for (x = 0; x < Width; x++) {
						NewX1 = x * Scaled->Bpp;
						NewX2 = (ILuint)(x / ScaleX) * Image->Bpp;
						for (c = 0; c < Scaled->Bpp; c++) {
							SShortPtr[NewZ1 + NewY1 + NewX1 + c] =
								ShortPtr[NewZ2 + NewY2 + NewX2 + c];
						}
					}
				}
			}
			break;

		case 4:
			IntPtr = (ILuint*)Image->Data;
			SIntPtr = (ILuint*)Scaled->Data;
			for (z = 0; z < Depth; z++) {
				NewZ1 = z * SclPlane;
				NewZ2 = (ILuint)(z / ScaleZ) * ImgPlane;
				for (y = 0; y < Height; y++) {
					NewY1 = y * SclBps;
					NewY2 = (ILuint)(y / ScaleY) * ImgBps;
					for (x = 0; x < Width; x++) {
						NewX1 = x * Scaled->Bpp;
						NewX2 = (ILuint)(x / ScaleX) * Image->Bpp;
						for (c = 0; c < Scaled->Bpp; c++) {
							SIntPtr[NewZ1 + NewY1 + NewX1 + c] =
								IntPtr[NewZ2 + NewY2 + NewX2 + c];
						}
					}
				}
			}
			break;
	}

	return Scaled;
}


ILimage *iluScale3DLinear_(ILimage *Image, ILimage *Scaled, ILuint Width, ILuint Height, ILuint Depth)
{
	ImgBps = Image->Bps / Image->Bpc;
	SclBps = Scaled->Bps / Scaled->Bpc;
	ImgPlane = Image->SizeOfPlane / Image->Bpc;
	SclPlane = Scaled->SizeOfPlane / Scaled->Bpc;

	switch (Image->Bpc)
	{
		case 1:
			for (z = 0; z < Depth; z++) {
				NewZ1 = (ILuint)(z / ScaleZ) * ImgPlane;
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

						Size = z * SclPlane + y * SclBps + x * Scaled->Bpp;
						for (c = 0; c < Scaled->Bpp; c++) {
							x1 = Image->Data[NewZ1 + NewY1 + NewX1 + c];
							x2 = Image->Data[NewZ1 + NewY1 + NewX2 + c];
							Scaled->Data[Size + c] = (ILubyte)((1.0 - f) * x1 + f * x2);
						}
					}
				}
			}
			break;

		case 2:
			ShortPtr = (ILushort*)Image->Data;
			SShortPtr = (ILushort*)Scaled->Data;
			for (z = 0; z < Depth; z++) {
				NewZ1 = (ILuint)(z / ScaleZ) * ImgPlane;
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

						Size = z * SclPlane + y * SclBps + x * Scaled->Bpp;
						for (c = 0; c < Scaled->Bpp; c++) {
							x1 = ShortPtr[NewZ1 + NewY1 + NewX1 + c];
							x2 = ShortPtr[NewZ1 + NewY1 + NewX2 + c];
							SShortPtr[Size + c] = (ILubyte)((1.0 - f) * x1 + f * x2);
						}
					}
				}
			}
			break;

		case 4:
			IntPtr = (ILuint*)Image->Data;
			SIntPtr = (ILuint*)Scaled->Data;
			for (z = 0; z < Depth; z++) {
				NewZ1 = (ILuint)(z / ScaleZ) * ImgPlane;
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

						Size = z * SclPlane + y * SclBps + x * Scaled->Bpp;
						for (c = 0; c < Scaled->Bpp; c++) {
							x1 = IntPtr[NewZ1 + NewY1 + NewX1 + c];
							x2 = IntPtr[NewZ1 + NewY1 + NewX2 + c];
							SIntPtr[Size + c] = (ILubyte)((1.0 - f) * x1 + f * x2);
						}
					}
				}
			}
			break;
	}

	return Scaled;
}


/*ILimage *iluScale3DBilinear_(ILimage *Image, ILimage *Scaled, ILuint Width, ILuint Height, ILuint Depth);
{
		Depth--;  // Only use regular Depth once in the following loop.
		Height--;  // Only use regular Height once in the following loop.
		for (z = 0; z < Depth; z++) {
			NewZ1 = (ILuint)(z / ScaleZ) * Image->SizeOfPlane;
			NewZ2 = (ILuint)((z+1) / ScaleZ) * Image->SizeOfPlane;
			for (y = 0; y < Height; y++) {
				NewY1 = (ILuint)(y / ScaleY) * Image->Bps;
				NewY2 = (ILuint)((y+1) / ScaleY) * Image->Bps;
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
						Table[0][0][c] = t3 * Image->Data[NewZ1 + NewY1 + NewX1 + c] +
							t2 * Image->Data[NewZ1 + NewY1 + NewX2 + c];

						Table[0][1][c] = t3 * Image->Data[NewZ1 + NewY2 + NewX1 + c] +
							t2 * Image->Data[NewZ1 + NewY2 + NewX2 + c];

						Table[1][0][c] = t3 * Image->Data[NewZ2 + NewY1 + NewX1 + c] +
							t2 * Image->Data[NewZ2 + NewY1 + NewX2 + c];

						Table[1][1][c] = t3 * Image->Data[NewZ2 + NewY2 + NewX1 + c] +
							t2 * Image->Data[NewZ2 + NewY2 + NewX2 + c];
					}

					// Linearly interpolate between the table values.
					t1 = y / (ILdouble)(Height + 1);  // Height+1 is the real height now.
					t2 = z / (ILdouble)(Depth + 1);   // Depth+1 is the real depth now.
					t3 = (1.0 - t1);
					Size = z * Scaled->SizeOfPlane + y * Scaled->Bps + x * Scaled->Bpp;
					for (c = 0; c < Scaled->Bpp; c++) {
						x1 = t3 * Table[0][0][c] + t1 * Table[0][1][c];
						x2 = t3 * Table[1][0][c] + t1 * Table[1][1][c];
						Scaled->Data[Size + c] = (ILubyte)((1.0 - t2) * x1 + t2 * x2);
					}
				}
			}

			// Calculate the last row.
			NewY1 = (ILuint)(Height / ScaleY) * Image->Bps;
			for (x = 0; x < Width; x++) {
				NewX = Width / ScaleX;
				t1 = x / (ILdouble)Width;
				t4 = t1 * Width;
				ft = (t4 - (ILuint)(t4)) * IL_PI;
				f = (1.0 - cos(ft)) * .5;  // Cosine interpolation
				NewX1 = (ILuint)(t1 * NewX) * Image->Bpp;
				NewX2 = (ILuint)(t1 * NewX + 1) * Image->Bpp;

				Size = Height * Scaled->Bps + x * Image->Bpp;
				for (c = 0; c < Scaled->Bpp; c++) {
					Scaled->Data[Size + c] = (ILubyte)((1.0 - f) * Image->Data[NewY1 + NewX1 + c] +
						f * Image->Data[NewY1 + NewX2 + c]);
				}
			}
		}

		NewZ1 = (ILuint)(Depth / ScaleZ) * Image->SizeOfPlane;
		for (y = 0; y < Height; y++) {
			NewY1 = (ILuint)(y / ScaleY) * Image->Bps;
			for (x = 0; x < Width; x++) {
				t1 = x / (ILdouble)Width;
				t4 = t1 * Width;
				t2 = t4 - (ILuint)(t4);
				ft = t2 * IL_PI;
				f = (1.0 - cos(ft)) * .5;
				NewX1 = ((ILuint)(t4 / ScaleX)) * Image->Bpp;
				NewX2 = ((ILuint)(t4 / ScaleX) + 1) * Image->Bpp;

				Size = (Depth) * Scaled->SizeOfPlane + y * Scaled->Bps + x * Scaled->Bpp;
				for (c = 0; c < Scaled->Bpp; c++) {
					x1 = Image->Data[NewZ1 + NewY1 + NewX1 + c];
					x2 = Image->Data[NewZ1 + NewY1 + NewX2 + c];
					Scaled->Data[Size + c] = (ILubyte)((1.0 - f) * x1 + f * x2);
				}
			}
		}
	}

	return Scaled;
}
*/
