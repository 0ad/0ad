#include <wchar.h>

namespace AtlasMessage { class MessageHandler; }

ATLASDLLIMPEXP void Atlas_SetMessageHandler(AtlasMessage::MessageHandler*);
ATLASDLLIMPEXP void Atlas_StartWindow(wchar_t* type);

ATLASDLLIMPEXP void Atlas_GLSetCurrent(void* context);
ATLASDLLIMPEXP void Atlas_GLSwapBuffers(void* context);

ATLASDLLIMPEXP void Atlas_NotifyEndOfFrame();
