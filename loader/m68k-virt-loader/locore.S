.global _start

_start:
	lea stack_top,%sp
	jsr cstart
loop:
	bra loop

.section .bootstrap_stack, "aw", @nobits
stack_bottom:
.skip 16384
stack_top:
