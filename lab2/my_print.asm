global	asm_print

	section .text
asm_print:
        push    ebp
        mov     ebp, esp
        mov     edx, [ebp+12]
        mov     ecx, [ebp+8]
        mov     ebx, 1
        mov     eax, 4
        int     80h
        pop     ebp
        ret

