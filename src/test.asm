public	_user_hooks
public	_user_hook_size

flags		equ	0D00080h
flag_continue	equ	10

_user_hooks:
hook_1: ; causes all A keypresses to become B
	db	$83
	ld	hl,flags-flag_continue
	ld	(hl),1
	cp	a,$9A
	ret	nz
	inc	a
	ld	(hl),0
	ret
user_hook_size = $-_user_hooks

hook_2: ; causes all C keypresses to become D
	db	$83
	ld	hl,flags-flag_continue
	ld	(hl),1
	cp	a,$9C
	ret	nz
	inc	a
	ld	(hl),0
	ret

hook_3: ; causes all E keypresses to become F
	db	$83
	ld	hl,flags-flag_continue
	ld	(hl),1
	cp	a,$9E
	ret	nz
	inc	a
	ld	(hl),0
	ret

_user_hook_size:
	dl	user_hook_size