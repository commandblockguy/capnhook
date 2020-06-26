public	_set_hook_by_type
public	_install_main_executor
public	_install_individual_executor

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

_FindFreeArcSpot	equ $0022078

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

	; temp - set breakpoint for individual executor
	;push	hl
	;pop	de
	;inc	de
	;scf
	;sbc    hl,hl
	;ld     (hl),3
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
