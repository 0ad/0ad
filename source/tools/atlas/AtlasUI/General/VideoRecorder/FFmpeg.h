#ifndef INCLUDED_FFMPEG
#define INCLUDED_FFMPEG

struct VideoEncoderImpl;

class VideoEncoder
{
public:
	VideoEncoder(const wxString& filename, int framerate, int bitrate, float duration, int width, int height);
	void Frame(const unsigned char* buffer);
	~VideoEncoder();

private:
	VideoEncoderImpl* m;
};

#endif // INCLUDED_FFMPEG
