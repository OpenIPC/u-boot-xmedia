// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) XMEDIA. All rights reserved.
 */

#ifndef __XMEDIA_TIMESTAMP_H__
#define __XMEDIA_TIMESTAMP_H__

#include <config.h>
#include <linux/types.h>

#ifdef CONFIG_TARGET_XM720XXX
typedef	u64 timestamp_type;
#else
typedef	u32 timestamp_type;
#endif
/*
 * timestamps storage arrangement
 * |-----Timestamp Item----|-----Timestamp Item----| ...
 * |stamp|func |line |type |stamp|func |line |type | ...
 */
#pragma pack(4)
typedef struct {
	timestamp_type stamp;
	char *func;
	u32 line;
	u32 type;
} timestamp_item;
#pragma pack()

#define TIME_STAMP(type) timestamp_mark(__func__, __LINE__, type)

void timestamp_mark(const char *func, u32 line, u32 type);
void timestamp_print(u32 type);
void timestamp_clear(void);

void stopwatch_trigger(void);
void stopwatch_clear(void);
void stopwatch_print(void);
#endif /* __XMEDIA_TIMESTAMP_H__ */
