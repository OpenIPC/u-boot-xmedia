/*
 * Copyright (c) XMEDIA. All rights reserved.
 */

.section .image,"a"

.globl g_ddr_training_cmd_start
g_ddr_training_cmd_start:
.incbin "drivers/ddr/xmedia/default/cmd_bin/ddr_cmd.bin"
.globl g_ddr_training_cmd_end
g_ddr_training_cmd_end:
