/*
 * wxJavaScript - membuf.h
 *
 * Copyright (c) 2002-2007 Franky Braem and the wxJavaScript project
 *
 * Project Info: http://www.wxjavascript.net or http://wxjs.sourceforge.net
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301,
 * USA.
 *
 * $Id: membuf.h 598 2007-03-07 20:13:28Z fbraem $
 */
#ifndef __MEMBUF_H__
#define __MEMBUF_H__

#include <string>

typedef unsigned __MEMBUF_INT__; // Buffer size for 32-bit compilers
const __MEMBUF_INT__ __MEMBUF_NO_MATCH__ = 0xFFFFFFFF; 

// 09/14/2005: Added null buffer class used to reserve address space 
// for null memory buffers
class MemoryBufferNULLPtr
{
public:
  MemoryBufferNULLPtr() { }
  ~MemoryBufferNULLPtr() { }

public: // Global null memory buffer pointers
  static unsigned char MemoryBufferNUllChar;
  static unsigned char MemoryBufferNUllBuf[1];
};

// Memory buffer class
class MemoryBuffer
{
public:
  MemoryBuffer() { mptr = 0; l_length = d_length = 0; }
  MemoryBuffer(__MEMBUF_INT__ bytes) {
    mptr = 0; l_length = d_length = 0; Alloc(bytes);
  }
  MemoryBuffer(const void *buf, __MEMBUF_INT__ bytes);
  ~MemoryBuffer() { if(mptr) delete mptr; }
  MemoryBuffer(const MemoryBuffer &buf);
  MemoryBuffer &operator=(const MemoryBuffer &buf);

public: // Pointer and length functions
  __MEMBUF_INT__ length() const { return l_length; }  // Returns logical length
  __MEMBUF_INT__ dlength() const { return d_length; } // Returns actual length
  int resize(__MEMBUF_INT__ bytes, int keep = 1);
  unsigned char *m_buf() { return mptr; }
  const unsigned char *m_buf() const { return mptr; } 
  int is_null() { return ((mptr == 0) || (l_length == 0)); }
  int is_null() const { return ((mptr == 0) || (l_length == 0)); }
  int Load(const void *buf, __MEMBUF_INT__ bytes); 
  void Clear(int clean_buf = 1);
  int MemSet(const char c, __MEMBUF_INT__ offset = 0, __MEMBUF_INT__ num = 0);

  std::string toString() { return std::string((char *) mptr, l_length); }

public: // Append, Insert, delete, and remove functions
  void Cat(const void *buf, __MEMBUF_INT__ bytes) { 
    InsertAt(l_length, buf, bytes); 
  }
  void Cat(unsigned char byte) { Cat((void *)&byte, 1); }
  void Cat(char byte) { Cat((void *)&byte, 1); }
  __MEMBUF_INT__ DeleteAt(__MEMBUF_INT__ position, __MEMBUF_INT__ bytes);
  __MEMBUF_INT__ InsertAt(__MEMBUF_INT__ position, const void *buf, 
			  __MEMBUF_INT__ bytes);
  __MEMBUF_INT__ InsertAt(__MEMBUF_INT__ position, const MemoryBuffer &buf) {
    return InsertAt(position, (const void *)buf.mptr, buf.l_length);
  }
  __MEMBUF_INT__ ReplaceAt(__MEMBUF_INT__ position, const void *buf,
			   __MEMBUF_INT__ bytes);
  __MEMBUF_INT__ ReplaceAt(__MEMBUF_INT__ position, const MemoryBuffer &buf) {
    return ReplaceAt(position, (const void *)buf.mptr, buf.l_length);
  }
  
public: // Search functions
  __MEMBUF_INT__ Find(void *buf, __MEMBUF_INT__ offset = 0); 
  __MEMBUF_INT__ Find(const void *buf, __MEMBUF_INT__ bytes, 
		      __MEMBUF_INT__ offset = 0) const;
  __MEMBUF_INT__ Find(MemoryBuffer &buf, 
		      __MEMBUF_INT__ offset = 0) {
    return Find(buf.mptr, buf.l_length, offset); 
  }
  __MEMBUF_INT__ Find(const MemoryBuffer &buf, 
		      __MEMBUF_INT__ offset = 0) const {
    return Find(buf.mptr, buf.l_length, offset); 
  }
  
public: // Memory allocation/de-allocation routines
  void *Alloc(__MEMBUF_INT__ bytes);
  void *Realloc(__MEMBUF_INT__ bytes, int keep = 1, int reuse = 1);
  void Destroy();
  void *FreeBytes();

public: // Conversion functions
  operator char *() const { return (char *)mptr; }
  operator const char *() const { return (const char*)mptr; }
  operator const int () const { return ((mptr != 0) && (l_length != 0)); }
  operator int () { return ((mptr != 0) && (l_length != 0)); }

public: // Overloaded operators
  int operator!() { return ((mptr == 0) || (l_length == 0)); } 
  int operator!() const { return ((mptr == 0) || (l_length == 0)); }
  unsigned char &operator[](__MEMBUF_INT__ i);
  unsigned char &operator[](__MEMBUF_INT__ i) const;
  void operator+=(const MemoryBuffer &buf) { Cat(buf.mptr, buf.l_length); }
  void operator+=(const unsigned char &byte) { Cat(byte); }
  void operator+=(const char &byte) { Cat(byte); }
  friend MemoryBuffer operator+(const MemoryBuffer &a, 
					     const MemoryBuffer &b);

  friend int operator==(const MemoryBuffer &a, 
				     const MemoryBuffer &b) {
    return BufferCompare(a, b) == 0;
  }

  friend int operator!=(const MemoryBuffer &a, 
				     const MemoryBuffer &b) {
    return BufferCompare(a, b) != 0;
  }

  friend int operator>(const MemoryBuffer &a, 
				    const MemoryBuffer &b) {
    return BufferCompare(a, b) > 0;
  }

  friend int operator>=(const MemoryBuffer &a, 
				     const MemoryBuffer &b) {
    return BufferCompare(a, b) >= 0;
  }
  
  friend int operator<(const MemoryBuffer &a, 
				    const MemoryBuffer &b) {
    return BufferCompare(a, b) < 0;
  }
  
  friend int operator<=(const MemoryBuffer &a, 
				     const MemoryBuffer &b) {
    return BufferCompare(a, b) <= 0;
  }

public: // Friend functions
  // Returns -1 if a < b, 0 if a == b, and 1 if a > b
  friend int BufferCompare(const MemoryBuffer &a, 
					const MemoryBuffer &b);  

private: 
  unsigned char *mptr;     // Pointer to start of buffer
  __MEMBUF_INT__ d_length; // Number of bytes allocated for the buffer
  __MEMBUF_INT__ l_length; // Logical length of the buffer
};

#endif  // __MEMBUF_H__

