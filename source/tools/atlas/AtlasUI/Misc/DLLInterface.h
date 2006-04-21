#include <wchar.h>

namespace AtlasMessage { class MessagePasser; }

ATLASDLLIMPEXP void Atlas_SetMessagePasser(AtlasMessage::MessagePasser*);
ATLASDLLIMPEXP void Atlas_StartWindow(wchar_t* type);

ATLASDLLIMPEXP void Atlas_GLSetCurrent(void* context);
ATLASDLLIMPEXP void Atlas_GLSwapBuffers(void* context);

ATLASDLLIMPEXP void Atlas_NotifyEndOfFrame();
