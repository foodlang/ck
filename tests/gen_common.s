[global _main]
_main:
    push r15
    push rbx
    push rcx
    push rdx
    pop rcx
    pop rbx
;autoblockaddr<ptr> %v3 <- %v0 
;{not implemented}
;zeroblock<ptr> %v3 <- %v24 
;{not implemented}
;store<ptr> %v2 <- %v3 
    mov [rbx], rbx
;return<b32> 
    nop
    pop rbx
    pop r15
    ret
