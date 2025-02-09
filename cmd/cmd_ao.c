/*
 * Copyright (c) XMEDIA. All rights reserved.
 */

/*
 * audio output Support
 */
#include <common.h>
#include <command.h>

#include "audio_ao.h"
#include "acodec.h"
#include "amp.h"
#include "ao.h"

#define CMD_AO_ARGS_BASE10 10
#define CMD_AO_ARGS_BASE16 16

audio_sample_rate g_sample_rate_list[] = {
    AUDIO_SAMPLE_RATE_8000,
    AUDIO_SAMPLE_RATE_11025,
    AUDIO_SAMPLE_RATE_12000,
    AUDIO_SAMPLE_RATE_16000,
    AUDIO_SAMPLE_RATE_22050,
    AUDIO_SAMPLE_RATE_24000,
    AUDIO_SAMPLE_RATE_32000,
    AUDIO_SAMPLE_RATE_44100,
    AUDIO_SAMPLE_RATE_48000
};

int ao_check_param(unsigned int addr, unsigned int size, audio_sample_rate sample_rate, unsigned int chn_cnt,
    int vol)
{
    unsigned int rate_flag = 0;
    unsigned int i;

    if (addr % 32) { /* 32: align */
        printf("[Error] Invalid addr parameter:0x%0x, address should be aligned by 32Byte!\n", addr);
        return -1;
    }

    if (size % 32) { /* 32: align */
        printf("[Warning] Invalid size parameter:0x%0x, size should be aligned by 32Byte!\n", size);
        return -1;
    }

    if (size == 0) {
        printf("[Warning] Invalid size parameter:0x%0x, size should be greater than 0!\n", size);
        return -1;
    }

    for (i = 0; i < sizeof(g_sample_rate_list) / sizeof(g_sample_rate_list[0]); i++) {
        if (sample_rate == g_sample_rate_list[i]) {
            rate_flag = 1;
            break;
        }
    }

    if (rate_flag == 0) {
        printf("[Warning] Invalid samplerate parameter.\n");
        return -1;
    }

    if (vol > 6 || vol < 0) { /* 6: max vol */
        printf("[Warning] Invalid volume parameter, range:[0, 6]dB!\n");
        return -1;
    }

    if ((chn_cnt == 0) || (chn_cnt > 2)) { /* 2: max chn */
        printf("[Warning] Invalid channelnum parameter, range:[1, 2].\n");
        return -1;
    }

    return 0;
}

int do_startao(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
    unsigned int addr, size;
    int vol;

    audio_sample_rate sample_rate;
    unsigned int chn_cnt;

    if (argc < 6) { /* 6: param num */
        printf("Insufficient parameter!\n");
        printf("Usage:\n%s\n", cmdtp->usage);
        return -1;
    }

    addr = (unsigned int)simple_strtoul(argv[1], NULL, CMD_AO_ARGS_BASE16);
    size = (unsigned int)simple_strtoul(argv[2], NULL, CMD_AO_ARGS_BASE16);        /* 2th arg */
    sample_rate = (unsigned int)simple_strtoul(argv[3], NULL, CMD_AO_ARGS_BASE10); /* 3th arg */
    chn_cnt = (unsigned int)simple_strtoul(argv[4], NULL, CMD_AO_ARGS_BASE10);     /* 4th arg */
    vol = (int)simple_strtol(argv[5], NULL, CMD_AO_ARGS_BASE10);                   /* 5th arg */
    if (ao_check_param(addr, size, sample_rate, chn_cnt, vol) != 0) {
        return -1;
    }

    acodec_device_init();
    acodec_i2s_set(sample_rate);
    udelay(100 * 1000); /* 100 * 1000 us */
    start_ao(addr, size, sample_rate, chn_cnt, vol);
    amp_unmute();

    printf("ao dev start ok!\n");
    return 0;
}

int do_stopao(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	amp_mute();
	stop_ao();

	printf("ao dev closed!\n");

	return 0;
}

U_BOOT_CMD(
	startao,    10,	1,  do_startao,
	"startao   - open interface of ao device.\n"
	"\t- startao [addr size samplerate channelnum volume]",
	"\nargs: [addr size samplerate channelnum volume]\n"
	"\t-<addr> : address of raw audio data,align by 32Byte\n"
	"\t-<size>: size of raw audio data,align by 32Byte\n"
	"\t-<samplerate>: sample rate of raw audio data\n"
	"\t-<channelnum>: channel number of raw audio data\n"
	"\t-<volume>: audio output volume default:0dB, range:0 ~ 6dB\n"
);

U_BOOT_CMD(
    stopao,    1,   0,  do_stopao,
    "stopao   - close interface of ao device.\n",
    "");