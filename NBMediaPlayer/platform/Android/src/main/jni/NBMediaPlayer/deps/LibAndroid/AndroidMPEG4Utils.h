#ifndef MPEG4_UTILS_H
#define MPEG4_UTILS_H

#include "AndroidJni.h"

static int MPEG4ProfileSimple              = 0x01;
static int MPEG4ProfileSimpleScalable      = 0x02;
static int MPEG4ProfileCore                = 0x04;
static int MPEG4ProfileMain                = 0x08;
static int MPEG4ProfileNbit                = 0x10;
static int MPEG4ProfileScalableTexture     = 0x20;
static int MPEG4ProfileSimpleFace          = 0x40;
static int MPEG4ProfileSimpleFBA           = 0x80;
static int MPEG4ProfileBasicAnimated       = 0x100;
static int MPEG4ProfileHybrid              = 0x200;
static int MPEG4ProfileAdvancedRealTime    = 0x400;
static int MPEG4ProfileCoreScalable        = 0x800;
static int MPEG4ProfileAdvancedCoding      = 0x1000;
static int MPEG4ProfileAdvancedCore        = 0x2000;
static int MPEG4ProfileAdvancedScalable    = 0x4000;
static int MPEG4ProfileAdvancedSimple      = 0x8000;

static inline int convertMPEG4ProfileFromFFmpegToAndroid(int profile) {
int androidProfile = MPEG4ProfileSimple;
switch(profile) {
case FF_PROFILE_MPEG4_SIMPLE:
    androidProfile = MPEG4ProfileSimple;
    break;
case FF_PROFILE_MPEG4_SIMPLE_SCALABLE:
    androidProfile = MPEG4ProfileSimpleScalable;
    break;
case FF_PROFILE_MPEG4_CORE:
    androidProfile = MPEG4ProfileCore;
    break;
case FF_PROFILE_MPEG4_MAIN:
    androidProfile = MPEG4ProfileMain;
    break;
case FF_PROFILE_MPEG4_N_BIT:
    androidProfile = MPEG4ProfileNbit;
    break;
case FF_PROFILE_MPEG4_SCALABLE_TEXTURE:
    androidProfile = MPEG4ProfileScalableTexture;
    break;
case FF_PROFILE_MPEG4_SIMPLE_FACE_ANIMATION:
    androidProfile = MPEG4ProfileSimpleFace;
    break;
case FF_PROFILE_MPEG4_BASIC_ANIMATED_TEXTURE:
    androidProfile = MPEG4ProfileBasicAnimated;
    break;
case FF_PROFILE_MPEG4_HYBRID:
    androidProfile = MPEG4ProfileHybrid;
    break;
case FF_PROFILE_MPEG4_ADVANCED_REAL_TIME:
    androidProfile = MPEG4ProfileAdvancedRealTime;
    break;
case FF_PROFILE_MPEG4_CORE_SCALABLE:
    androidProfile = MPEG4ProfileCoreScalable;
    break;
case FF_PROFILE_MPEG4_ADVANCED_CODING:
    androidProfile = MPEG4ProfileAdvancedCoding;
    break;
case FF_PROFILE_MPEG4_ADVANCED_CORE:
    androidProfile = MPEG4ProfileAdvancedCore;
    break;
case FF_PROFILE_MPEG4_ADVANCED_SCALABLE_TEXTURE:
    androidProfile = MPEG4ProfileAdvancedScalable;
    break;
case FF_PROFILE_MPEG4_SIMPLE_STUDIO:
    androidProfile = MPEG4ProfileSimpleFBA;
    break;
case FF_PROFILE_MPEG4_ADVANCED_SIMPLE:
    androidProfile = MPEG4ProfileAdvancedSimple;
default:
    //Fix me , never being here
    break;
}
return androidProfile;
}

static int MPEG4Level0      = 0x01;
static int MPEG4Level0b     = 0x02;
static int MPEG4Level1      = 0x04;
static int MPEG4Level2      = 0x08;
static int MPEG4Level3      = 0x10;
static int MPEG4Level4      = 0x20;
static int MPEG4Level4a     = 0x40;
static int MPEG4Level5      = 0x80;

static inline int convertMPEG4LevelFromFFmpegToAndroid(int level) {
int androidLevel = MPEG4Level3;
/*switch(level) {
case 9:
    androidLevel = MPEG4Level1b;
    break;
case 10:
    androidLevel = MPEG4Level1;
    break;
case 11:
    androidLevel = MPEG4Level11;
    break;
case 12:
    androidLevel = MPEG4Level12;
    break;
case 13:
    androidLevel = MPEG4Level13;
    break;
case 20:
    androidLevel = MPEG4Level2;
    break;
case 21:
    androidLevel = MPEG4Level21;
    break;
case 22:
    androidLevel = MPEG4Level22;
    break;
case 30:
    androidLevel = MPEG4Level3;
    break;
case 31:
    androidLevel = MPEG4Level31;
    break;
case 32:
    androidLevel = MPEG4Level32;
    break;
case 40:
    androidLevel = MPEG4Level4;
    break;
case 41:
    androidLevel = MPEG4Level41;
    break;
case 42:
    androidLevel = MPEG4Level42;
    break;
case 50:
    androidLevel  = MPEG4Level5;
    break;
case 51:
    androidLevel = MPEG4Level51;
    break;
 default:
    //Fix me , never being here
    break;
}*/
return androidLevel;
}

#endif