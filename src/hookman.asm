public _hook_Sync
public _init

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

extern _initted
extern _existing_checked
extern _database_modified

extern _insert_existing
extern _hook_Discard
extern _install_hooks

include	"hook_equates.inc"

_GetCSC			equ	$02014C
_Mov9ToOP1		equ	$020320
_ChkFindSym		equ	$02050C
_EnoughMem		equ	$02051C
_PopOP1			equ	$0205C4
_PushOP1		equ	$020628
_CreateAppVar		equ	$021330
_DelVarArc		equ	$021434
_Arc_Unarc		equ	$021448
_ChkInRam		equ	$021F98
_FindFreeArcSpot	equ	$022078

OP1			equ	$D005F8
flags			equ	$D00080

AppVarObj		equ	$15

virtual at 0
	HOOK_SUCCESS				rb	1
	HOOK_ERROR_NO_MATCHING_ID		rb	1
	HOOK_ERROR_INVALID_USER_HOOK		rb	1
	HOOK_ERROR_UNKNOWN_TYPE			rb	1
	HOOK_ERROR_DESCRIPTION_TOO_LONG		rb	1
	HOOK_ERROR_INTERNAL_DATABASE_IO		rb	1
	HOOK_ERROR_NEEDS_GC			rb	1
	HOOK_ERROR_DATABASE_CORRUPTED		rb	1
	HOOK_ERROR_UNKNOWN_DATABASE_VERSION	rb	1
end virtual

current_version		equ	0

; todo: remove
open_dbg:
	push	hl
	scf
	sbc    hl,hl
	ld     (hl),2
	pop	hl
	ret

_hook_Sync:
	ld	a,(_existing_checked)
	or	a,a
	call	z,_init

	ld	a,(_initted)
	or	a,a
	ret	z

	ld	a,(_database_modified)
	or	a,a
	jr	z,.not_modified

	ld	hl,temp_db_name
	call	_Mov9ToOP1
	call	_ChkFindSym

	ld	a,HOOK_ERROR_INTERNAL_DATABASE_IO
	ret	c
	call	_ChkInRam
	ret	nz

	ex	hl,de ; hl = pointer to size
	ld	de,(hl)
	ex.sis	hl,de

	ld	bc,12
	add	hl,bc
	call	_FindFreeArcSpot ; no idea how this call works, and am just copying ti_ArchiveHasRoom
	ld	a,HOOK_ERROR_NEEDS_GC
	ret	z

	ld	hl,db_name
	call	_Mov9ToOP1
	call	_ChkFindSym
	jr	c,.deleted
	call	_DelVarArc

.deleted:
	ld	hl,temp_db_name
	call	_Mov9ToOP1
	call	_ChkFindSym

	ld	a,HOOK_ERROR_INTERNAL_DATABASE_IO
	ret	c

	ld	de,-13
	add	hl,de
	ld	de,'BDS'
	ld	(hl),de

	ld	hl,db_name
	call	_Mov9ToOP1
	ld	iy,flags
	call	_Arc_Unarc

	call	_install_hooks
	or	a,a
	ret	nz

.not_modified:
	call	_hook_Discard

	ret

_init:
	; check if temp DB already exists
	ld	hl,temp_db_name
	call	_Mov9ToOP1
	call	_ChkFindSym
	jq	c,.create_temp_db
	call	_ChkInRam
	jq	z,.temp_db_exists
	; todo: unarchive temp db
	jq	.temp_db_exists
.create_temp_db:
	; check if real DB exists
	ld	hl,db_name
	call	_Mov9ToOP1
	call	_ChkFindSym
	jq	nc,.real_db_exists

	; real DB does not exist - create empty temp DB
	ld	hl,3 ; return if no space
	call	_EnoughMem
	ld	a,HOOK_ERROR_INTERNAL_DATABASE_IO ; todo: add new "no memory" error?
	ret	c

	ld	hl,temp_db_name
	call	_Mov9ToOP1
	ld	hl,3
	call	_CreateAppVar

	ex	hl,de
	inc	hl
	inc	hl ; hl = appvar data pointer
	ld	de,current_version
	ld	(hl),de
	jq	.temp_db_exists
.real_db_exists:
	; todo: check version, archive if not already
	; get size of DB
	ex	hl,de
	ld	bc,9 ; todo: only do this if archived
	add	hl,bc ; hl = pointer to name length
	ld	c,(hl)
	add	hl,bc
	inc	hl ; hl = pointer to size
	push	hl
	ld	de,(hl)
	ex	hl,de
	; check if there is enough memory
	call	_EnoughMem
	jq	c,.pop_ret_internal_error
	push	de

	; create var for temp DB
	ld	hl,temp_db_name
	call	_Mov9ToOP1
	pop	hl
	push	hl
	call	_CreateAppVar
	pop	bc
	pop	hl
	inc	hl
	inc	hl
	inc	de
	inc	de
	ldir
.temp_db_exists:
	ld	hl,_existing_checked
	bit	0,(hl)
	jq	nz,.checked_existing
	set	0,(hl)
	call	_insert_existing
	or	a,a
	ret	c
.checked_existing:
	ld	a,1
	ld	(_initted),a

	xor	a,a ; return HOOK_SUCCESS
	ret
.pop_ret_internal_error:
	pop	hl
	ld	a,HOOK_ERROR_INTERNAL_DATABASE_IO
	ret

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

temp_db_name:
	db	AppVarObj,"HOOKTMP",0
db_name:
	db	AppVarObj,"HOOKSDB",0
