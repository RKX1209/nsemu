.section .text, "e"
.global main
main:
        adr x5, 0x1000
        add x0, x5, #0x30
        sub x1, x0, #0x20
        movz x3, 0xbeef
        movk x3, 0xdead, lsl 16
        b main
