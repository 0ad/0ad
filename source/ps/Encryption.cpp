// last modified Friday, May 09, 2003

#include "Encryption.h"

//--------------------------------------------------------------------
// EncryptData - Takes A pointer to data in memory, the length of that data, 
//				 Along with a key in memory and its length. It will allocated space
// for the encrypted copy of the data via new[], It is the responsiblity of the user
// to call delete[]. 
//--------------------------------------------------------------------
char *EncryptData(char *Data, long DataLength, char *Key, long KeyLength)
{
	// Allocate space for new Encrypted data
	char *NewData = new char[DataLength];

	// A counter to hold our absolute position in data
	long DataOffset = 0;

	// Loop through Data until end is reached
	while (DataOffset < DataLength)
	{
		// Loop through the key
		for (long KeyOffset = 0; KeyOffset < KeyLength; KeyOffset++)
		{
			// If were at end of data break and the the while loop should end as well.
			if (DataOffset >= DataLength)
				break;

			// Otherwise Add the previous element of the key from the newdata
			// (just something a little extra to confuse the hackers)
			if (KeyOffset > 0) // Don't mess with the first byte 
				NewData[DataOffset] += Key[KeyOffset - 1];

			// Xor the Data byte with the key byte and get new data
			NewData[DataOffset] = Data[DataOffset] ^ Key[KeyOffset];

			// Increase position in data
			DataOffset++;
		}	
	}

	// return the new data
	return NewData;
}


//--------------------------------------------------------------------
// DecryptData - Takes A pointer to data in memory, the length of that data, 
//				 Along with a key in memory and its length. It will allocated space
// for the decrypted copy of the data via new[], It is the responsiblity of the user
// to call delete[]. 
//--------------------------------------------------------------------
char *DecryptData(char *Data, long DataLength, char *Key, long KeyLength)
{
	// Allocate space for new Decrypted data
	char *NewData = new char[DataLength];

	// A counter to hold our absolute position in data
	long DataOffset = 0;

	// Loop through Data until end is reached
	while (DataOffset < DataLength)
	{
		// Loop through the key
		for (long KeyOffset = 0; KeyOffset < KeyLength; KeyOffset++)
		{
			// If were at end of data break and the the while loop should end as well.
			if (DataOffset >= DataLength)
				break;

			// Otherwise Xor the Data byte with the key byte and get new data
			NewData[DataOffset] = Data[DataOffset] ^ Key[KeyOffset];

			// Subtract the previous element of the key from the newdata
			// (just something a little extra to confuse the hackers)
			if (KeyOffset > 0) // Don't mess with the first byte 
				NewData[DataOffset] -= Key[KeyOffset - 1];

			// Increase position in data
			DataOffset++;
		}	
	}

	// return the new data
	return NewData;
}
