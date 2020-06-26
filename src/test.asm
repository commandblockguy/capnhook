public _hook_1
public _hook_2
public _hook_3
public _hook_1_run
public _hook_2_run
public _hook_3_run
public _trigger_key_hook

include 'hook_equates.inc'

flag_continue	equ	10

_hook_1: ; causes all A keypresses to become B
	db	$83
	ld	hl,_hook_1_run
	ld	(hl),1
	set	0,(iy - flag_continue)
	cp	a,$9A
	ret	nz
	inc	a
	res	0,(iy - flag_continue)
	ret

_hook_2: ; causes all C keypresses to become D
	db	$83
	ld	hl,_hook_2_run
	ld	(hl),1
	set	0,(iy - flag_continue)
	cp	a,$9C
	ret	nz
	inc	a
	res	0,(iy - flag_continue)
	ret

_hook_3: ; causes all D keypresses to become E
	db	$83
	ld	hl,_hook_3_run
	ld	(hl),1
	set	0,(iy - flag_continue)
	cp	a,$9D
	ret	nz
	inc	a
	res	0,(iy - flag_continue)
	ret

_hook_1_run:	db	0
_hook_2_run:	db	0
_hook_3_run:	db	0

_trigger_key_hook:
	ld	iy,0D00080h
	xor	a,a
	ld	(_hook_1_run),a
	ld	(_hook_2_run),a
	ld	(_hook_3_run),a

	pop	hl
	pop	de
	push	de
	push	hl

	ld	hl,(rawKeyHookPtr)
	ld	a,$83
	cp	a,(hl)
	jr	nz,.invalid

	inc	hl
	ld	a,e
	jp	(hl)

.invalid:
	ld	a,-1
	ret
