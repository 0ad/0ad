/*
Encryption.h
by Caecus
caecus18@hotmail.com

Two simple functions for encrypting and decrypting data. The KeyLength is
specified in bytes, therefore Keys must be in multiples of 8 bits. I'd
advice using 128bit keys which you can define as such.

_byte MyKey[16] = { _byte(0xAF), _byte(0x2B), _byte(0x80), _byte(0x7E),
					_byte(0x09), _byte(0x23), _byte(0xCC), _byte(0x95),
					_byte(0xB4), _byte(0x2D), _byte(0xF4), _byte(0x90),
					_byte(0xB3), _byte(0xC4), _byte(0x2A), _byte(0x3B) };

There may be a better way to do this but this looks alright to me.
*/


#include "Prometheus.h"

#ifndef ENCRYPTION_H
#define ENCRYPTION_H

// Simple Encryption function
_byte *EncryptData(_byte *Data, _long DataLength, _byte *Key, _long KeyLength);

// Simple Decryption function
_byte *DecryptData(_byte *Data, _long DataLength, _byte *Key, _long KeyLength);

#endif