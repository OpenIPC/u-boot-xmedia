/*
 * Copyright (c) XMEDIA. All rights reserved.
 */

#ifndef __AUDIO_AO_H__
#define __AUDIO_AO_H__

typedef enum {
    AUDIO_SAMPLE_RATE_8000   = 8000,    /* 8kHz samplerate*/
    AUDIO_SAMPLE_RATE_12000  = 12000,   /* 12kHz samplerate*/
    AUDIO_SAMPLE_RATE_11025  = 11025,   /* 11.025kHz samplerate*/
    AUDIO_SAMPLE_RATE_16000  = 16000,   /* 16kHz samplerate*/
    AUDIO_SAMPLE_RATE_22050  = 22050,   /* 22.050kHz samplerate*/
    AUDIO_SAMPLE_RATE_24000  = 24000,   /* 24kHz samplerate*/
    AUDIO_SAMPLE_RATE_32000  = 32000,   /* 32kHz samplerate*/
    AUDIO_SAMPLE_RATE_44100  = 44100,   /* 44.1kHz samplerate*/
    AUDIO_SAMPLE_RATE_48000  = 48000,   /* 48kHz samplerate*/
    AUDIO_SAMPLE_RATE_BUTT,
} audio_sample_rate;

#endif