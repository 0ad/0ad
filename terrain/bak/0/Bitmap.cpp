//***********************************************************
//
// Name:		Bitmap.Cpp
// Author:		Poya Manouchehri
//
// Description: CBitmap operates on a bitmap. Currently it
//				can load BMPs and TGAs. All bitmaps (even 8-bit
//				ones) are converted into full RGBAs.
//
//***********************************************************

#include "Bitmap.H"
#include "RenderPrims.H"

CBitmap::CBitmap ()
{
	//everthing is 0
	m_pBits = NULL;
	m_Height = m_Width = 0;
}

CBitmap::~CBitmap ()
{
	//destroy the data
	DestroyData ();
}

//Load the image from a file
FRESULT CBitmap::LoadBitmap (char *path, RESOURCETYPE type)
{
	FILE *ImageFile = NULL;
	int ImageType;
	FRESULT Result;

	CFileResource::SetupParams (path, type);

	//clean up
	DestroyData ();

	ImageFile = fopen (path, "rb");

	if (!ImageFile)
	{
		return R_FILE_NOOPEN;
	}

	ImageType = GetImageType (ImageFile);

	switch (ImageType)
	{
		case FILE_TYPE_BMP:
			//try to load as a BMP
			Result = LoadBMP (ImageFile);
			break;

		case FILE_TYPE_TGA:
			//try to load as a TGA
			Result = LoadTGA (ImageFile);
			break;

		default:
			return R_BADPARAMS;
			break;
	}

	if (Result != R_OK)
		return R_FAIL;


	return R_OK;
}

FRESULT CBitmap::CreateBitmap (RESOURCETYPE type, char *name, int width, int height)
{
	//clean any old data
	DestroyData ();

	strcpy (m_Name, name);
	m_Type = type;

	m_Width = width;
	m_Height = height;

	m_pBits = new unsigned char [m_Width*m_Height*4];

	if (!m_pBits)
		return R_NOMEMORY;

	return R_OK;
}

//Get the file format
int CBitmap::GetImageType (FILE *file)
{
	short int type;

	//is the file valid ?
	if (!file)
	{
		fclose(file);
		return FILE_TYPE_NOSUPPORT;
	}

	// Read the first 2 bytes of the file
	if (fread (&type, 2, 1, file) != 1)
	{
		fclose(file);
		return FILE_TYPE_NOSUPPORT;
	}
	
	rewind(file);

	// Is it a bitmap ?
	if (type == 0x4d42)
	{
		rewind (file);
		return FILE_TYPE_BMP;
	}

	// TGA's don't have a header ID, so just take a guess
	rewind(file);
	return FILE_TYPE_TGA;
}

//Load a bmp file
FRESULT CBitmap::LoadBMP (FILE *file)
{
	BITMAPFILEHEADER bmfileh;
	BITMAPINFOHEADER bminfoh;
	RGBQUAD bmColors[256];

	//Note: the file format of a BMP file is:

	//		BITMAPFILEHEADER
	//		BITMAPINFOHEADER
	//		RGBQUAD[2^bitcount]
	//		the actual bits (indices in case of an 8 bit bitmap)

	fread (&bmfileh, sizeof (BITMAPFILEHEADER), 1, file);
	//read the file header
	fread (&bminfoh, sizeof (BITMAPINFOHEADER), 1, file);
	//read the bitmap header

	m_Width = bminfoh.biWidth;
	m_Height = abs(bminfoh.biHeight);

	//read the color table if indexed
	if (bminfoh.biBitCount < 24)
		fread (&bmColors, sizeof (RGBQUAD), 256, file);

	//move to the point where the actual bit data start
	fseek (file, bmfileh.bfOffBits, SEEK_SET);

	//load the data
	return LoadData (file, bminfoh.biBitCount, bmColors);
}

//Load a TGA file
FRESULT CBitmap::LoadTGA (FILE *file)
{
	TGAHeader tgaHeader;

	int loadedOK = 1;
	int i, temp;

	// We have to load the header explicitly, due to some time
	// weird memory aligning thing that goes on at compile
	loadedOK &= fread(&tgaHeader.IDLength, 1, 1, file);
	loadedOK &= fread(&tgaHeader.MapType, 1, 1, file);
	loadedOK &= fread(&tgaHeader.ImageType, 1, 1, file);
	loadedOK &= fread(&tgaHeader.MapOrigin, 2, 1, file);
	loadedOK &= fread(&tgaHeader.MapLength, 2, 1, file);
	loadedOK &= fread(&tgaHeader.MapEntrySize, 1, 1, file);
	loadedOK &= fread(&tgaHeader.XOrigin, 2, 1, file);
	loadedOK &= fread(&tgaHeader.YOrigin, 2, 1, file);
	loadedOK &= fread(&tgaHeader.Width, 2, 1, file);
	loadedOK &= fread(&tgaHeader.Height, 2, 1, file);
	loadedOK &= fread(&tgaHeader.BPP, 1, 1, file);
	loadedOK &= fread(&tgaHeader.Descriptor, 1, 1, file);

	// Ensure all elements of the file header were loaded
	if (!loadedOK) 
	{
		fclose(file);
		return R_FAIL;
	}

	// Check the image type
	if (tgaHeader.ImageType != 1 && tgaHeader.ImageType != 2 && tgaHeader.ImageType != 3)
	{
		fclose(file);
		return R_FAIL;
	}
	
	if (tgaHeader.BPP != 24 && tgaHeader.BPP != 32)
	{
		fclose(file);
		return R_FAIL;
	}

	// Skip any identification data in the header (we're not interested in that)
	for (i=0; i<tgaHeader.IDLength; i++)
		fread(&temp, sizeof(unsigned char), 1, file);

	// Skip the colour look up tables
	for (i=0; i<tgaHeader.MapLength*(tgaHeader.MapEntrySize>>3); i++)
		fread(&temp, sizeof(unsigned char), 1, file);

	m_Width = tgaHeader.Width;
	m_Height = tgaHeader.Height;

	return LoadData (file, tgaHeader.BPP, NULL);
}

//Load the actual bits from a file. the table is NULL
//unless the image is 8-bit
FRESULT CBitmap::LoadData (FILE *file, int bpp, RGBQUAD *table)
{
	unsigned char *TempBits = NULL;
	//temporary bits which are read straight from the file

	m_pBits = new unsigned char[m_Width*m_Height*4];
	//allocate memory for the bits
	
	//this is rather bad: no memory
	if (!m_pBits)
	{
		fclose (file);
		return R_NOMEMORY;
	}
	
	//read the bits
	switch (bpp)
	{
		case 8:
		{
			TempBits = new unsigned char[m_Width*m_Height];
			//allocate some memory for temporary bits
			break;
		}

		case 24:
		{
			TempBits = new unsigned char[m_Width*m_Height*3];
			//allocate some memory for temporary bits
			break;
		}

		case 32:
		{
			TempBits = new unsigned char[m_Width*m_Height*4];
			//allocate some memory for temporary bits
			break;
		}
		
		default:
		{
			fclose (file);
			return R_FILE_INVALID;
			break;
		}
	}

	//no memory
	if (!TempBits)
	{
		fclose (file);
		return R_NOMEMORY;
	}		

	//now lets actually read the bits from the file and copy them into
	//Bits in the proper fashion
	switch (bpp)
	{
		case 8:
		{
			//only one channel (8-bit) so there are Width*Height values to read
			if (fread (TempBits, sizeof (unsigned char), m_Width*m_Height, file) != (unsigned int)(m_Width*m_Height))
			{
				fclose (file);
				return R_FILE_NOREAD;
			}

			//the following loop converts the index data of TempBits,
			//into actual color data (read from the color table) and
			//stores it to Bits
			int i;

			for (int index=0;index<m_Width*m_Height;index++)
			{
				i = TempBits[index];
				m_pBits[index*4    ] = table[i].rgbRed;
				m_pBits[index*4 + 1] = table[i].rgbGreen;
				m_pBits[index*4 + 2] = table[i].rgbBlue;
				m_pBits[index*4 + 3] = 255;
			}

			break;
		}

		case 24:
		{
			//3 channels (24-bit) so there are Width*Height*3 values to read
			if (fread (TempBits, sizeof (unsigned char), m_Width*m_Height*3, file) != (unsigned int)(m_Width*m_Height*3))
			{
				fclose (file);
				return R_FILE_NOREAD;
			}

			//we have no palette so just read all the color values
			for (int index=0;index<m_Width*m_Height;index++)
			{
				//since data is stored in BGR we convert it to RGB
				m_pBits[index*4    ] = TempBits[index*3  ];
				m_pBits[index*4 + 1] = TempBits[index*3+1];
				m_pBits[index*4 + 2] = TempBits[index*3+2];
				m_pBits[index*4 + 3] = 255;
			}

			break;
		}

		case 32:
		{
			//4 channels (32-bit) so there are Width*Height*4 values to read
			if (fread (TempBits, sizeof (unsigned char), m_Width*m_Height*4, file) != (unsigned int)(m_Width*m_Height*4))
			{
				fclose (file);
				return R_FILE_NOREAD;
			}

			//load a 32 bit image. The 4th channel is the alpha data
			//again we convert from BGRA to RGBA
			for (int index=0;index<m_Width*m_Height;index++)
			{
				m_pBits[index*4    ] = TempBits[index*4  ];
				m_pBits[index*4 + 1] = TempBits[index*4+1];
				m_pBits[index*4 + 2] = TempBits[index*4+2];
				m_pBits[index*4 + 3] = TempBits[index*4+3];
			}

			break;
		}
	}

	//delete all the unwanted:
	delete [] TempBits;

	return R_OK;
}

void CBitmap::DestroyData ()
{
	if (m_pBits)
	{
		delete [] m_pBits;
		m_pBits = NULL;
	}

	m_Width = m_Height = 0;
}