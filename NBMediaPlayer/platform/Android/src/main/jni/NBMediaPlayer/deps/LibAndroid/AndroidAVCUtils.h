#ifndef AVC_UTILS_H
#define AVC_UTILS_H

#include "AndroidJni.h"

static int AVCProfileBaseline = 0x01;
static int AVCProfileMain     = 0x02;
static int AVCProfileExtended = 0x04;
static int AVCProfileHigh     = 0x08;
static int AVCProfileHigh10   = 0x10;
static int AVCProfileHigh422  = 0x20;
static int AVCProfileHigh444  = 0x40;

static inline int convertAVCProfileFromFFmpegToAndroid(int profile) {
    int androidProfile = AVCProfileBaseline;
    switch(profile) {
    case FF_PROFILE_H264_BASELINE:
    case FF_PROFILE_H264_CONSTRAINED_BASELINE:
        androidProfile = AVCProfileBaseline;
        break;
    case FF_PROFILE_H264_MAIN:
        androidProfile = AVCProfileMain;
        break;
    case FF_PROFILE_H264_EXTENDED:
        androidProfile = AVCProfileExtended;
        break;
    case FF_PROFILE_H264_HIGH:
        androidProfile = AVCProfileHigh;
        break;
    case FF_PROFILE_H264_HIGH_10:
    case FF_PROFILE_H264_HIGH_10_INTRA:
        androidProfile = AVCProfileHigh10;
        break;
    case FF_PROFILE_H264_HIGH_422:
    case FF_PROFILE_H264_HIGH_422_INTRA:
        androidProfile = AVCProfileHigh422;
        break;
    case FF_PROFILE_H264_HIGH_444:
    case FF_PROFILE_H264_HIGH_444_PREDICTIVE:
    case FF_PROFILE_H264_HIGH_444_INTRA:
    case FF_PROFILE_H264_CAVLC_444:
        androidProfile = AVCProfileHigh444;
        break;
    default:
        //Fix me , never being here
        break;
    }
    return androidProfile;
}

static int AVCLevel1       = 0x01;
static int AVCLevel1b      = 0x02;
static int AVCLevel11      = 0x04;
static int AVCLevel12      = 0x08;
static int AVCLevel13      = 0x10;
static int AVCLevel2       = 0x20;
static int AVCLevel21      = 0x40;
static int AVCLevel22      = 0x80;
static int AVCLevel3       = 0x100;
static int AVCLevel31      = 0x200;
static int AVCLevel32      = 0x400;
static int AVCLevel4       = 0x800;
static int AVCLevel41      = 0x1000;
static int AVCLevel42      = 0x2000;
static int AVCLevel5       = 0x4000;
static int AVCLevel51      = 0x8000;

static inline int convertAVCLevelFromFFmpegToAndroid(int level) {
    int androidLevel = AVCLevel1;
    switch(level) {
    case 9:
        androidLevel = AVCLevel1b;
        break;
    case 10:
        androidLevel = AVCLevel1;
        break;
    case 11:
        androidLevel = AVCLevel11;
        break;
    case 12:
        androidLevel = AVCLevel12;
        break;
    case 13:
        androidLevel = AVCLevel13;
        break;
    case 20:
        androidLevel = AVCLevel2;
        break;
    case 21:
        androidLevel = AVCLevel21;
        break;
    case 22:
        androidLevel = AVCLevel22;
        break;
    case 30:
        androidLevel = AVCLevel3;
        break;
    case 31:
        androidLevel = AVCLevel31;
        break;
    case 32:
        androidLevel = AVCLevel32;
        break;
    case 40:
        androidLevel = AVCLevel4;
        break;
    case 41:
        androidLevel = AVCLevel41;
        break;
    case 42:
        androidLevel = AVCLevel42;
        break;
    case 50:
        androidLevel  = AVCLevel5;
        break;
    case 51:
        androidLevel = AVCLevel51;
        break;
     default:
        //Fix me , never being here
        break;
    }
    return androidLevel;
}

        /* Parse the SPS/PPS Metadata and convert it to annex b format */
static int convertSpsPps( const uint8_t *p_buf, size_t i_buf_size,
                            uint8_t *p_out_buf, size_t i_out_buf_size,
                            size_t *p_sps_pps_size, size_t *p_nal_size) {
    // int i_profile;
    uint32_t i_data_size = i_buf_size, i_nal_size, i_sps_pps_size = 0;
    unsigned int i_loop_end;

    /* */
    if( i_data_size < 7 ) {
        return -1;
    }

    /* Read infos in first 6 bytes */
    // i_profile    = (p_buf[1] << 16) | (p_buf[2] << 8) | p_buf[3];
    if (p_nal_size)
        *p_nal_size  = (p_buf[4] & 0x03) + 1;
    p_buf       += 5;
    i_data_size -= 5;

    for ( unsigned int j = 0; j < 2; j++ ) {
        /* First time is SPS, Second is PPS */
        if( i_data_size < 1 ) {
            return -1;
        }
        i_loop_end = p_buf[0] & (j == 0 ? 0x1f : 0xff);
        p_buf++; i_data_size--;

        for ( unsigned int i = 0; i < i_loop_end; i++) {
            if( i_data_size < 2 ) {
                return -1;
            }

            i_nal_size = (p_buf[0] << 8) | p_buf[1];
            p_buf += 2;
            i_data_size -= 2;

            if( i_data_size < i_nal_size ) {
                return -1;
            }
            if( i_sps_pps_size + 4 + i_nal_size > i_out_buf_size ) {
                return -1;
            }

            p_out_buf[i_sps_pps_size++] = 0;
            p_out_buf[i_sps_pps_size++] = 0;
            p_out_buf[i_sps_pps_size++] = 0;
            p_out_buf[i_sps_pps_size++] = 1;

            memcpy( p_out_buf + i_sps_pps_size, p_buf, i_nal_size );
            i_sps_pps_size += i_nal_size;

            p_buf += i_nal_size;
            i_data_size -= i_nal_size;
        }
    }

    *p_sps_pps_size = i_sps_pps_size;

    return 0;
}


typedef struct H264ConvertState {
    uint32_t nal_len;
    uint32_t nal_pos;
} H264ConvertState;

static void convertH264ToAnnexb( uint8_t *p_buf, size_t i_len,
                                    size_t i_nal_size,
                                    H264ConvertState *state ) {
    if( i_nal_size < 3 || i_nal_size > 4 )
        return;

    /* This only works for NAL sizes 3-4 */
    while( i_len > 0 ) {
        if( state->nal_pos < i_nal_size ) {
            unsigned int i;
            for( i = 0; state->nal_pos < i_nal_size && i < i_len; i++, state->nal_pos++ ) {
                state->nal_len = (state->nal_len << 8) | p_buf[i];
                p_buf[i] = 0;
            }
            if( state->nal_pos < i_nal_size )
                return;
            p_buf[i - 1] = 1;
            p_buf += i;
            i_len -= i;
        }
        if( state->nal_len > INT_MAX )
            return;
        if( state->nal_len > i_len ) {
            state->nal_len -= i_len;
            return;
        } else {
            p_buf += state->nal_len;
            i_len -= state->nal_len;
            state->nal_len = 0;
            state->nal_pos = 0;
        }
    }
}

#endif