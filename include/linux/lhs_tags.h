
/******************************************************************************
* Copyright (c) XMEDIA. All rights reserved.
*
* File:             param_atags.h
*
* Author:           Lynn
* Created:          12/07/22
* Description:      Param tag API and data structure
*****************************************************************************/
#ifndef _PARAM_ATAGS_H
#define _PARAM_ATAGS_H

#include <asm/setup.h>

void set_param_data(const char *name, const char *buf, int buflen);
int get_param_tag_data(char *tagbuf);

void setup_atags(char *fdt, int fdt_size);
void setup_board_tags(struct tag **in_params);

#endif /* _PARAM_ATAGS_H  */
