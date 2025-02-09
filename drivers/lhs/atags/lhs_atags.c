/******************************************************************************
* File:             lhs_atags.c
*
* Author:           Lynn
* Created:          12/15/22
* Description:      lhs atags for lhs private kernel booting and boot params
*****************************************************************************/

#include <common.h>
#include <command.h>
#include <asm/global_data.h>
#include <asm/io.h>
#include <linux/libfdt.h>
#include <image.h>
#include <asm/cache.h>
#include <bootm.h>
#include <usb.h>
#include <hw_decompress.h>
#include <xmedia_timestamp.h>
#include <irq_func.h>
#include <dm/device.h>
#include <dm/root.h>
#include <asm/setup.h>
#include <linux/sizes.h>

#include <linux/lhs_tags.h>

/* Support ATAGs */
static struct tag *g_params;
/************************************************************************************************/
static void setup_start_tag(bd_t *bd)
{
	g_params = (struct tag *)bd->bi_boot_params;

	g_params->hdr.tag = ATAG_CORE;
	g_params->hdr.size = tag_size(tag_core);

	g_params->u.core.flags = 0;
	g_params->u.core.pagesize = 0;
	g_params->u.core.rootdev = 0;

	g_params = tag_next(g_params);
}

static void setup_commandline_tag(bd_t *bd, char *commandline)
{
	char *p;

	if (!commandline)
		return;

	/* eat leading white space */
	for (p = commandline; *p == ' '; p++);

	/* skip non-existent command lines so the kernel will still
	 * use its default command line.
	 */
	if (*p == '\0')
		return;

	g_params->hdr.tag = ATAG_CMDLINE;
	g_params->hdr.size =
		(sizeof(struct tag_header) + strlen(p) + 1 + 4) >> 2;

	strcpy(g_params->u.cmdline.cmdline, p);

	g_params = tag_next(g_params);
}

static void setup_fdt_tag(bd_t *bd, char *fdt, int fdt_size)
{
	g_params->hdr.tag = ATAG_FDT;
	g_params->hdr.size = (sizeof(struct tag_header) + fdt_size) >> 2;
	memcpy((char *)&g_params->u, fdt, fdt_size);
	g_params = tag_next (g_params);
}

static void setup_end_tag(bd_t *bd)
{
	g_params->hdr.tag = ATAG_NONE;
	g_params->hdr.size = 0;
}

/******************************************************************************
* Function:         static void setup_param_tag
* Description:      To setup the PDM params ATAG in the global ATAG area
* In:
*                   bd_t *bd - global board info, may not be used
* Return:           bd_t *bd - global board info, may not be used
*****************************************************************************/
static void setup_param_tag(bd_t *bd)
{
        int length;

	/* Setup end item in param tag */
#define END_OF_PARAM_STR "End of param"
	set_param_data("end_tag", END_OF_PARAM_STR, sizeof(END_OF_PARAM_STR));

	/* Get all PDM params data from PDM atag */
        length = get_param_tag_data((char *)&g_params->u);
        if (length == 0) {
                return;
        }

        g_params->hdr.tag  = ATAG_PARAM;
        g_params->hdr.size = ((sizeof(struct tag_header) + length) >> 2); /* 2: size is num of words */

        g_params = tag_next(g_params);
}

/* Replace __weak void setup_board_tags(struct tag **in_params) {} */
void setup_board_tags(struct tag **in_params)
{
	debug("Setup board tags\n");
	g_params = *in_params;
	setup_param_tag(gd->bd);
	*in_params = g_params;
}

/******************************************************************************
* Function:         void setup_atags
* Description:      Setup all ATAGs to linux
* In:
*                   char *fdt - FDT(Device Tree Blob) to be packeted to ATAGs list
*                   int fdt_size - FDT size
* Return:           NA
*****************************************************************************/
void setup_atags(char *fdt, int fdt_size)
{
	debug("Setup LHS ATAGs\n");

	setup_start_tag(gd->bd);

#ifdef CONFIG_CMDLINE_TAG
	setup_commandline_tag(gd->bd, env_get("bootargs"));
#endif

	if (fdt)
		setup_fdt_tag(gd->bd, fdt, fdt_size);

	setup_board_tags(&g_params);

	setup_end_tag(gd->bd);
}
