org 0x7C00

	mov AX, CS
	mov DS, AX
	mov SS, AX
	mov ES, AX
	mov SP, 0x7C00-2

	mov [word 0x7C00-2], DL

	call load_system

	mov SI, end
	call print_string

	xor AX, AX
	int 0x16
	int 0x19


load_system:
	mov AX, 1
repeat_read:
	mov BX, 0x600
	call read_sector
	jc read_error

	mov SI, 0x600
next_name:
	mov DI, system
	mov CX, 12
	rep cmpsb
	cmp CX, 0
	jz load_file
	add SI, CX
	add SI, 4
	cmp SI, 0x600 + 512 - 16
	jnz next_name

	mov AX, [0x600+510]
	cmp AX, 0
	jnz repeat_read

	mov SI, system
	call print_string
	mov SI, not_found_message
	call print_string
	ret

read_error:
	mov SI, read_error_message
	call print_string
	ret

load_file:
	mov DI, 0x8000
load_next_file_chunk:
	cmp word[SI], 0
	jz end_load_file

	mov AX, [SI]
	mov BX, 0x600
	call read_sector
	jc read_error

	mov SI, 0x600
	mov CX, 254
	rep movsw

	mov SI, 0x600 + 508
	jmp load_next_file_chunk

end_load_file:
	call 0x8000
	ret


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


;in  SI - printed string
print_string:
	lodsb
	cmp AL, 0
	jz end_print_string
	mov AH, 0x07
	int 0x10
	jmp print_string
end_print_string:
	ret


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


;in AX - sector_num
;in BX - buffer
read_sector:
	mov [start_sector_low], AX
	mov [buffer_address_offset], BX

	mov AH, 0x42
	mov DL, [0x7C00-2]
	mov SI, LBA_packet
	int 0x13
	ret

	LBA_packet:
		size:                   db 16
		zero:                   db 0
		number_of_sectors:      dw 1
		buffer_address_offset:  dw 0
		buffer_address_segment: dw 0
		start_sector_low:       dd 0
		start_sector_high:      dd 0


system: db 's','y','s','t','e','m',0,0,0,0,0,0,0,0
not_found_message: db ' ','n','o','t',' ','f','o','u','n','d',13,10,0
end: db 'p','r','e','s','s',' ','a','n','y',' ','k','e','y',' ','t','o',' ','r','e','b','o','o','t',13,10,0
read_error_message: db ' ','r','e','a','d',' ','e','r','r','o','r',13,10,0


align 0x7C00 + 510
db 0x55, 0xAA