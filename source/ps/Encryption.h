/*
Encryption.h
by Caecus
caecus18@hotmail.com

Two simple functions for encrypting and decrypting data. The KeyLength is
specified in bytes, therefore Keys must be in multiples of 8 bits. I'd
advice using 128bit keys which you can define as such.

char MyKey[16] = { char(0xAF), char(0x2B), char(0x80), char(0x7E),
					char(0x09), char(0x23), char(0xCC), char(0x95),
					char(0xB4), char(0x2D), char(0xF4), char(0x90),
					char(0xB3), char(0xC4), char(0x2A), char(0x3B) };

There may be a better way to do this but this looks alright to me.
*/


#include "Pyrogenesis.h"

#ifndef ENCRYPTION_H
#define ENCRYPTION_H

// Simple Encryption function
char *EncryptData(char *Data, long DataLength, char *Key, long KeyLength);

// Simple Decryption function
char *DecryptData(char *Data, long DataLength, char *Key, long KeyLength);

#endif
