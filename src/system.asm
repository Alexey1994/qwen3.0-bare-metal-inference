org 0x8000


mov CX, 80*25
clear_screen:
	mov AX, 0x0E20
	int 0x10
	dec CX
	cmp CX, 0
jnz clear_screen


;enable A20
cli
in AL, 0x92
or AL, 2
out 0x92, AL


lgdt [GDT_pointer]

xor AX, AX
mov [saved_ESP+2], AX
mov [saved_ESP], SP
call switch_to_32_bits
use32

;mov EAX, 0xB8000
;mov word[EAX], 1 + 2*256


enable_sse:
    ; ---------------------------------------------------------
    ; ШАГ 1: Настройка CR0 (Control Register 0)
    ; Бит 1 (MP) = 1 : Monitor Coprocessor (нужен для WAIT/FWAIT)
    ; Бит 2 (EM) = 0 : Emulation (должен быть 0, иначе #UD при FPU/SSE)
    ; Бит 3 (TS) = 0 : Task Switched (сбрасываем, чтобы не было #NM)
    ; ---------------------------------------------------------
    mov     eax, cr0
    and     ax, 0xFFFB          ; Сброс бита 2 (CR0.EM = 0)
    or      ax, 0x0002          ; Установка бита 1 (CR0.MP = 1)
    and     ax, 0xFFF7          ; Сброс бита 3 (CR0.TS = 0)
    mov     cr0, eax

    ; ---------------------------------------------------------
    ; ШАГ 2: Настройка CR4 (Control Register 4)
    ; Бит 9  (OSFXSR)     = 1 : Операционная система поддерживает FXSAVE/FXRSTOR
    ;                           и SSE-инструкции. Без этого бита #UD!
    ; Бит 10 (OSXMMEXCPT) = 1 : ОС обрабатывает исключения SIMD (#XM).
    ;                           Рекомендуется включить вместе с OSFXSR.
    ; ---------------------------------------------------------
    mov     eax, cr4
    or      ax, 0x0600          ; Установка битов 9 и 10
    mov     cr4, eax

    ; ---------------------------------------------------------
    ; ШАГ 3: Инициализация состояния x87 FPU
    ; Устанавливает регистры управления FPU в значения по умолчанию.
    ; Обязательно, т.к. после аппаратного сброса состояние не определено.
    ; ---------------------------------------------------------
    fninit


push matmul_optimized
push read_sector
call main
add ESP, 4

call switch_to_16_bits
use16

;mov AX, 0x0E21
;int 0x10

ret


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

align 4
saved_EIP: dd 0
saved_ESP: dd 0

use32
switch_to_16_bits:
	cli

	pop dword [saved_EIP]
	mov [saved_ESP], ESP
	xor ESP, ESP
	
	lidt [idtr_16]

	mov AX, 32
	mov DS, AX
	mov SS, AX
	mov ES, AX
	mov FS, AX
	mov GS, AX

	jmp 24:p_16_bit
	
	align 16
	use16
	p_16_bit:

	mov EAX, CR0
	and EAX, 0xFFFFFFFE
	mov CR0, EAX

	jmp 0:r_16_bit
	
	r_16_bit:
	mov AX, CS
	mov DS, AX
	mov SS, AX
	mov ES, AX
	mov FS, AX
	mov GS, AX
	
	mov SP, 0x1000
	push word [saved_EIP]

	;pop AX
	;pop BX
	;push AX
	
	mov BX, 0x0870
	call reset_pic

	sti
	ret


;in saved_ESP
use16
switch_to_32_bits:
	cli

	mov BX, 0x2028
	call reset_pic
	
	lidt [IDT_pointer]

	mov EAX, CR0
	or EAX, 1
	mov CR0, EAX
	
	jmp 8:setup_32_bit_segment_registers

	use32
	setup_32_bit_segment_registers:
	mov EAX, 16
	mov DS, EAX
	mov SS, EAX
	mov ES, EAX
	mov FS, EAX
	mov GS, EAX
	
	xor EAX, EAX
	pop AX
	mov ESP, [saved_ESP]
	push EAX
	
	sti
	ret


%macro out_8 2
	mov AL, %2
	out %1, AL
%endmacro

use16
;BX - PIC setting, 0x0870 for 16 bit, 0x2028 for 32
reset_pic:
	out_8 0x20, 0x11
	out_8 0x21, BH
	out_8 0x21, 0x04
	out_8 0x21, 0x01
	out_8 0x21, 0x00
	
	out_8 0xA0, 0x11
	out_8 0xA1, BL
	out_8 0xA1, 0x02
	out_8 0xA1, 0x01
	out_8 0xA1, 0x00
	ret


align 4
idtr_16:
	dw 0x3FF
	dd 0


align 16
GDT:
	; dummy
	dq 0

	; CODE (CS register = 8)
	dw 0xFFFF     ; размер сегмента
	dw 0          ; базовый адрес
	db 0          ; базовый адрес
	db 0b10011010 ; 1    сегмент правильный(должно быть 1)
	              ; 00   уровень привилегий(меньше - больше привилегий)
	              ; 1    если сегмент в памяти то 1
	              ; 1    сегмент исполняемый
	              ; 0    направление для сегмента данных либо возможность перехода с низких привилегий на высокие для сегмента кода(1 - разрешено, 0 - запрещено)
	              ; 1    разрешение на чтение для сегмента кода, разрешение на запись для сегмента данных
	              ; 0    бит доступа к сегменту, устанавливается процессором(рекомендуется 0)
	db 0b11001111 ; 1    гранулярность(если 0, то размер адреса равен размеру сегмента кода, если 1 то размеру сегмента кода * 4096)
	              ; 1    размер, если 0 и 64 битный режим(следующий бит) = 0, то селектор определяет 16 битный режим, если 1 - 32 битный. Если 64 битный режим равен 1, то должен быть равен 0(значение 1 зарезервировано, будет генерировать исключение)
	              ; 0    64 битный режим
	              ; 0    зарезервировано
	              ; 1111 размер сегмента
	db 0          ;      базовый адрес

	; DATA (DS register = 16)
	dw 0xffff     ; размер сегмента
	dw 0          ; базовый адрес
	db 0          ; базовый адрес
	db 0b10010010 ; 1    сегмент правильный(должно быть 1)
	              ; 00   уровень привилегий(меньше - больше привилегий)
	              ; 1    если сегмент в памяти то 1
	              ; 0    сегмент исполняемый
	              ; 0    направление для сегмента данных либо возможность перехода с низких привилегий на высокие для сегмента кода
	              ; 1    разрешение на чтение для сегмента кода, разрешение на запись для сегмента данных
	              ; 0    бит доступа к сегменту, устанавливается процессором(рекомендуется 0)
	db 0b11001111 ; 1    гранулярность(если 0, то размер адреса равен размеру сегмента кода, если 1 то размеру сегмента кода * 4096)
	              ; 1    размер, если 0 и 64 битный режим(следующий бит) = 0, то селектор определяет 16 битный режим, если 1 - 32 битный. Если 64 битный режим равен 1, то должен быть равен 0(значение 1 зарезервировано, будет генерировать исключение)
	              ; 0    64 битный режим
	              ; 0    зарезервировано
	              ; 1111 размер сегмента
	db 0          ;      базовый адрес
	
	; CODE16 (CS register = 24)
	dw 0xFFFF     ; размер сегмента
	dw 0          ; базовый адрес
	db 0          ; базовый адрес
	db 0b10011010 ; 1    сегмент правильный(должно быть 1)
	              ; 00   уровень привилегий(меньше - больше привилегий)
	              ; 1    0 - системный, 1 - код или данные
	              ; 1    сегмент исполняемый
	              ; 1    направление для сегмента данных либо возможность перехода с низких привилегий на высокие для сегмента кода(1 - разрешено, 0 - запрещено)
	              ; 1    разрешение на чтение для сегмента кода, разрешение на запись для сегмента данных
	              ; 0    бит доступа к сегменту, устанавливается процессором(рекомендуется 0)
	db 0b00001111 ; 0    гранулярность(если 0, то размер адреса равен размеру сегмента кода, если 1 то размеру сегмента кода * 4096)
	              ; 0    размер, если 0 и 64 битный режим(следующий бит) = 0, то селектор определяет 16 битный режим, если 1 - 32 битный. Если 64 битный режим равен 1, то должен быть равен 0(значение 1 зарезервировано, будет генерировать исключение)
	              ; 0    64 битный режим
	              ; 0    зарезервировано
	              ; 1111 размер сегмента
	db 0          ;      базовый адрес

	; DATA16 (DS register = 32)
	dw 0xFFFF     ; размер сегмента
	dw 0          ; базовый адрес
	db 0          ; базовый адрес
	db 0b10010010 ; 1    сегмент правильный(должно быть 1)
	              ; 00   уровень привилегий(меньше - больше привилегий)
	              ; 1    0 - системный, 1 - код или данные
	              ; 0    сегмент исполняемый
	              ; 0    направление для сегмента данных либо возможность перехода с низких привилегий на высокие для сегмента кода
	              ; 1    разрешение на чтение для сегмента кода, разрешение на запись для сегмента данных
	              ; 0    бит доступа к сегменту, устанавливается процессором(рекомендуется 0)
	db 0b00001111 ; 0    гранулярность(если 0, то размер адреса равен размеру сегмента кода, если 1 то размеру сегмента кода * 4096)
	              ; 0    размер, если 0 и 64 битный режим(следующий бит) = 0, то селектор определяет 16 битный режим, если 1 - 32 битный. Если 64 битный режим равен 1, то должен быть равен 0(значение 1 зарезервировано, будет генерировать исключение)
	              ; 0    64 битный режим
	              ; 0    зарезервировано
	              ; 1111 размер сегмента
	db 0          ;      базовый адрес

GDT_pointer:
	dw $ - GDT ;размер
	dd GDT     ;адрес


%macro Trap_Desc 3
	dw %1  ; handler_address_low
	dw %2  ; selector
	db 0   ; zero
	db %3  ; attributes
	dw 0   ; handler_address_high
%endmacro

align 16
IDT:
	%rep 256
	Trap_Desc empty_interrupt_handler, 8, 0x8E
	%endrep
	

IDT_pointer:
	dw $ - IDT ;размер
	dd IDT     ;адрес


use32
empty_interrupt_handler:
	pusha
	mov AL, 0x20
	out 0x20, AL
	out 0xA0, AL
	popa
	iret


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

use32
read_sector:
	mov EAX, [ESP+4]
	mov [start_sector_low], EAX


	call switch_to_16_bits
	use16
	
	mov AH, 42h
	mov DL, 0x80
	mov SI, LBA_packet
	int 13h
	
	call switch_to_32_bits
	use32
	
	ret
	
	LBA_packet:
		size:                   db 16
		zero:                   db 0
		number_of_sectors:      dw 127
		buffer_address_offset:  dw 0
		buffer_address_segment: dw 0x2000
		start_sector_low:       dd 0
		start_sector_high:      dd 0


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;



; ============================================================================
; void matmul_optimized(float* C, float* A, float* B, int M, int K, int N)
;
; 32-bit x86, cdecl, bare-metal.
; Требует SSE (CR4.OSFXSR = 1).
; Компиляция: nasm -f elf32 matmul.asm
;             или nasm -f win32 matmul.asm (для TCC под Windows)
; ============================================================================

bits 32

;section .text
;global matmul_optimized
;export matmul_optimized

; Смещения аргументов (cdecl, после push ebp):
%define ARG_C       [ebp + 8]
%define ARG_A       [ebp + 12]
%define ARG_B       [ebp + 16]
%define ARG_M       [ebp + 20]
%define ARG_K       [ebp + 24]
%define ARG_N       [ebp + 28]

; Локальные переменные на стеке:
%define LOC_K_BYTES     [esp + 0]
%define LOC_PTR_A_ROW   [esp + 4]
%define LOC_PTR_C_ROW   [esp + 8]
%define LOC_PTR_B_COL   [esp + 12]
%define LOC_I           [esp + 16]
%define LOC_J           [esp + 20]

matmul_optimized:
    push    ebp
    mov     ebp, esp
    sub     esp, 24             ; 6 × 4 байта для локальных переменных

    push    ebx                 ; callee-saved
    push    esi                 ; callee-saved
    push    edi                 ; callee-saved

    ; Проверяем M > 0
    mov     eax, ARG_M
    test    eax, eax
    jle     .exit

    ; K_bytes = K * 4
    mov     eax, ARG_K
    shl     eax, 2
    mov     LOC_K_BYTES, eax

    ; i = 0
    mov     dword LOC_I, 0

.L_i:
    mov     ecx, LOC_I
    cmp     ecx, ARG_M
    jge     .exit

    ; ptr_a_row = A + i * K_bytes
    mov     eax, ecx
    imul    eax, LOC_K_BYTES
    add     eax, ARG_A
    mov     LOC_PTR_A_ROW, eax

    ; ptr_c_row = C + i * N * 4
    mov     eax, ecx
    imul    eax, ARG_N
    shl     eax, 2
    add     eax, ARG_C
    mov     LOC_PTR_C_ROW, eax

    ; j = 0
    mov     dword LOC_J, 0

.L_j:
    mov     edx, LOC_J
    cmp     edx, ARG_N
    jge     .L_next_i

    ; ptr_b_col = B + j * K_bytes
    ; (Предполагается, что B транспонирована, как в оригинале)
    mov     eax, edx
    imul    eax, LOC_K_BYTES
    add     eax, ARG_B
    mov     LOC_PTR_B_COL, eax

    ; sum = 0.0f
    xorps   xmm0, xmm0

    ; K_vec = K / 4
    mov     ecx, ARG_K
    shr     ecx, 2
    jz      .L_hsum

    ; cursor_a и cursor_b
    mov     esi, LOC_PTR_A_ROW
    mov     edi, LOC_PTR_B_COL

.L_k_vec:
    movups  xmm1, [esi]         ; 4 floats из A
    movups  xmm2, [edi]         ; 4 floats из B
    mulps   xmm1, xmm2          ; поэлементное умножение
    addps   xmm0, xmm1          ; накопление суммы
    add     esi, 16             ; += 4 * sizeof(float)
    add     edi, 16
    dec     ecx
    jnz     .L_k_vec

.L_hsum:
    ; Горизонтальная сумма xmm0: [s3, s2, s1, s0] -> s0+s1+s2+s3
    movaps  xmm1, xmm0
    shufps  xmm1, xmm1, 0x0E    ; [s1, s0, s3, s2]
    addps   xmm0, xmm1          ; [s1+s3, s0+s2, s1+s3, s0+s2]
    movaps  xmm1, xmm0
    shufps  xmm1, xmm1, 0x01    ; [s0+s2, ..., ..., ...]
    addss   xmm0, xmm1          ; xmm0[0] = итоговая сумма

    ; K_rem = K % 4
    mov     ecx, ARG_K
    and     ecx, 3
    jz      .L_store

    ; Вычисляем смещение после векторной части: (K / 4) * 16
    mov     eax, ARG_K
    shr     eax, 2
    shl     eax, 4
    mov     esi, LOC_PTR_A_ROW
    add     esi, eax
    mov     edi, LOC_PTR_B_COL
    add     edi, eax

.L_k_scalar:
    movss   xmm1, [esi]
    movss   xmm2, [edi]
    mulss   xmm1, xmm2
    addss   xmm0, xmm1
    add     esi, 4
    add     edi, 4
    dec     ecx
    jnz     .L_k_scalar

.L_store:
    ; C[i][j] = sum
    mov     edi, LOC_PTR_C_ROW
    mov     eax, LOC_J
    movss   [edi + eax*4], xmm0

    ; j++
    inc     dword LOC_J
    jmp     .L_j

.L_next_i:
    ; i++
    inc     dword LOC_I
    jmp     .L_i

.exit:
    pop     edi
    pop     esi
    pop     ebx
    mov     esp, ebp
    pop     ebp
    ret                         ; caller чистит стек (cdecl)



align 32
main: