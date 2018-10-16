//
// Created by parallels on 9/9/18.
//

#ifndef NBFOUNDATION_H
#define NBFOUNDATION_H

#define NBMAX(a,b)    (((a) > (b)) ? (a) : (b))
#define NBMIN(a,b)    (((a) < (b)) ? (a) : (b))

void initialize_NBString();
void deinitialize_NBString();

extern const char *MEDIA_MIMETYPE_IMAGE_JPEG;

extern const char *MEDIA_MIMETYPE_VIDEO_VPX;
extern const char *MEDIA_MIMETYPE_VIDEO_AVC;
extern const char *MEDIA_MIMETYPE_VIDEO_MPEG4;
extern const char *MEDIA_MIMETYPE_VIDEO_H263;
extern const char *MEDIA_MIMETYPE_VIDEO_MPEG2;
extern const char *MEDIA_MIMETYPE_VIDEO_RAW;

extern const char *MEDIA_MIMETYPE_AUDIO_AMR_NB;
extern const char *MEDIA_MIMETYPE_AUDIO_AMR_WB;
extern const char *MEDIA_MIMETYPE_AUDIO_MPEG;           // layer III
extern const char *MEDIA_MIMETYPE_AUDIO_MPEG_LAYER_I;
extern const char *MEDIA_MIMETYPE_AUDIO_MPEG_LAYER_II;
extern const char *MEDIA_MIMETYPE_AUDIO_AAC;
extern const char *MEDIA_MIMETYPE_AUDIO_QCELP;
extern const char *MEDIA_MIMETYPE_AUDIO_VORBIS;
extern const char *MEDIA_MIMETYPE_AUDIO_G711_ALAW;
extern const char *MEDIA_MIMETYPE_AUDIO_G711_MLAW;
extern const char *MEDIA_MIMETYPE_AUDIO_RAW;
extern const char *MEDIA_MIMETYPE_AUDIO_FLAC;
extern const char *MEDIA_MIMETYPE_AUDIO_AAC_ADTS;

extern const char *MEDIA_MIMETYPE_CONTAINER_MPEG4;
extern const char *MEDIA_MIMETYPE_CONTAINER_WAV;
extern const char *MEDIA_MIMETYPE_CONTAINER_OGG;
extern const char *MEDIA_MIMETYPE_CONTAINER_MATROSKA;
extern const char *MEDIA_MIMETYPE_CONTAINER_MPEG2TS;
extern const char *MEDIA_MIMETYPE_CONTAINER_AVI;
extern const char *MEDIA_MIMETYPE_CONTAINER_MPEG2PS;

extern const char *MEDIA_MIMETYPE_CONTAINER_WVM;

extern const char *MEDIA_MIMETYPE_TEXT_3GPP;

extern const char *MEDIA_MIMETYPE_CONTAINER_FFMPEG;
extern const char *MEDIA_MIMETYPE_AUDIO_FFMPEG;
extern const char *MEDIA_MIMETYPE_VIDEO_FFMPEG;
extern const char *MEDIA_MIMETYPE_SUBTITLE_FFMPEG;

extern const char *MEDIA_MIMETYPE_CONTARNER_LIVE555;

extern const char *MEDIA_DECODE_COMPONENT_FFMPEG;
extern const char *MEDIA_DECODE_COMPONENT_VIDEOTOOLBOX;
extern const char *MEDIA_DECODE_COMPONENT_MEDIACODEC;

#endif //NBFOUNDATION_H
