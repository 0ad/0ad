#include "posix.h"

// opaque structs are too hard to keep in sync with the real definition,
// and we don't want to expose the internals. therefore, use the pImpl pattern.

struct FAMRequest
{
	void* internal;
};

struct FAMConnection
{
	void* internal;
};



enum FAMChangeCode { FAMDeleted, FAMCreated, FAMChanged };

typedef struct
{
	FAMConnection* fc;
	FAMRequest fr;
	char filename[PATH_MAX];
	void* user;
	FAMChangeCode code;
}
FAMEvent;


extern int FAMOpen2(FAMConnection*, const char* app_name);
extern void FAMClose(FAMConnection*);

extern int FAMMonitorDirectory(FAMConnection*, const char* dir, FAMRequest* req, void* user);
extern void FAMCancelMonitor(FAMConnection*, FAMRequest* req);

extern int FAMPending(FAMConnection*);
extern int FAMNextEvent(FAMConnection*, FAMEvent* event);

