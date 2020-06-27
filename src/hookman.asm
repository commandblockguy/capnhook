public	_set_hook_by_type
public	_install_main_executor
public	_install_individual_executor
public	_hook_addresses
public	_check_hook
public	_clear_hook

extern	main_executor
extern	main_executor_call_location
extern	main_executor_call_destination
extern	main_executor_size

extern	individual_executor
extern	individual_executor_table
extern	individual_executor_jump
extern	individual_executor_size

extern _flash_relocate

include	"hook_equates.inc"

_FindFreeArcSpot	equ	$022078
flags			equ	$D00080

_install_main_executor:
	ld	hl,main_executor_size
	push	hl
	call	find_install_location
	ld	(main_executor_location),hl

	ld	de,main_executor_call_destination - main_executor
	add	hl,de
	ld	(main_executor_call_location),hl

	ld	bc,main_executor
	push	bc
	call	_flash_relocate
	pop	bc,bc
	ret

; void install_individual_executor(hook_type_t type, hook_t *table)
_install_individual_executor:
	push	ix
	ld	ix,0
	add	ix,sp

	ld	bc,(ix+9)
	ld	(individual_executor_table),bc

	ld	bc,(main_executor_location)
	ld	(individual_executor_jump),bc

	ld	bc,individual_executor_size
	push	bc
	ld	bc,individual_executor
	push	bc
	call	_flash_relocate
	pop	bc,bc

	inc	hl

	; temp - set breakpoint for individual executor
	;push	hl
	;pop	de
	;inc	de
	;scf
	;sbc	hl,hl
	;ld	 (hl),3
	;push	de
	;pop	hl
	;dec	hl

	ld	bc,(ix+6)
	ld	ix,hook_setters
	add	ix,bc
	add	ix,bc
	add	ix,bc
	ld	ix,(ix)

	call	jmp_ix

	pop	ix
	ret

jmp_ix:
	jp	(ix)

; takes size in hl, returns ptr in hl
; todo: is this only called in one place?
find_install_location:
	ld	bc,12
	add	hl,bc
	push	hl
	pop	bc
	call	_FindFreeArcSpot
	ld	bc,15
	add	hl,bc
	ret

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

_clear_hook:
	pop	bc,de
	push	de,bc

	ld	iy,flags

	ld	hl,hook_clearers
	add	hl,de
	add	hl,de
	add	hl,de

	ld	hl,(hl)

	jp	(hl)

main_executor_location:
	rl	1

hook_setters:
	dl	_SetCursorHook
	dl	_SetLibraryHook
	dl	_SetGetKeyHook
	dl	_SetGetCSCHook
	dl	_SetHomescreenHook
	dl	_SetWindowHook
	dl	_SetGraphModeHook
	dl	_SetYeditHook
	dl	_SetFontHook
	dl	_SetRegraphHook
	dl	_SetGraphicsHook
	dl	_SetTraceHook
	dl	_SetParserHook
	dl	_SetAppChangeHook
	dl	_SetCatalog1Hook
	dl	_SetHelpHook
	dl	_SetCxReDispHook
	dl	_SetMenuHook
	dl	_SetCatalog2Hook
	dl	_SetTokenHook
	dl	_SetLocalizeHook
	dl	_SetSilentLinkHook
	dl	_SetUSBActivityHook
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
_hook_addresses:
	dl	cursorHookPtr
	dl	libraryHookPtr
	dl	rawKeyHookPtr
	dl	getKeyHookPtr
	dl	homescreenHookPtr
	dl	windowHookPtr
	dl	graphHookPtr
	dl	yEqualsHookPtr
	dl	fontHookPtr
	dl	regraphHookPtr
	dl	graphicsHookPtr
	dl	traceHookPtr
	dl	parserHookPtr
	dl	appChangeHookPtr
	dl	catalog1HookPtr
	dl	helpHookPtr
	dl	cxRedispHookPtr
	dl	menuHookPtr
	dl	catalog2HookPtr
	dl	tokenHookPtr
	dl	localizeHookPtr
	dl	silentLinkHookPtr
	dl	USBActivityHookPtr
