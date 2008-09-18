#include <wchar.h>

namespace AtlasMessage { class MessagePasser; }

ATLASDLLIMPEXP void Atlas_SetMessagePasser(AtlasMessage::MessagePasser*);
ATLASDLLIMPEXP void Atlas_StartWindow(const wchar_t* type);

ATLASDLLIMPEXP void Atlas_GLSetCurrent(void* context);
ATLASDLLIMPEXP void Atlas_GLSwapBuffers(void* context);

ATLASDLLIMPEXP void Atlas_NotifyEndOfFrame();

ATLASDLLIMPEXP void Atlas_DisplayError(const wchar_t* text, size_t flags);

ATLASDLLIMPEXP void Atlas_ReportError();

ATLASDLLIMPEXP void Atlas_ReportError();
