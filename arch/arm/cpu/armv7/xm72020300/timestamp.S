
#include <config.h>

#ifdef __ASSEMBLY__

.macro TIME_STAMP_STOPWATCH_ASM ctrl, value
	ldr \ctrl, =STOPWATCH_CTRL_REG
	ldr \value, =1
	str \value, [\ctrl]
.endm

/* The stack MUST be ready when using this macro */
.macro TIME_STAMP_TIMER_ASM func, line, type
	ldr \func, =0
	ldr \line, =0
	ldr \type, =0
	bl  timestamp_mark
.endm

#endif /* __ASSEMBLY__ */

