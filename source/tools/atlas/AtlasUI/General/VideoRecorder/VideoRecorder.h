#ifndef INCLUDED_VIDEORECORDER
#define INCLUDED_VIDEORECORDER

class VideoRecorder
{
public:
	static void RecordCinematic(wxWindow* window, const wxString& trackName, float duration);
};

#endif // INCLUDED_VIDEORECORDER
