#include "precompiled.h"

#define USE_FFMPEG 0

#include "FFmpeg.h"

/*

Code originally taken from ffmpeg's output_example.c
This is all rather hacked together and unreliable. In particular, I should:
* Change the fprintf/exit error handling so that it works in Atlas.
* Send all the logging output to wx too.
* Support variable bitrate (set qscale to 1 (best) .. 31 (worst), sameq)
* See if other codecs (particularly lossless ones) could be made to work.
  (Currently it half assumes that it's passed a .mp4 filename.)
* See if other compression parameters would give better quality/speed/etc.
* Make the frame size variable.
* Tidy everything up a bit.

Please complain if I forget to do those things.

*/
#if USE_FFMPEG

#ifdef __GNUC__
// ugly hack to make recent versions of FFmpeg work
#define __STDC_CONSTANT_MACROS
#undef _STDINT_H
#undef _STDINT_H_
#include <stdint.h>
#endif

#include "FFmpeg.h"
#if _MSC_VER	// HACK
#define vsnprintf _vsnprintf
#endif

#ifdef _MSC_VER
# pragma warning(disable: 4100 4505 4510 4610)
// Doesn't have inttypes.h, so cheat before including the ffmpeg headers
typedef unsigned __int8 uint8_t;
typedef unsigned __int16 uint16_t;
typedef unsigned __int32 uint32_t;
typedef unsigned __int64 uint64_t;
typedef __int8 int8_t;
typedef __int16 int16_t;
typedef __int32 int32_t;
typedef __int64 int64_t;
#endif

#define WXUNUSED(arg)

extern "C" {
#include "ffmpeg/avformat.h"
#include "ffmpeg/swscale.h"
}
// (Have to use a sufficiently recent version to get swscale - it needs the ~x86 keyword on Gentoo)

struct VideoEncoderImpl
{
	int framerate;
	int bitrate;
	float duration;
	int width, height;

	AVStream *video_st;
	AVFormatContext *oc;
	AVOutputFormat *fmt;

	AVFrame *picture, *tmp_picture;
	uint8_t *video_outbuf;
	int frame_count, video_outbuf_size;

	VideoEncoderImpl()
	{
		video_st = NULL;
		oc = NULL;
		fmt = NULL;

		picture = NULL;
		tmp_picture = NULL;
		video_outbuf = NULL;
		frame_count = 0;
		video_outbuf_size = 0;
	}

	AVStream *add_video_stream(AVFormatContext *oc, int codec_id)
	{
		AVCodecContext *c;
		AVStream *st;

		st = av_new_stream(oc, 0);
		if (!st) {
			fprintf(stderr, "Could not alloc stream\n");
			exit(1);
		}

		c = st->codec;
		c->codec_id = (CodecID)codec_id;
		c->codec_type = CODEC_TYPE_VIDEO;

// 		c->bit_rate = bitrate*1000;
		// TODO: support compressed formats, using qscale

		c->width = width;
		c->height = height;

		c->time_base.den = framerate;
		c->time_base.num = 1;

// 		c->pix_fmt = PIX_FMT_YUV420P;
		c->pix_fmt = PIX_FMT_RGBA32;

		if (oc->oformat->flags & AVFMT_GLOBALHEADER)
			c->flags |= CODEC_FLAG_GLOBAL_HEADER;

		return st;
	}

	AVFrame *alloc_picture(int pix_fmt, int width, int height)
	{
		AVFrame *picture;
		uint8_t *picture_buf;
		int size;

		picture = avcodec_alloc_frame();
		if (!picture)
			return NULL;
		size = avpicture_get_size(pix_fmt, width, height);
		picture_buf = (uint8_t*)av_malloc(size);
		if (!picture_buf) {
			av_free(picture);
			return NULL;
		}
		avpicture_fill((AVPicture *)picture, picture_buf,
					   pix_fmt, width, height);
		return picture;
	}

	void open_video(AVFormatContext *oc, AVStream *st)
	{
		AVCodec *codec;
		AVCodecContext *c;

		c = st->codec;

		/* find the video encoder */
		codec = avcodec_find_encoder(c->codec_id);
		if (!codec) {
			fprintf(stderr, "codec not found\n");
			exit(1);
		}

		/* open the codec */
		if (avcodec_open(c, codec) < 0) {
			fprintf(stderr, "could not open codec\n");
			exit(1);
		}

		video_outbuf = NULL;
		if (!(oc->oformat->flags & AVFMT_RAWPICTURE)) {
			/* allocate output buffer */
			/* XXX: API change will be done */
			/* buffers passed into lav* can be allocated any way you prefer,
			   as long as they're aligned enough for the architecture, and
			   they're freed appropriately (such as using av_free for buffers
			   allocated with av_malloc) */
			video_outbuf_size = c->width * c->height * 4;
			video_outbuf = (uint8_t*)av_malloc(video_outbuf_size);
		}

		/* allocate the encoded raw picture */
		picture = alloc_picture(c->pix_fmt, c->width, c->height);
		if (!picture) {
			fprintf(stderr, "Could not allocate picture\n");
			exit(1);
		}

		/* if the output format is not YUV420P, then a temporary YUV420P
		   picture is needed too. It is then converted to the required
		   output format */
		tmp_picture = NULL;
		if (c->pix_fmt != PIX_FMT_RGB24) {
			tmp_picture = alloc_picture(PIX_FMT_RGB24, c->width, c->height);
			if (!tmp_picture) {
				fprintf(stderr, "Could not allocate temporary picture\n");
				exit(1);
			}
		}
	}

	void copy_rgb_image(AVFrame *pict, int width, int height, const unsigned char* buffer)
	{
		for(int y=0;y<height;y++) {
			memcpy(&pict->data[0][y * pict->linesize[0]], buffer+y*width*3, width*3);
		}
	}

	void write_video_frame(AVFormatContext *oc, AVStream *st, const unsigned char* buffer)
	{
		AVCodecContext *c;
		static struct SwsContext *img_convert_ctx;
		int out_size, ret;

		c = st->codec;

		if (!buffer || frame_count >= (int)(duration*framerate)) {
			/* no more frame to compress. The codec has a latency of a few
			   frames if using B frames, so we get the last frames by
			   passing the same picture again */
		} else {
			if (c->pix_fmt != PIX_FMT_RGB24) {
				/* as we only generate a YUV420P picture, we must convert it
				   to the codec pixel format if needed */
				if (img_convert_ctx == NULL) {
					img_convert_ctx = sws_getContext(c->width, c->height,
													 PIX_FMT_RGB24,
													 c->width, c->height,
													 c->pix_fmt,
													 SWS_BICUBIC, NULL, NULL, NULL);
					if (img_convert_ctx == NULL) {
						fprintf(stderr, "Cannot initialize the conversion context\n");
						exit(1);
					}
				}
				copy_rgb_image(tmp_picture, c->width, c->height, buffer);
				sws_scale(img_convert_ctx, tmp_picture->data, tmp_picture->linesize,
						  0, c->height, picture->data, picture->linesize);
			} else {
				copy_rgb_image(picture, c->width, c->height, buffer);
			}
		}

		if (oc->oformat->flags & AVFMT_RAWPICTURE) {
			/* raw video case. The API will change slightly in the near
			   future for that */
			AVPacket pkt;
			av_init_packet(&pkt);

			pkt.flags |= PKT_FLAG_KEY;
			pkt.stream_index= st->index;
			pkt.data= (uint8_t *)picture;
			pkt.size= sizeof(AVPicture);

			ret = av_write_frame(oc, &pkt);
		} else {
			/* encode the image */
			out_size = avcodec_encode_video(c, video_outbuf, video_outbuf_size, picture);
			/* if zero size, it means the image was buffered */
			if (out_size > 0) {
				AVPacket pkt;
				av_init_packet(&pkt);

				if (c->coded_frame->pts == AV_NOPTS_VALUE)
					pkt.pts = AV_NOPTS_VALUE;
				else
					pkt.pts = av_rescale_q(c->coded_frame->pts, c->time_base, st->time_base);
				if(c->coded_frame->key_frame)
					pkt.flags |= PKT_FLAG_KEY;
				pkt.stream_index= st->index;
				pkt.data= video_outbuf;
				pkt.size= out_size;

				/* write the compressed frame in the media file */
				ret = av_write_frame(oc, &pkt);
			} else {
				ret = 0;
			}
		}
		if (ret != 0) {
			fprintf(stderr, "Error while writing video frame\n");
			exit(1);
		}
		frame_count++;
	}

	void close_video(AVFormatContext *WXUNUSED(oc), AVStream *st)
	{
		avcodec_close(st->codec);
		av_free(picture->data[0]);
		av_free(picture);
		if (tmp_picture) {
			av_free(tmp_picture->data[0]);
			av_free(tmp_picture);
		}
		av_free(video_outbuf);
	}
};

//////////////////////////////////////////////////////////////////////////

void log(void* WXUNUSED(v), int i, const char* format, va_list ap)
{
	char buf[512];
	vsnprintf(buf, sizeof(buf), format, ap);
	buf[sizeof(buf)-1] = '\0';
	wxLogDebug(L"[%d] %hs", i, buf);
}

VideoEncoder::VideoEncoder(const wxString& filenameStr, int framerate, int bitrate, float duration, int width, int height)
: m(new VideoEncoderImpl)
{
	wxCharBuffer filename = filenameStr.ToAscii();
	m->framerate = framerate;
	m->bitrate = bitrate;
	m->duration = duration;
	m->width = width;
	m->height = height;

	/* initialize libavcodec, and register all codecs and formats */
	av_register_all();

	av_log_set_callback(&log);

	/* auto detect the output format from the name. default is mpeg. */
	m->fmt = guess_format(NULL, filename, NULL);
	if (!m->fmt) {
		fprintf(stderr, "Could not deduce output format from file extension: using MPEG.\n");
		m->fmt = guess_format("mpeg", NULL, NULL);
	}
	if (!m->fmt) {
		fprintf(stderr, "Could not find suitable output format\n");
		exit(1);
	}

	/* allocate the output media context */
	m->oc = av_alloc_format_context();
	if (!m->oc) {
		fprintf(stderr, "Memory error\n");
		exit(1);
	}
	m->oc->oformat = m->fmt;
	strncpy(m->oc->filename, filename, sizeof(m->oc->filename));
	m->oc->filename[sizeof(m->oc->filename) - 1] = '\0';

	/* add the audio and video streams using the default format codecs
	and initialize the codecs */
// 	m->video_st = NULL;
// 	if (m->fmt->video_codec != CODEC_ID_NONE) {
// 		m->video_st = m->add_video_stream(m->oc, m->fmt->video_codec);
// 	}
	m->video_st = m->add_video_stream(m->oc, CODEC_ID_FFV1);

	/* set the output parameters (must be done even if no
	parameters). */
	if (av_set_parameters(m->oc, NULL) < 0) {
		fprintf(stderr, "Invalid output format parameters\n");
		exit(1);
	}

	dump_format(m->oc, 0, filename, 1);

	/* now that all the parameters are set, we can open the audio and
	video codecs and allocate the necessary encode buffers */
	if (m->video_st)
		m->open_video(m->oc, m->video_st);

	/* open the output file, if needed */
	if (!(m->fmt->flags & AVFMT_NOFILE)) {
		if (url_fopen(&m->oc->pb, filename, URL_WRONLY) < 0) {
			fprintf(stderr, "Could not open '%s'\n", filename.data());
			exit(1);
		}
	}

	/* write the stream header, if any */
	av_write_header(m->oc);

}

void VideoEncoder::Frame(const unsigned char* buffer)
{
	double video_pts = (double)m->video_st->pts.val * m->video_st->time_base.num / m->video_st->time_base.den;
	if (video_pts >= m->duration)
		return;

	m->write_video_frame(m->oc, m->video_st, buffer);
}

VideoEncoder::~VideoEncoder()
{
	for(;;)
	{
		double video_pts = (double)m->video_st->pts.val * m->video_st->time_base.num / m->video_st->time_base.den;
		if (video_pts >= m->duration)
			break;

		m->write_video_frame(m->oc, m->video_st, NULL);
	}

	
	/* close each codec */
	if (m->video_st)
		m->close_video(m->oc, m->video_st);

	/* write the trailer, if any */
	av_write_trailer(m->oc);

	/* free the streams */
	for(unsigned int i = 0; i < m->oc->nb_streams; i++) {
		av_freep(&m->oc->streams[i]->codec);
		av_freep(&m->oc->streams[i]);
	}

	if (!(m->fmt->flags & AVFMT_NOFILE)) {
		/* close the output file */
		url_fclose(&m->oc->pb);
	}

	/* free the stream */
	av_free(m->oc);

	
	delete m;
}

#else // !USE_FFMPEG:

VideoEncoder::VideoEncoder(const wxString& WXUNUSED(filenameStr), int WXUNUSED(framerate), int WXUNUSED(bitrate), float WXUNUSED(duration), int WXUNUSED(width), int WXUNUSED(height))
{
}

void VideoEncoder::Frame(const unsigned char* WXUNUSED(buffer))
{
}

VideoEncoder::~VideoEncoder()
{
}

#endif // !USE_FFMPEG
