#include "precompiled.h"

/*
 * wxJavaScript - membuf.cpp
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
 * $Id: membuf.cpp 598 2007-03-07 20:13:28Z fbraem $
 */
#include "membuf.h"

// 09/14/2005: Initialize the global null memory buffer pointers
unsigned char MemoryBufferNULLPtr::MemoryBufferNUllChar = '\0';
unsigned char MemoryBufferNULLPtr::MemoryBufferNUllBuf[1] = { '\0' };
      
MemoryBuffer::MemoryBuffer(const void *buf, __MEMBUF_INT__ bytes)
{
  mptr = 0;
  l_length = d_length = 0;
  // PC-lint 09/14/2005: Possibly passing a null pointer
  if(buf) {
    if(Alloc(bytes)) { 
      if(mptr) {
	memmove(mptr, buf, bytes);
      }
    }
  }
}

MemoryBuffer::MemoryBuffer(const MemoryBuffer &buf)
{
  mptr = 0;
  l_length = d_length = 0;
  // PC-lint 09/14/2005: Possibly passing a null pointer
  if(buf.mptr) {
    if(Alloc(buf.l_length)) {
      if(mptr) {
	memmove(mptr, buf.mptr, buf.l_length);
      }
    }
  }
}

MemoryBuffer &MemoryBuffer::operator=(const MemoryBuffer &buf)
{
  if(this != &buf) { // Prevent self assignment
    if(Alloc(buf.l_length)) {
      // PC-lint 09/14/2005: Possible use of null pointer
      if(!mptr) return *this;
      memmove(mptr, buf.mptr, buf.l_length);
    }
  }
  return *this;
}

void *MemoryBuffer::Alloc(__MEMBUF_INT__ bytes)
// Allocate a specified number of bytes for this memory buffer.
// This function will try to re-use the current memory segment
// allocated for this buffer before re-allocating memory for
// the buffer. Returns a void pointer to the buffer or a null
// value if a memory allocation error occurs.
{
  if(mptr) { // Memory was previously allocated for this object
    if(d_length >= bytes) { // Try to reuse this space
      l_length = bytes;
      return (void *)mptr;
    }
    else { // Must resize the buffer
      delete[] mptr;// Delete the previous copy and re-allocate memory
      l_length = d_length = 0;
    }
  }

  // Allocate the specified number of bytes
  mptr = new unsigned char[bytes];
  if(!mptr) return 0; // Memory allocation error

  // Set the logical and dimensioned length of the buffer
  l_length = d_length = bytes;
  return (void *)mptr; // Return a pointer to the buffer
}

void *MemoryBuffer::Realloc(__MEMBUF_INT__ bytes, int keep, int reuse)
// Resize the logical length of the buffer. If the
// "keep" variable is true the old data will be
// copied into the new space. By default the old
// data will not be deleted. Returns a pointer to the
// buffer or a null value if an error occurs.
{
  // Try to reuse the memory allocated for this object
  if((reuse == 1) && (mptr != 0)) { 
    if(d_length >= bytes) { // No additional memory has to be allocated
      l_length = bytes;
      return (void *)mptr;
    }
  }
  
  unsigned char *nmptr = new unsigned char[bytes];
  if(!nmptr) return 0;

  if((keep == 1) && (mptr != 0)) { // Copy old data into the new memory segment
    if(bytes < l_length) l_length = bytes;
    memmove(nmptr, mptr, l_length);
  }

  if(mptr) delete[] mptr;        // Free the previously allocated memory
  mptr = nmptr;                // Point to new memory buffer
  l_length = d_length = bytes; // Record new allocated length
  return (void *)mptr;
}

void MemoryBuffer::Destroy()
// Frees the memory allocated for the buffer and resets the
// length variables.
{
  if(mptr) delete[] mptr;
  mptr = 0;
  l_length = d_length = 0;
}

void *MemoryBuffer::FreeBytes()
// Free any unused bytes allocated for this buffer. Returns
// a pointer to the re-allocated memory buffer or a null
// value if an error occurs.
{
  // Check for unused bytes
  if(d_length == l_length) return (void *)mptr;
  return Realloc(l_length, 1, 0);
}

int MemoryBuffer::resize(__MEMBUF_INT__ bytes, int keep)
// Resize the logical length of the buffer. If the
// "keep" variable is true the old data will be
// copied into the new space. By default the old
// data will not be deleted. Returns true if
// successful or false if an error occurs.
{
  if(!Realloc(bytes, keep)) return 0;
  return 1;
}

__MEMBUF_INT__ MemoryBuffer::Find(void *buf, __MEMBUF_INT__ offset)
// Returns index of first occurrence of pattern void *buf.
// Returns __MEMBUF_NO_MATCH__ if the pattern is not found.
{
  // PC-lint 09/14/2005: Possible use of null pointer
  if(!mptr) return __MEMBUF_NO_MATCH__;

  unsigned char *start = mptr + offset;          // Start of buffer data
  unsigned char *next = start;                   // Next buffer element
  unsigned char *pattern = (unsigned char *)buf; // Next pattern element
  __MEMBUF_INT__ i = offset;                     // Next buffer element index
  
  while(i < l_length && *pattern) {
    if (*next == *pattern) {
      pattern++;
      if(*pattern == 0) return i; // Pattern was found
      next++;
    }
    else {
      i++;
      start++;
      next = start;
      pattern = (unsigned char *)buf;
    }
  }
  return __MEMBUF_NO_MATCH__; // No match was found
}

__MEMBUF_INT__ MemoryBuffer::Find(const void *buf, __MEMBUF_INT__ bytes, 
				  __MEMBUF_INT__ offset) const
// Returns index of first occurrence of pattern void *buf.
// Returns __MEMBUF_NO_MATCH__ if the pattern is not found.
{
  // PC-lint 09/14/2005: Possible use of null pointer
  if(!mptr) return __MEMBUF_NO_MATCH__;

  unsigned char *start = (unsigned char *)mptr + offset; // Start of buf data
  unsigned char *next = start;                   // Next buffer element
  unsigned char *pattern = (unsigned char *)buf; // Next pattern element
  __MEMBUF_INT__ i = offset, j = 1;              // Buffer and pattern indexes
  
  while(i < l_length && j <= bytes) {
    if (*next == *pattern) {   // Matching character
      if(j == bytes) return i; // Entire match was found
      next++; pattern++; j++;
    }
    else { // Try next spot in buffer
      i++;
      start++;
      next = start;
      pattern = (unsigned char *)buf; j = 1;
    }
  }
  return __MEMBUF_NO_MATCH__; // No match was found
}

__MEMBUF_INT__ MemoryBuffer::DeleteAt(__MEMBUF_INT__ position, 
				      __MEMBUF_INT__ bytes)
{
  __MEMBUF_INT__ buf;
  __MEMBUF_INT__ end;

  if(position < l_length && bytes !=0) {
    buf = position + bytes;
    if(buf > l_length) buf = l_length;
    end = unsigned(buf);
    bytes = end - position;

    // PC-lint 09/14/2005: Possible use of null pointer
    if(!mptr) return (__MEMBUF_INT__)0;

    memmove(mptr+position, mptr+end, l_length-end);
    l_length -= bytes;
  }
  else
    bytes = 0;

  return bytes;
}

__MEMBUF_INT__ MemoryBuffer::InsertAt(__MEMBUF_INT__ position, const void *buf,
				      __MEMBUF_INT__ bytes)
// Insert a specified number of bytes a the current position, keeping
// the current buffer intact. Returns the number of bytes inserted or
// zero if an error occurs.
{
  __MEMBUF_INT__ old_length = l_length; // Record the current buffer length
  
  // Ensure that there are enough bytes to hold the insertion
  if(!resize(bytes+l_length)) return 0;

  // PC-lint 09/14/2005: Possible use of null pointer
  if(!mptr) return (__MEMBUF_INT__)0;

  if(position < old_length) { // Move the data in the middle of the buffer
    memmove(mptr+position+bytes, mptr+position, old_length-position);
  }
  
  if(position > l_length) position = l_length; // Stay in bounds
  memmove(mptr+position, buf, bytes);
  return bytes;
}

__MEMBUF_INT__ MemoryBuffer::ReplaceAt(__MEMBUF_INT__ position, 
				       const void *buf,
				       __MEMBUF_INT__ bytes)
// Replace a specified number of bytes at the specified position. Returns
// the number of bytes replaced or zero if an error occurs.
{
  if(position > l_length) position = l_length; // Stay in bounds
  __MEMBUF_INT__ end = l_length-position;
  if(bytes > end) { // There are not enough bytes to hold the replacement
    __MEMBUF_INT__ needed = bytes-end;
    if(!resize(l_length + needed)) return 0;
  }

  // PC-lint 09/14/2005: Possible use of null pointer
  if(!mptr) return (__MEMBUF_INT__)0;

  memmove(mptr+position, buf , bytes);
  return bytes;
}

int BufferCompare(const MemoryBuffer &a, const MemoryBuffer &b)
// Returns -1 if a < b, 0 if a == b, and 1 if a > b
{
  __MEMBUF_INT__ an = a.l_length;
  __MEMBUF_INT__ bn = b.l_length;
  __MEMBUF_INT__ sn = (an < bn) ? an : bn;
  unsigned char *ap = (unsigned char *)a.mptr;
  unsigned char *bp = (unsigned char *)b.mptr;

  for(__MEMBUF_INT__ i = 0; i < sn; i++) {
    if(*ap < *bp) return -1;
    if(*ap++ > *bp++) return 1;
  }

  if(an == bn) return 0;
  if(an < bn) return -1;
  return 1;
}

int MemoryBuffer::Load(const void *buf, __MEMBUF_INT__ bytes) 
// Load this object with a unique copy of the specified buffer.
// Returns true if successful or false if an error occurs.
{   
  if(!mptr) { // Ensure that this buffer has been initialized
    if(!Alloc(bytes)) return 0;
  }

  if(d_length < bytes) { // Ensure enough byte have been allocated
    if(!Realloc(bytes, 0, 0)) return 0;
  }
  else { // Reuse the current memory segment
    l_length = bytes;
  }

  // PC-lint 09/14/2005: Possible use of null pointer
  if(!mptr) return 0;

  memmove(mptr, (unsigned char *)buf, l_length);
  return 1;
}

void MemoryBuffer::Clear(int clean_buf) 
// Clear the buffer. If the "clean_buf" variable is
// true the contents of the buffer will be cleared,
// otherwise the logical length will be set to 
// zero and the contents of the buffer will not
// cleared.
{ 
  if(l_length > 0) {
    if(clean_buf) DeleteAt(0, l_length); 
  }
  l_length = 0;
} 

int MemoryBuffer::MemSet(const char c, __MEMBUF_INT__ offset,
			 __MEMBUF_INT__ num)
// Public member function used to fill the memory buffer
// starting at the specified offset. Returns false if the
// buffer is null or the offset is out of range. The "num"
// variable is used to limit fills to a specified number 
// of bytes.
{
  if(is_null()) return 0; // This buffer is null

  // PC-lint 09/14/2005: Possible use of null pointer
  if(!mptr) return 0;
  
  // Test the offset before filling the memory buffer
  if((offset + num) > d_length) return 0;
  unsigned char *ptr = mptr+offset;
  __MEMBUF_INT__ pos = d_length - offset;
  for(__MEMBUF_INT__ i = 0; i < pos; i++) {
    if((num > 0) && (num == i)) break;
    *ptr++ = c;  
  }  
  return 1;
}

MemoryBuffer operator+(const MemoryBuffer &a, 
				    const MemoryBuffer &b)
{
  MemoryBuffer buf(a.l_length + b.l_length);
  buf += a;
  buf += b;
  return buf;
}

unsigned char &MemoryBuffer::operator[](__MEMBUF_INT__ i) 
{
  // PC-lint 09/14/2005: Possible use of null pointer
  if(!mptr) return MemoryBufferNULLPtr::MemoryBufferNUllChar;
  if(i >= l_length) i = l_length; // If not in bounds truncate to l_length
  return mptr[i];
}

unsigned char &MemoryBuffer::operator[](__MEMBUF_INT__ i) const
{
  // PC-lint 09/14/2005: Possible use of null pointer
  if(!mptr) return MemoryBufferNULLPtr::MemoryBufferNUllChar;
  if(i >= l_length) i = l_length; // If not in bounds truncate to l_length
  return mptr[i];
}
