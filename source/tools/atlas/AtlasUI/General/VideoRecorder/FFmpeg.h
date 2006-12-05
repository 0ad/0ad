#ifndef FFMPEG_H__
#define FFMPEG_H__

struct VideoEncoderImpl;

class VideoEncoder
{
public:
	VideoEncoder(const wxString& filename, int framerate, int bitrate, float duration);
	void Frame(const unsigned char* buffer);
	~VideoEncoder();

private:
	VideoEncoderImpl* m;
};

#endif // FFMPEG_H__
