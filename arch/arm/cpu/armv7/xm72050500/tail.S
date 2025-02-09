 /*
  * Copyright (c) XMEDIA. All rights reserved.
  */
 #include <config.h>

#ifndef CONFIG_NO_HEAD

.section .tail,#alloc

.align  9 /* align to 0x200 */

.globl _checked_area_end
_checked_area_end:


/* boot code area signature    */
_boot_sign_area:
        .fill 0x200,1,0

.globl _total_boot_area_end
  _total_boot_area_end:

#endif
