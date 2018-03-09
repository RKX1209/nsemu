.section .text, "e"
.global main
main:
	eor x2, x2, x2
        movz x1, 0xbeef
	mov x2, x1
        movk x2, 0xdead, lsl 16
	sub x3, x1, #0x431 // x3 = 0xbabe
	movk x4, 0xcafe, lsl 16 // x4 = 0xcafe0000
	orr x3, x3, x4 // x3 = 0xcafebabe
	cmp w9, 0
	subs wzr, w9, #0
	subs w31, w9, #0
	cmp x9, 0
	subs xzr, x9, #0
	subs x31, x9, #0
	//subs sp, x9, #0  assembler error!
	sub sp, x9, #0
        b main
