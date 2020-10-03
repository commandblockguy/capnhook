public _hook_1
public _hook_2
public _hook_3
public _hook_os

public _hook_1_run
public _hook_2_run
public _hook_3_run
public _hook_os_run

public _clear_hook
public _check_hook

public _trigger_key_hook
public _clear_key_hook
public _set_key_hook

include '../../src/hook_equates.inc'

flags		equ	$D00080
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

_hook_os: ; causes all F keypresses to become G
	db	$83
	ld	hl,_hook_os_run
	ld	(hl),1
	cp	a,$9F
	ret	nz
	inc	a
	ret

_hook_1_run:	db	0
_hook_2_run:	db	0
_hook_3_run:	db	0
_hook_os_run:	db	0

_trigger_key_hook:
	ld	iy,0D00080h
	xor	a,a
	ld	(_hook_1_run),a
	ld	(_hook_2_run),a
	ld	(_hook_3_run),a
	ld	(_hook_os_run),a

	pop	hl
	pop	de
	push	de
	push	hl

	ld	hl,(rawKeyHookPtr)
	ld	a,$83
	cp	a,(hl)
	jr	nz,.invalid

	push	ix

	inc	hl
	ld	a,e
	call	jp_hl
	pop	ix
	ret

.invalid:
	ld	a,-1
	ret

_clear_hook:
	pop	bc,de
	push	de,bc

	ld	iy,flags

	ld	hl,hook_clearers
	add	hl,de
	add	hl,de
	add	hl,de

	ld	hl,(hl)
jp_hl:
	jp	(hl)

macro iypos flag,bit
	db	flag,($46) or (bit shl 3)
end macro
_check_hook:
	ld	iy,flags
	pop	bc,de
	push	de,bc

	ld	hl,.table
	add	hl,de
	add	hl,de

	ld	a,(hl)
	ld	(.smc),a
	inc	hl
	ld	a,(hl)
	ld	(.smc + 1),a

	xor	a,a
	bit	0,(iy+0)
.smc = $-2
	ret	z
	inc	a
	ret
.table:
	iypos	 hookflags2,7	; CURSOR
	iypos	 hookflags2,1	; LIBRARY
	iypos	 hookflags2,5	; RAW_KEY
	iypos	 hookflags2,0	; GET_CSC
	iypos	 hookflags2,4	; HOMESCREEN
	iypos	 hookflags3,2	; WINDOW
	iypos	 hookflags3,3	; GRAPH
	iypos	 hookflags3,4	; Y_EQUALS
	iypos	 hookflags3,5	; FONT
	iypos	 hookflags3,6	; REGRAPH
	iypos	 hookflags3,7	; GRAPHICS
	iypos	 hookflags4,0	; TRACE
	iypos	 hookflags4,1	; PARSER
	iypos	 hookflags4,2	; APP_CHANGE
	iypos	 hookflags4,3	; CATALOG1
	iypos	 hookflags4,4	; HELP
	iypos	 hookflags4,5	; CX_REDISP
	iypos	 hookflags4,6	; MENU
	iypos	 hookflags2,6	; CATALOG2
	iypos	 hookflags3,0	; TOKEN
	iypos	 hookflags3,1	; LOCALIZE
	iypos	 hookflags4,7	; SILENT_LINK
	iypos	 hookflags5,0	; USB_ACTIVITY

_clear_key_hook:
	ld	iy,flags
	jp	_ClrRawKeyHook
_set_key_hook:
	pop	de,hl
	push	hl,de
	jp	_SetGetKeyHook

hook_clearers:
	dl	_ClrCursorHook
	dl	_ClrLibraryHook
	dl	_ClrRawKeyHook
	dl	_ClrGetKeyHook
	dl	_ClrHomescreenHook
	dl	_ClrWindowHook
	dl	_ClrGraphModeHook
	dl	_ClrYeditHook
	dl	_ClrFontHook
	dl	_ClrRegraphHook
	dl	_ClrGraphicsHook
	dl	_ClrTraceHook
	dl	_ClrParserHook
	dl	_ClrAppChangeHook
	dl	_ClrCatalog1Hook
	dl	_ClrHelpHook
	dl	_ClrCxReDispHook
	dl	_ClrMenuHook
	dl	_ClrCatalog2Hook
	dl	_ClrTokenHook
	dl	_ClrLocalizeHook
	dl	_ClrSilentLinkHook
	dl	_ClrUSBActivityHook
