/*
	Copyright (C) 2005-2007 Feeling Software Inc.
	Portions of the code are:
	Copyright (C) 2005-2007 Sony Computer Entertainment America
	
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/

#include "StdAfx.h"
#include "FUBase64.h"

namespace FUBase64
{
	const static char BASE64_ALPHABET[64] = 
	{
		'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', //   0 -   9
		'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', //  10 -  19
		'U', 'V', 'W', 'X', 'Y', 'Z', 'a', 'b', 'c', 'd', //  20 -  29
		'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', //  30 -  39
		'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', //  40 -  49
		'y', 'z', '0', '1', '2', '3', '4', '5', '6', '7', //  50 -  59
		'8', '9', '+', '/'								  //  60 -  63
	};

	const static char BASE64_DEALPHABET[128] = 
	{
		 0,  0,  0,  0,  0,  0,  0,  0,  0,  0, //   0 -   9
		 0,  0,  0,  0,  0,  0,  0,  0,  0,  0, //  10 -  19
		 0,  0,  0,  0,  0,  0,  0,  0,  0,  0, //  20 -  29
		 0,  0,  0,  0,  0,  0,  0,  0,  0,  0, //  30 -  39
		 0,  0,  0, 62,  0,  0,  0, 63, 52, 53, //  40 -  49
		54, 55, 56, 57, 58, 59, 60, 61,  0,  0, //  50 -  59
		 0, 61,  0,  0,  0,  0,  1,  2,  3,  4, //  60 -  69
		 5,  6,  7,  8,  9, 10, 11, 12, 13, 14, //  70 -  79
		15, 16, 17, 18, 19, 20, 21, 22, 23, 24, //  80 -  89
		25,  0,  0,  0,  0,  0,  0, 26, 27, 28, //  90 -  99
		29, 30, 31, 32, 33, 34, 35, 36, 37, 38, // 100 - 109
		39, 40, 41, 42, 43, 44, 45, 46, 47, 48, // 110 - 119
		49, 50, 51,  0,  0,  0,  0,  0			// 120 - 127
	};


	size_t CalculateRecquiredEncodeOutputBufferSize(size_t inputByteCount)
	{
		div_t result = div ((int) inputByteCount, 3);

		// Number of encoded characters
		size_t requiredBytes = result.quot * 4;
		if (result.rem > 0) requiredBytes += 4;
		return requiredBytes;
	}

	size_t CalculateRecquiredDecodeOutputBufferSize (const uint8* inputBufferString, size_t inputLength)
	{
		size_t bufferLength = inputLength;
		div_t result = div ((int) bufferLength, 4);

		if (inputBufferString[bufferLength - 1] != '=')
		{
			return result.quot * 3;
		}
		else
		{
			if (inputBufferString[bufferLength - 2] == '=')
			{
				return result.quot * 3 - 2;
			}
			else
			{
				return result.quot * 3 - 1;
			}
		}
	}

	void EncodeByteTriple(const uint8* inputBuffer, size_t inputbufferLength, uint8* outputBuffer)
	{
		uint32 mask = 0xfc000000;
		uint32 buffer = 0;

		uint8* temp = (uint8*) &buffer;
		temp[3] = inputBuffer[0];
		if (inputbufferLength > 1) temp[2] = inputBuffer[1];
		if (inputbufferLength > 2) temp[1] = inputBuffer[2];

		switch (inputbufferLength)
		{
		case 3:
			{
				outputBuffer[0] = BASE64_ALPHABET[(buffer & mask) >> 26];
				buffer = buffer << 6;
				outputBuffer[1] = BASE64_ALPHABET[(buffer & mask) >> 26];
				buffer = buffer << 6;
				outputBuffer[2] = BASE64_ALPHABET[(buffer & mask) >> 26];
				buffer = buffer << 6;
				outputBuffer[3] = BASE64_ALPHABET[(buffer & mask) >> 26];
				break;
			}
		case 2:
			{
				outputBuffer[0] = BASE64_ALPHABET[(buffer & mask) >> 26];
				buffer = buffer << 6;
				outputBuffer[1] = BASE64_ALPHABET[(buffer & mask) >> 26];
				buffer = buffer << 6;
				outputBuffer[2] = BASE64_ALPHABET[(buffer & mask) >> 26];
				outputBuffer[3] = '=';
				break;
			}
		case 1:
			{
				outputBuffer[0] = BASE64_ALPHABET[(buffer & mask) >> 26];
				buffer = buffer << 6;
				outputBuffer[1] = BASE64_ALPHABET[(buffer & mask) >> 26];
				outputBuffer[2] = '=';
				outputBuffer[3] = '=';
				break;
			}
		default:
			FUFail(break);
		} // end switch
	}

	size_t DecodebyteQuartet (const uint8* inputBuffer, uint8* outputBuffer)
	{
		uint32 buffer = 0;

		if (inputBuffer[3] == '=')
		{
			if (inputBuffer[2] == '=')
			{
				buffer = (buffer | BASE64_DEALPHABET[inputBuffer[0]]) << 6;
				buffer = (buffer | BASE64_DEALPHABET[inputBuffer[1]]) << 6;
				buffer = buffer << 14;

				uint8* temp = (uint8*) &buffer;
				outputBuffer[0] = temp[3];
				
				return 1;
			}
			else
			{
				buffer = (buffer | BASE64_DEALPHABET[inputBuffer[0]]) << 6;
				buffer = (buffer | BASE64_DEALPHABET[inputBuffer[1]]) << 6;
				buffer = (buffer | BASE64_DEALPHABET[inputBuffer[2]]) << 6;
				buffer = buffer << 8;

				uint8* temp = (uint8*) &buffer;
				outputBuffer[0] = temp[3];
				outputBuffer[1] = temp[2];
				
				return 2;
			}
		}
		else
		{
			buffer = (buffer | BASE64_DEALPHABET[inputBuffer[0]]) << 6;
			buffer = (buffer | BASE64_DEALPHABET[inputBuffer[1]]) << 6;
			buffer = (buffer | BASE64_DEALPHABET[inputBuffer[2]]) << 6;
			buffer = (buffer | BASE64_DEALPHABET[inputBuffer[3]]) << 6; 
			buffer = buffer << 2;

			uint8* temp = (uint8*) &buffer;
			outputBuffer[0] = temp[3];
			outputBuffer[1] = temp[2];
			outputBuffer[2] = temp[1];

			return 3;
		}
		FUFail(return (size_t)-1);
	}

	FCOLLADA_EXPORT void encode(const UInt8List& input, UInt8List& output)
	{
		output.clear();
		if (input.size() == 0) return;

		const uint8* inputBuffer = &input.at(0);
		size_t p_inputbufferLength = input.size();

		output.resize(CalculateRecquiredEncodeOutputBufferSize(input.size()), 0);
		uint8* outputBufferString = &output.at(0);

		size_t inputBufferIndex = 0;
		size_t outputBufferIndex = 0;

		while (inputBufferIndex < p_inputbufferLength)
		{
			if (p_inputbufferLength - inputBufferIndex <= 2)
			{
				EncodeByteTriple(inputBuffer + inputBufferIndex, p_inputbufferLength - inputBufferIndex, outputBufferString + outputBufferIndex);
				break;
			}
			else
			{
				EncodeByteTriple(inputBuffer + inputBufferIndex, 3, outputBufferString + outputBufferIndex);
				inputBufferIndex += 3;
				outputBufferIndex += 4;
			}
		}
	}

	FCOLLADA_EXPORT void decode(const UInt8List& input, UInt8List& output)
	{
		size_t inputBufferIndex  = 0;
		size_t outputBufferIndex = 0;

		const uint8* inputBufferString = &input.at(0);
		size_t inputbufferLength = input.size();

		output.clear();
		output.resize(CalculateRecquiredDecodeOutputBufferSize(inputBufferString, inputbufferLength), 0);
		uint8* outputBuffer = &output.at(0);

		uint8 byteQuartet[4];

		while (inputBufferIndex < inputbufferLength)
		{
			for (int i = 0; i < 4; ++i)
			{
				byteQuartet[i] = inputBufferString[inputBufferIndex];

				// Ignore all characters except the ones in BASE64_ALPHABET
				if ((byteQuartet[i] >= 48 && byteQuartet[i] <=  57) ||
					(byteQuartet[i] >= 65 && byteQuartet[i] <=  90) ||
					(byteQuartet[i] >= 97 && byteQuartet[i] <= 122) ||
					 byteQuartet[i] == '+' || byteQuartet[i] == '/' || byteQuartet[i] == '=')
				{}
				else
				{
					// Invalid character
					--i;
				}

				++inputBufferIndex;
			}

			outputBufferIndex += DecodebyteQuartet(byteQuartet, outputBuffer + outputBufferIndex);
		}
	}
}
