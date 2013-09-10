/* Copyright (c) 2013 Wildfire Games
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */


#import <AvailabilityMacros.h> // MAC_OS_X_VERSION_MIN_REQUIRED
#import <Foundation/Foundation.h>
#import <string>

#import "osx_bundle.h"

#define STRINGIZE2(id) # id
#define STRINGIZE(id) STRINGIZE2(id)

// Pass the bundle identifier string as a build option
#ifdef BUNDLE_IDENTIFIER
static const char* BUNDLE_ID_STR = STRINGIZE(BUNDLE_IDENTIFIER);
#else
static const char* BUNDLE_ID_STR = "";
#endif


void GetSystemVersion( int &major, int &minor, int &bugfix )
{
  // sensible default
  static int mMajor = 10;
  static int mMinor = 8;
  static int mBugfix = 0;

  static dispatch_once_t onceToken;
  dispatch_once(&onceToken, ^{
    NSString* versionString = [[NSDictionary dictionaryWithContentsOfFile:@"/System/Library/CoreServices/SystemVersion.plist"] objectForKey:@"ProductVersion"];
    NSArray* versions = [versionString componentsSeparatedByString:@"."];
    check( versions.count >= 2 );
    if ( versions.count >= 1 ) {
      mMajor = [[versions objectAtIndex:0] integerValue];
    }
    if ( versions.count >= 2 ) {
      mMinor = [[versions objectAtIndex:1] integerValue];
    }
    if ( versions.count >= 3 ) {
      mBugfix = [[versions objectAtIndex:2] integerValue];
    }
  });

  major = mMajor;
  minor = mMinor;
  bugfix = mBugfix;
}
