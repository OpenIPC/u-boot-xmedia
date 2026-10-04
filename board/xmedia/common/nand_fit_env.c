// SPDX-License-Identifier: GPL-2.0+
/*
 * Shared by every xmedia board: brings a NAND environment saved by a
 * pre-FIT U-Boot up to date with the FIT boot (see nand_fit_env_migrate).
 */
#include <common.h>
#if defined(CONFIG_FIT) && defined(CONFIG_FMC_SPI_NAND)
#include <env.h>
#include <malloc.h>
#include "nand_fit_env.h"

/* The hardcoded root of either NAND layout, which ${rootargs} replaces. */
static const char * const nand_root_tokens[] = {
	"root=/dev/ubiblock0_1", "ubi.block=0,1",
	"root=ubi0:rootfs", "rootfstype=ubifs",
};

static int nand_root_token(const char *tok, size_t len)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(nand_root_tokens); i++)
		if (strlen(nand_root_tokens[i]) == len &&
		    !strncmp(tok, nand_root_tokens[i], len))
			return 1;
	return 0;
}

/*
 * Rewrite bootargs with its hardcoded root replaced by ${rootargs}, every
 * other argument kept in order.  Returns whether anything changed.
 */
static int nand_bootargs_migrate(const char *args)
{
	const char *p = args, *tok;
	char *out, *o;
	int placed = 0;
	size_t len;

	out = malloc(strlen(args) + sizeof(" ${rootargs}"));
	if (!out)
		return 0;
	o = out;
	while (*p) {
		while (*p == ' ')
			p++;
		tok = p;
		while (*p && *p != ' ')
			p++;
		len = p - tok;
		if (!len)
			break;
		if (nand_root_token(tok, len)) {
			if (placed)
				continue;
			tok = "${rootargs}";
			len = strlen(tok);
			placed = 1;
		}
		if (o != out)
			*o++ = ' ';
		memcpy(o, tok, len);
		o += len;
	}
	*o = '\0';
	if (placed)
		env_set("bootargs", out);
	free(out);
	return placed;
}

/*
 * An environment saved under a pre-FIT U-Boot overrides the new defaults.
 * Its verify=n, which those builds set on every boot, would turn the FIT
 * hash checks off, so drop it.  Its bootcmd and bootargs hardcode the root
 * of one layout -- the stock ubiblock one, or whatever the firmware's
 * allocator setup copied from /proc/cmdline -- and would not boot the
 * other.  A stock bootcmd is replaced by the one that picks the root from
 * the kernel image, and while that is the bootcmd in use, the root in
 * bootargs becomes ${rootargs}.  A bootcmd the owner edited is left alone,
 * and so are the bootargs it expands.
 *
 * The result is saved: Linux rewrites bootargs from the copy in flash
 * (fw_setenv), so a migration kept only in RAM would be undone on the
 * first boot.  The migrated env still boots under an older U-Boot, which
 * has no itest and so falls through to the ubiblock root, its only layout.
 */
void nand_fit_env_migrate(void)
{
	const char *cmd = env_get("bootcmd");
	const char *args;
	int changed = 0;

	if (env_get("verify")) {
		env_set("verify", NULL);
		changed = 1;
	}
	if (cmd && !strcmp(cmd, NAND_UBIBLOCK_BOOTCOMMAND)) {
		env_set("bootcmd", CONFIG_BOOTCOMMAND);
		cmd = env_get("bootcmd");
		changed = 1;
	}
	args = env_get("bootargs");
	if (cmd && !strcmp(cmd, CONFIG_BOOTCOMMAND) && args &&
	    !strstr(args, "${rootargs}"))
		changed |= nand_bootargs_migrate(args);

	if (changed) {
		printf("Env:   migrated for the FIT NAND layout\n");
		env_save();
	}
}
#endif
