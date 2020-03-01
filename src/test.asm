public	_user_hooks
public	_user_hook_size

flag_continue	equ	10

_user_hooks:
hook_1: ; causes all A keypresses to become B
	db	$83
	set	0,(iy - flag_continue)
	cp	a,$9A
	ret	nz
	inc	a
	res	0,(iy - flag_continue)
	ret
user_hook_size = $-_user_hooks

hook_2: ; causes all C keypresses to become D
	db	$83
	set	0,(iy - flag_continue)
	cp	a,$9C
	ret	nz
	inc	a
	res	0,(iy - flag_continue)
	ret

hook_3: ; causes all E keypresses to become F
	db	$83
	set	0,(iy - flag_continue)
	cp	a,$9E
	ret	nz
	inc	a
	res	0,(iy - flag_continue)
	ret

_user_hook_size:
	dl	user_hook_size