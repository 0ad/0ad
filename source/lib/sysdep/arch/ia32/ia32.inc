; disable executable stack
%ifidn __OUTPUT_FORMAT__,elf
section .note.GNU-stack noalloc noexec nowrite progbits
%endif
%ifidn __OUTPUT_FORMAT__,elf64
section .note.GNU-stack noalloc noexec nowrite progbits
%endif

; set section attributes
; section .data data align=32 use32
; section .bss  bss  align=16 use32
; section .text code align=64 use32
; activate .text (needs to be separate because __SECT__ will otherwise
; complain that the above definition is redeclaring attributes)
section .text

; Usage:
; use sym(ia32_cap) instead of _ia32_cap - on relevant platforms, sym() will add
; the underlines automagically, on others it won't
%ifdef DONT_USE_UNDERLINE
%define sym(a) a
%else
%define sym(a) _ %+ a
%endif
