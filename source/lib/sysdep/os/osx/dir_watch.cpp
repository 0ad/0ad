/* Copyright (C) 2014 Wildfire Games.
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

#include "precompiled.h"

#include "lib/sysdep/dir_watch.h"
#include "lib/file/file_system.h"
#include "osx_sys_version.h"

#include "lib/os_path.h"
#include "lib/file/file.h"
#include "lib/posix/posix_filesystem.h" // mode_t

#include <AvailabilityMacros.h> // MAC_OS_X_VERSION_MIN_REQUIRED
#include <CoreFoundation/CoreFoundation.h>
#include <CoreServices/CoreServices.h>


#include "ps/CLogger.h"

static FSEventStreamRef g_Stream = NULL;

struct DirWatch
{
  OsPath path;
  int reqnum;
};

typedef std::vector<DirWatch> DirWatchMap;
static DirWatchMap g_Paths;
static DirWatchMap g_RootPaths;
static DirWatchNotifications g_QueuedDirs;

static bool CanRunNotifications()
{
  int major = 0;
  int minor = 0;
  int bugfix = 0;

  GetSystemVersion( major, minor, bugfix);

  if ((major == 10 && minor >= 7) || major >= 11)
    return true;

  return false;
}

#if MAC_OS_X_VERSION_MIN_REQUIRED < 1070
  #define kFSEventStreamCreateFlagFileEvents  0x00000010
  #define kFSEventStreamEventFlagItemIsFile   0x00010000
  #define kFSEventStreamEventFlagItemRemoved  0x00000200
  #define kFSEventStreamEventFlagItemRenamed  0x00000800
  #define kFSEventStreamEventFlagItemCreated  0x00000100
  #define kFSEventStreamEventFlagItemModified 0x00001000
#endif

static void fsevent_callback(
    ConstFSEventStreamRef UNUSED(streamRef),
    void * UNUSED(clientCallBackInfo),
    size_t numEvents,
    void *eventPaths,
    const FSEventStreamEventFlags eventFlags[],
    const FSEventStreamEventId UNUSED(eventIds)[] )
{
    unsigned long i;
    char **paths = (char **)eventPaths;

    for (i=0; i<numEvents; i++)
    {
      bool    isWatched = false;
      OsPath eventPath  = OsPath(paths[i]);
      unsigned long eventType = eventFlags[i];

      if ( eventPath.Filename().string().c_str()[0] != '.' )
      {
        for ( DirWatchMap::iterator it = g_Paths.begin() ; it != g_Paths.end(); ++it)
          if ( path_is_subpath( it->path.string().c_str(), eventPath.string().c_str() ) )
            isWatched = true;
      }

      if ( ! isWatched )
        return;

      OsPath filename = Path( eventPath.string().c_str() );

      if ( eventType & kFSEventStreamEventFlagItemIsFile)
      {
        if ( eventType & kFSEventStreamEventFlagItemRemoved )
          g_QueuedDirs.push_back(DirWatchNotification( filename.string().c_str(), DirWatchNotification::Deleted ));
        else if ( eventType & kFSEventStreamEventFlagItemRenamed )
          g_QueuedDirs.push_back(DirWatchNotification( filename.string().c_str(), DirWatchNotification::Deleted ));
        else if ( eventType & kFSEventStreamEventFlagItemCreated )
          g_QueuedDirs.push_back(DirWatchNotification( filename.string().c_str(), DirWatchNotification::Created ));
        else if ( eventType & kFSEventStreamEventFlagItemModified )
          g_QueuedDirs.push_back(DirWatchNotification( filename.string().c_str(), DirWatchNotification::Changed ));
      }
    }

}

static FSEventStreamRef CreateEventStream( DirWatchMap path )
{
  if ( ( g_Stream == NULL ) && CanRunNotifications() && !path.empty() )
  {
    CFStringRef* pathLists = (CFStringRef*)malloc( sizeof(CFStringRef*) * path.size() );
    int   index = 0;
    for ( DirWatchMap::iterator it = path.begin() ; it != path.end(); ++it)
    {
      pathLists[index] = CFStringCreateWithFileSystemRepresentation( NULL, OsString(it->path).c_str());
      index++;
    }
    CFArrayRef pathsToWatch = CFArrayCreate(NULL, (const void **)pathLists, index, NULL);

    FSEventStreamContext *callbackInfo = NULL;

    FSEventStreamRef stream = FSEventStreamCreate(NULL, &fsevent_callback, callbackInfo, pathsToWatch,
        kFSEventStreamEventIdSinceNow, 1.0, kFSEventStreamCreateFlagFileEvents );

    CFRelease( pathsToWatch );
    free( pathLists );

    FSEventStreamScheduleWithRunLoop(stream, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);
    if (!FSEventStreamStart(stream))
      debug_warn(L"event_loop FSEventStreamStart failed!");
    else
      return stream;
  }
  return NULL;
}

static void DeleteEventStream()
{
  if ( g_Stream != NULL )
  {
    FSEventStreamStop(g_Stream);
    FSEventStreamInvalidate(g_Stream);
    FSEventStreamRelease(g_Stream);

    g_Stream = NULL;
  }
}


Status dir_watch_Add(const OsPath& path, PDirWatch& dirWatch)
{
  PDirWatch tmpDirWatch(new DirWatch);
  dirWatch.swap(tmpDirWatch);
  dirWatch->path = path;
  dirWatch->reqnum = 0;
  g_Paths.push_back( *dirWatch );

  bool  alreadyInsideRootPath = false;
  for ( DirWatchMap::iterator it = g_RootPaths.begin() ; it != g_RootPaths.end(); ++it)
  {
    if ( path_is_subpath( path.string().c_str(), it->path.string().c_str() ) )
      alreadyInsideRootPath = true;
  }

  if ( !alreadyInsideRootPath )
  {
    DeleteEventStream();
    g_RootPaths.push_back( *dirWatch );
  }

	return INFO::OK;
}

Status dir_watch_Poll(DirWatchNotifications& notifications)
{
  if ( g_Stream == NULL )
  {
    g_Stream = CreateEventStream( g_RootPaths );
  }
  else
  {
    for ( DirWatchNotifications::iterator it = g_QueuedDirs.begin() ; it != g_QueuedDirs.end(); ++it)
      notifications.push_back(DirWatchNotification( *it ));

    g_QueuedDirs.clear();
  }

	return INFO::OK;
}
