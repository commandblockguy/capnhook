public	main_executor
public	main_executor_call_location
public	main_executor_call_destination
public	main_executor_size
public	individual_executor
public	individual_executor_table
public	individual_executor_jump
public	individual_executor_size

macro compare_hl_zero?
	add	hl,de
	or	a,a
	sbc	hl,de
end macro

scrapMem	equ 0D02AD7h
flags		equ	$D00080
flag_continue	equ	10

main_executor:
	push	af,bc,de,hl,iy
.loop:
	ld	hl,(ix)	; check if we've reached the null terminator
	compare_hl_zero
	jr	z,.exit

	ld	a,$83	; check if this is a valid hook
	cp	a,(hl)
	inc	hl
	jr	nz,.next

	xor	a,a	; reset our custom continue flag
	ld	(flags - flag_continue),a

	push	ix	; pointer to current table entry
	call	.execute
main_executor_call_location = $-3	; since we don't know where exactly this is running at assembly-time, we'll have to edit this while installing

	push	af,hl	; check if the return flag is set
	ld	hl,flags - flag_continue
	bit	0,(hl)
	jr	z,.return_value
	pop	hl,af,ix

.next:
	inc	ix	; next entry please
	inc	ix
	inc	ix
	jr	.loop

.return_value:
	pop	hl,af
	ld	(scrapMem),hl
	ld	hl,7*3	; we don't care about anything else on the stack anymore
	add	hl,sp
	ld	sp,hl
	ld	hl,(scrapMem)
	ret
.exit:
	pop	iy,hl,de,bc,af,ix
	ret

.execute:
main_executor_call_destination = $
	ld	ix,0
	add	ix,sp
	ld	de,(ix+9) ; original value of hl
	push	de
	ld	a,(ix+19) ; restore original registers
	ld	bc,(ix+15)
	ld	de,(ix+12)
	ld	iy,(ix+6)
	ld	ix,(iy+21)
	ex	(sp),hl
	ret
main_executor_size = $-main_executor

individual_executor:
	db	$83
	push	ix
	ld	ix,0
individual_executor_table = $-3
	jp	0
individual_executor_jump = $-3
individual_executor_size = $ - individual_executor
