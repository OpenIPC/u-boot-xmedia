// SPDX-License-Identifier: GPL-2.0+
/*
 * Shared by every xmedia board: brings a NAND environment saved by a
 * pre-FIT U-Boot up to date with the FIT boot (see nand_fit_env_migrate).
 */
#include <common.h>
#if defined(CONFIG_FIT) && defined(CONFIG_FMC_SPI_NAND)
#include <command.h>
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
 * An environment saved under an earlier U-Boot overrides the new defaults.
 * Its verify=n, which those builds set on every boot, would turn the FIT
 * hash checks off, so drop it.  Its bootcmd reads a `kernel` volume -- the
 * stock one of the retired ubiblock layout, or of the first FIT layout --
 * and would not boot a kernel that lives in the rootfs.  Reinstalling such
 * a camera writes the new U-Boot and the new UBI image but keeps the env, so
 * a stock bootcmd is replaced by the one that boots /boot from the UBIFS
 * rootfs, and while that is the bootcmd in use, a root hardcoded in bootargs
 * -- stock, or copied there from /proc/cmdline by the firmware's allocator
 * setup -- becomes ${rootargs}.  A bootcmd the owner edited is left alone,
 * and so are the bootargs it expands.  bootm_size is added when missing,
 * and a partition table without a `ubi` partition is set back to the default.
 *
 * The result is saved: Linux rewrites bootargs from the copy in flash
 * (fw_setenv), so a migration kept only in RAM would be undone on the
 * first boot.  It is one-way -- an older U-Boot cannot boot the migrated
 * env -- which is why it waits for the new layout to be on the flash.
 */
/*
 * Whether the flash holds the current layout: a UBIFS rootfs with a kernel
 * in /boot.  Asked only before replacing a retired layout's stock bootcmd,
 * so a camera still on that layout -- given this U-Boot alone, or this one
 * loaded into RAM for a recovery -- keeps a bootcmd that boots it, and is
 * migrated only once its new UBI image is written.
 */
static int nand_kernel_in_rootfs(void)
{
	int ret;

	ret = !run_command("ubi part ubi && ubifsmount ubi0:rootfs && "
			   "ubifsls /boot/fitImage || ubifsls /boot/uImage", 0);
	run_command("ubifsumount", 0);
	return ret;
}

void nand_fit_env_migrate(void)
{
	const char *cmd = env_get("bootcmd");
	const char *args, *mtdparts;
	int changed = 0;

	if (env_get("verify")) {
		env_set("verify", NULL);
		changed = 1;
	}
	/*
	 * A FIT boot relocates the DTB under bootm_size; an environment saved
	 * before there was one would put it out of the kernel's lowmem.
	 */
	/*
	 * urnand and the boot command find the UBI partition by name; a saved
	 * table that has none gets the default one back, as the boot and env
	 * partitions are fixed by this U-Boot anyway.
	 */
	mtdparts = env_get("mtdparts");
	if (!mtdparts || !strstr(mtdparts, "(ubi)")) {
		env_set("mtdparts", NAND_MTDPARTS);
		changed = 1;
	}
	if (!env_get("bootm_size")) {
		env_set("bootm_size", "0x2000000");
		changed = 1;
	}
	if (cmd && (!strcmp(cmd, NAND_UBIBLOCK_BOOTCOMMAND) ||
		    !strcmp(cmd, NAND_FITVOL_BOOTCOMMAND)) &&
	    nand_kernel_in_rootfs()) {
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
