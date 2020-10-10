
include '../include/library.inc'
;-------------------------------------------------------------------------------

library 'CAPNHOOK',1

;-------------------------------------------------------------------------------
; v1 functions
;-------------------------------------------------------------------------------
export hook_Sync
export hook_Discard
export hook_Install
export hook_Uninstall
export hook_SetPriority
export hook_Enable
export hook_Disable
export hook_GetHook
export hook_GetType
export hook_GetPriority
export hook_IsInstalled
export hook_IsEnabled
export hook_GetDescription
export hook_CheckValidity

include	"hook_equates.inc"

_strcpy			equ	$0000CC
_GetCSC			equ	$02014C
_Mov9ToOP1		equ	$020320
_ChkFindSym		equ	$02050C
_InsertMem		equ	$020514
_EnoughMem		equ	$02051C
_DelMem			equ	$020590
_PopOP1			equ	$0205C4
_PushOP1		equ	$020628
_CreateAppVar		equ	$021330
_DelVarArc		equ	$021434
_Arc_Unarc		equ	$021448
_ChkInRam		equ	$021F98
_FindFreeArcSpot	equ	$022078

OP1			equ	$D005F8
flags			equ	$D00080

flag_continue	equ	10

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

current_version		equ	3

hook_Sync:
	ld	a,(existing_checked)
	or	a,a
	push	ix
	call	z,init
	pop	ix

	ld	a,(initted)
	or	a,a
	ret	z

	ld	a,(database_modified)
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
	push	hl
	pop	bc
	call	_FindFreeArcSpot
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

	push	ix
	call	install_hooks
	pop	ix
	or	a,a
	ret	nz

.not_modified:
	jq	hook_Discard

hook_Discard:
	ld	hl,temp_db_name
	call	_Mov9ToOP1
	call	_ChkFindSym
	jr	c,.deleted
	call	_DelVarArc
.deleted:
	xor	a,a
	ld	(initted),a
	ld	(database_modified),a
	ret

hook_Install:
	ld	iy,0
	add	iy,sp

	ld	a,(iy+12) ; check that the type is valid
	cp	a,23
	ld	a,HOOK_ERROR_UNKNOWN_TYPE
	ret	nc

	ld	hl,(iy+6)
	ld	a,(hl)
	cp	a,$83
	ld	a,HOOK_ERROR_INVALID_USER_HOOK
	ret	nz

	ld	hl,(iy+18)
	xor	a,a
	ld	bc,0
	cpir
	or	a,a
	sbc	hl,hl
	sbc	hl,bc
	or	a,h ; a is 0
	ld	a,HOOK_ERROR_DESCRIPTION_TOO_LONG
	ret	nz
	push	ix,hl

	ld	bc,9 ; size of entry not including description
	add	hl,bc
	call	_EnoughMem
	ld	a,HOOK_ERROR_INTERNAL_DATABASE_IO
	jq	c,.pop_exit

	ld	hl,(iy+9) ; check if size is 0
	add	hl,de
	or	a,a
	sbc	hl,de
	jq	z,.copied

	push	iy,de
	push	hl
	ld	bc,(iy+6)
	push	bc
	ld	iy,flags
	call	flash_relocate
	pop	bc,bc,de,iy

	add	hl,de ; check if null
	or	a,a
	sbc	hl,de
	ld	a,HOOK_ERROR_NEEDS_GC
	jq	z,.pop_exit

	ld	(iy+6),hl
.copied:

	push	iy,de ; delete existing
	ld	bc,(iy+3) ; id
	call	get_entry_rw
	pop	de,iy
	cp	a,HOOK_ERROR_NO_MATCHING_ID+1
	jq	nc,.pop_exit ; return error if not success or no id
	push	iy,de,hl
	cp	a,HOOK_ERROR_NO_MATCHING_ID
	jq	z,.removed
	ld	a,(ix+7) ; set new priority to old priority
	ld	(iy+15),a
	call	remove_entry
.removed:
	pop	hl,de

	ld	bc,5
	add	hl,bc
	ex	hl,de
	ld	iy,flags
	push	hl
	call	_InsertMem
	pop	bc,iy

	push	de
	pop	ix

	ld	hl,0
	ld	l,(ix-5)
	ld	h,(ix-4)
	add	hl,bc
	ld	(ix-5),l
	ld	(ix-4),h

	ld	bc,(iy+3) ; id
	ld	(ix),bc
	ld	bc,(iy+6) ; hook
	ld	(ix+3),bc
	ld	a,(iy+12) ; type
	ld	(ix+6),a
	ld	a,(iy+15) ; priority
	ld	(ix+7),a
	ld	a,1 ; enabled
	ld	(ix+8),a

	lea	ix,ix+9
	push	ix
	pop	de,bc,ix
	ld	hl,(iy+18)
	ldir

	ld	a,1
	ld	(database_modified),a
	xor	a,a
	ret
.pop_exit:
	pop	bc,ix
	ret

hook_Uninstall:
	pop	de,bc
	push	bc,de,ix
	call	get_entry_rw
	or	a,a
	jq	nz,.exit

	call	remove_entry

	ld	a,1
	ld	(database_modified),a
	xor	a,a
.exit:
	pop	ix
	ret

hook_SetPriority:
	pop	de,bc,hl
	push	hl,bc,de,ix,hl
	call	get_entry_rw
	pop	bc
	or	a,a
	jq	nz,.exit

	; todo: check that this is not an imported OS hook

	ld	a,1
	ld	(database_modified),a ; todo: only mark modified if changed
	ld	(ix+7),c

	xor	a,a
.exit:
	pop	ix
	ret

hook_Enable:
	pop	de,bc
	push	bc,de,ix
	call	get_entry_rw
	or	a,a
	jq	nz,.exit

	; todo: check that hook is still valid

	ld	a,1
	ld	(database_modified),a ; todo: only mark modified if previously disabled
	ld	(ix+8),a

	xor	a,a
.exit:
	pop	ix
	ret

hook_Disable:
	pop	de,bc
	push	bc,de,ix
	call	get_entry_rw
	or	a,a
	jq	nz,.exit

	ld	a,1
	ld	(database_modified),a ; todo: only mark modified if previously enabled

	xor	a,a
	ld	(ix+8),a
.exit:
	pop	ix
	ret

hook_GetHook:
	pop	de,bc,hl
	push	hl,bc,de,ix,hl
	ld	de,0
	ld	(hl),de

	call	get_entry_readonly
	pop	hl
	ld	a,HOOK_ERROR_NO_MATCHING_ID
	jq	nc,.pop_ix_ret

	ld	de,(ix+3)
	ld	(hl),de
	xor	a,a
.pop_ix_ret:
	pop	ix
	ret

hook_GetType:
	pop	de,bc,hl
	push	hl,bc,de,ix,hl
	ld	a,-1
	ld	(hl),a

	call	get_entry_readonly
	pop	hl
	ld	a,HOOK_ERROR_NO_MATCHING_ID
	jq	nc,.pop_ix_ret

	ld	a,(ix+6)
	ld	(hl),a
	xor	a,a
.pop_ix_ret:
	pop	ix
	ret

hook_GetPriority:
	pop	de,bc,hl
	push	hl,bc,de,ix,hl

	call	get_entry_readonly
	pop	hl
	ld	a,HOOK_ERROR_NO_MATCHING_ID
	jq	nc,.pop_ix_ret

	ld	a,(ix+7)
	ld	(hl),a
	xor	a,a
.pop_ix_ret:
	pop	ix
	ret

hook_IsInstalled:
	pop	de,bc,hl
	push	hl,bc,de,ix,hl
	xor	a,a
	ld	(hl),a

	call	get_entry_readonly
	pop	hl
	ld	a,0
	jq	nc,.pop_ix_ret

	inc	a
	ld	(hl),a
	xor	a,a
.pop_ix_ret:
	pop	ix
	ret

hook_IsEnabled:
	pop	de,bc,hl
	push	hl,bc,de,ix,hl
	xor	a,a
	ld	(hl),a

	call	get_entry_readonly
	pop	hl
	ld	a,HOOK_ERROR_NO_MATCHING_ID
	jq	nc,.pop_ix_ret

	ld	a,(ix+8)
	ld	(hl),a
	xor	a,a
.pop_ix_ret:
	pop	ix
	ret

hook_GetDescription:
	pop	de,bc,hl
	push	hl,bc,de,ix,hl
	ld	de,0
	ld	(hl),de

	call	get_entry_readonly
	pop	hl
	ld	a,HOOK_ERROR_NO_MATCHING_ID
	jq	nc,.pop_ix_ret

	lea	ix,ix+9
	ld	(hl),ix
	xor	a,a
.pop_ix_ret:
	pop	ix
	ret

hook_CheckValidity:
	pop	de,bc
	push	bc,de,ix

	call	get_entry_readonly
	ld	a,HOOK_ERROR_NO_MATCHING_ID
	jq	nc,.pop_ix_ret

	ld	hl,(ix+3)
	ld	a,$83
	cp	a,(hl)
	ld	a,HOOK_ERROR_INVALID_USER_HOOK
	jr	nz,.pop_ix_ret
	xor	a,a
.pop_ix_ret:
	pop	ix
	ret

init:
	ld	iy,flags
	; check if temp DB already exists
	ld	hl,temp_db_name
	call	_Mov9ToOP1
	call	_ChkFindSym
	jq	c,.create_temp_db
	call	_ChkInRam
	jq	z,.temp_db_exists
	ld	hl,temp_db_name
	call	_Mov9ToOP1
	call	_Arc_Unarc
	jq	.temp_db_exists
.create_temp_db:
	; check if real DB exists
	ld	hl,db_name
	call	_Mov9ToOP1
	call	_ChkFindSym
	jq	nc,.real_db_exists

.real_db_not_exists:
	; real DB does not exist - create empty temp DB
	ld	hl,3 ; return if no space
	call	_EnoughMem
	ld	a,HOOK_ERROR_INTERNAL_DATABASE_IO
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
	call	_ChkInRam
	jq	nz,.real_db_archived

	; todo: check if GC required?
	ld	hl,db_name ; archive DB if not already
	call	_Mov9ToOP1
	call	_PushOP1
	call	_Arc_Unarc
	call	_PopOP1
	call	_ChkFindSym
	jq	c,.real_db_not_exists
.real_db_archived:
	; get size of DB
	ex	hl,de
	call	skip_archive_header
	push	hl
	ld	de,(hl)
	inc	hl
	inc	hl
	ld	bc,(hl)
	ld	hl,current_version
	or	a,a
	sbc	hl,bc
	jq	z,.version_correct

	ld	a,HOOK_ERROR_UNKNOWN_DATABASE_VERSION
	pop	hl
	ret
.version_correct:
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
	ld	hl,existing_checked
	bit	0,(hl)
	jq	nz,.checked_existing
	set	0,(hl)
	call	insert_existing
.checked_existing:
	ld	a,1
	ld	(initted),a

	xor	a,a ; return HOOK_SUCCESS
	ret
.pop_ret_internal_error:
	pop	hl
	ld	a,HOOK_ERROR_INTERNAL_DATABASE_IO
	ret

skip_archive_header:
	ld	bc,9
	add	hl,bc ; hl = pointer to name length
	ld	c,(hl)
	add	hl,bc
	inc	hl ; hl = pointer to size
	ret

; ix: pointer to entry
get_next:
	lea	ix,ix+9 ; start of description
.loop:
	ld	a,(ix)
	or	a,a
	inc	ix
	jr	nz,.loop
	ret

; bc: entry id
; ix: pointer to size bytes of appvar
; returns pointer to entry in ix
; carry flag set if found
get_entry:
	ld	de,0
	ld	e,(ix)
	ld	d,(ix+1)
	lea	iy,ix+2
	add	iy,de ; iy: end of file
	push	iy
	pop	de
	lea	ix,ix+5 ; ix: pointer to first entry
.loop:
	or	a,a
	push	ix
	pop	hl
	sbc	hl,de
	ret	nc

	ld	hl,(ix) ; hl = id of current entry
	or	a,a
	sbc	hl,bc
	scf
	ret	z

	call	get_next
	jq	.loop

; returns pointer to size bytes in ix
; carry flag set if success
open_readonly:
	ld	hl,temp_db_name
	call	_Mov9ToOP1
	call	_ChkFindSym
	jq	nc,.found

	ld	hl,db_name
	call	_Mov9ToOP1
	call	_ChkFindSym
	ccf
	ret	nc
.found:
	call	_ChkInRam
	ex	hl,de
	call	nz,skip_archive_header
	push	hl
	pop	ix
	; todo: return the error type to the user somehow
	ld	de,(ix+2) ; check database version
	ld	hl,current_version
	or	a,a
	sbc	hl,de
	scf
	ret	z
	ccf
	ret

; bc: id
; returns entry in ix
; carry flag set if found
get_entry_readonly:
	push	bc
	call	open_readonly
	pop	bc
	ret	nc
	jq	get_entry

; bc: id
; returns entry in ix
; returns pointer to var size in hl
; returns error in a
get_entry_rw:
	push	bc
	ld	a,(initted)
	or	a,a
	jr	nz,.initted

	call	init
	or	a,a
	pop	bc
	ret	nz
	push	bc
.initted:
	ld	hl,temp_db_name
	call	_Mov9ToOP1
	call	_ChkFindSym
	pop	bc
	ccf
	ld	a,HOOK_ERROR_INTERNAL_DATABASE_IO
	ret	nc

	ex	hl,de
	push	hl,hl
	pop	ix
	call	get_entry
	pop	hl
	ld	a,HOOK_ERROR_NO_MATCHING_ID
	ret	nc
	ld	a,0
	ret

; ix: entry
; hl: pointer to size
remove_entry:
	push	hl,ix
	call	get_next
	push	ix
	pop	hl,de
	; c is reset from get_next
	sbc	hl,de ; hl: size of entry to remove
	ex	hl,de ; hl: pointer, de: size
	push	de
	call	_DelMem
	pop	de
	pop	ix ; hl: pointer to size
	ld	hl,0
	ld	l,(ix) ; todo: figure out what this is doing
	ld	h,(ix+1)
	or	a,a
	sbc	hl,de
	ld	(ix),l
	ld	(ix+1),h
	ret

insert_existing:
	ld	bc,23 ; num types
	push	bc
.loop:
	pop	bc
	dec	c
	ret	m
	push	bc

	call	check_hook
	or	a,a
	jq	z,.loop

	pop	bc ; check if hook is valid
	push	bc
	ld	hl,hook_addresses
	add	hl,bc
	add	hl,bc
	add	hl,bc
	ld	hl,(hl)
	ld	hl,(hl)
	push	hl
	pop	de
	ld	a,(hl)
	cp	a,$83
	jq	nz,.loop
	dec	hl ; check if this is one of our own hooks
	ld	a,(hl)
	cp	a,'m'
	jq	z,.loop

	pop	bc ; set types in entry
	push	bc
	ld	ix,.entry
	ld	(ix),c
	ld	(ix+3),de ; pointer to hook
	ld	(ix+6),c

	ld	hl,10 ; don't do anything if there's insufficient memory
	call	_EnoughMem ; todo: don't do this check if there's an existing entry with this ID?
	jq	c,.loop

	call	get_entry_rw
	cp	a,HOOK_ERROR_NO_MATCHING_ID+1
	jq	nc,.loop ; don't do anything if error besides "no matching id"
	push	hl
	cp	a,HOOK_ERROR_NO_MATCHING_ID
	call	z,remove_entry
	pop	ix

	push	ix
	ld	de,10
	ld	l,(ix)
	ld	h,(ix+1)
	add	hl,de
	ld	(ix),l
	ld	(ix+1),h
	pop	hl

	ld	bc,5
	add	hl,bc
	ex	hl,de
	push	de
	ld	iy,flags
	call	_InsertMem

	pop	de ; actually copy entry
	ld	hl,.entry
	ld	bc,10
	ldir

	ld	a,1
	ld	(database_modified),a

	jq	.loop
.entry:
	db	0,0,0, 0,0,0, 0, $ff, 1, 0

install_hooks:
	call	install_main_executor
	add	hl,de
	or	a,a
	sbc	hl,de
	jq	nz,.main_executor_installed
	ld	a,HOOK_ERROR_NEEDS_GC
	ret
.main_executor_installed:
	call	open_readonly
	ld	a,HOOK_ERROR_INTERNAL_DATABASE_IO
	ret	nc
	push	ix
	ld	bc,23 ; num types
	push	bc
.type_loop:
	pop	bc
	dec	c
	pop	ix
	ld	a,HOOK_SUCCESS
	ret	m
	push	ix,bc

	xor	a,a
	ld	(.num_hooks),a

	ld	de,0
	ld	e,(ix)
	ld	d,(ix+1)
	lea	iy,ix+2
	add	iy,de ; iy: end of file
	push	iy
	pop	de
	lea	ix,ix+5 ; ix: pointer to first entry
.entry_loop:
	or	a,a
	push	ix
	pop	hl
	sbc	hl,de
	jq	nc,.entry_loop_break

	pop	bc
	push	bc
	ld	a,(ix+6) ; type
	cp	a,c
	jq	nz,.skip

	ld	hl,(ix+3)
	ld	a,$83
	cp	a,(hl)
	jq	nz,.skip

	bit	0,(ix+8)
	jq	z,.skip

	ld	hl,.num_hooks
	ld	c,(hl)
	ld	hl,hooks_buf
	add	hl,bc
	add	hl,bc
	add	hl,bc
	add	hl,bc
	ld	a,(ix+7)
	ld	(hl),a
	inc	hl
	ld	bc,(ix+3) ; hook ptr
	ld	(hl),bc

	ld	a,(.num_hooks)
	inc	a
	ld	(.num_hooks),a
	cp	a,$FF
	jq	z,.entry_loop_break
.skip:
	call	get_next
	jq	.entry_loop

.entry_loop_break:
	ld	a,(.num_hooks)
	or	a,a
	jq	z,.type_loop

	ld	b,a
	call	sort_by_priority

	or	a,a
	ld	de,hooks_buf
	sbc	hl,de
	push	hl
	push	de

	call	flash_relocate
	pop	bc,bc,bc
	push	bc,hl,bc
	call	install_individual_executor
	pop	bc,bc
	add	hl,de
	or	a,a
	sbc	hl,de

	jq	nz,.type_loop
	pop	bc,ix
	ld	a,HOOK_ERROR_NEEDS_GC
	ret
.num_hooks:
	rb	1

hooks_buf:
	rb	256 * 4

; input: b = num hooks
; hl = i/j
; b = num hooks
; c = priority
; de = scrap
sort_by_priority:
	ld	hl,hooks_buf
	push	bc,hl
.loop:
	pop	hl
	inc	hl
	inc	hl
	inc	hl
	inc	hl
	dec	b
	jq	z,.exit
	ld	c,(hl)
	inc	hl
	ld	de,(hl)
	dec	hl
	push	hl,de
.inner_loop:
	or	a,a
	dec	hl
	dec	hl
	dec	hl
	dec	hl

	ld	de,hooks_buf
	sbc	hl,de
	add	hl,de
	jq	c,.inner_exit

	ld	a,(hl)
	cp	a,c
	jq	c,.inner_exit

	inc	hl
	ld	de,(hl)
	inc	hl
	inc	hl
	inc	hl
	ld	(hl),a
	inc	hl
	ld	(hl),de
	dec	hl
	dec	hl
	dec	hl
	dec	hl
	dec	hl
	jq	.inner_loop
.inner_exit:
	pop	de
	inc	hl
	inc	hl
	inc	hl
	inc	hl
	ld	(hl),c
	inc	hl
	ld	(hl),de

	jq	.loop
.exit:
	pop	bc
	ld	hl,0
	ld	de,0
	ld	e,b
	add	hl,de
	add	hl,de
	add	hl,de
	push	hl
	pop	bc
	ld	hl,hooks_buf+1
	ld	de,hooks_buf
.remove_priorities_loop:
	ldi
	ldi
	ldi
	inc	hl
	jq	pe,.remove_priorities_loop
	ex	hl,de
	ld	de,0
	ld	(hl),de
	inc	hl
	inc	hl
	inc	hl
	ret

; todo: should I use actual relocation for this?
flash_relocate:
	pop	de,bc,hl ; hl = size
	push	hl,bc,de,bc
	ld	iy,flags

	call	_EnoughMem
	jq	c,.ret_null
	push	de
	ld	hl,12
	add	hl,de
	push	hl
	pop	bc
	call	_FindFreeArcSpot
	jq	z,.ret_null

	ld	hl,temp_var_name
	call	_Mov9ToOP1
	call	_PushOP1

	pop	hl ; hl = size
	push	hl
	call	_CreateAppVar
	pop	bc ; todo: bc already contains size?
	inc	de
	inc	de
	pop	hl ; hl = data to copy
	ldir

	call	_PopOP1
	call	_Arc_Unarc

	call	_ChkFindSym
	push	de
	call	_DelVarArc
	pop	hl
	ld	bc,15 ; skip archive header
	add	hl,bc
	ret
.ret_null:
	pop	hl
	ld	hl,0
	ret

; todo: check if main executor is already installed?
install_main_executor:
	ld	hl,main_executor_size
	push	hl
	call	find_install_location
	ld	(main_executor_location),hl

	ld	de,main_executor_call_destination - main_executor
	add	hl,de
	ld	(main_executor_call_location),hl

	ld	bc,main_executor
	push	bc
	call	flash_relocate
	pop	bc,bc
	ret

; void install_individual_executor(hook_type_t type, hook_t *table)
install_individual_executor:
	push	ix
	ld	ix,0
	add	ix,sp

	ld	bc,(ix+9)
	ld	(individual_executor_table),bc

	ld	bc,(main_executor_location)
	ld	(individual_executor_jump),bc

	ld	hl,hook_exec_tails
	ld	bc,(ix+6)
	add	hl,bc
	add	hl,bc
	add	hl,bc
	ld	hl,(hl)
	ld	bc,(hl) ; size of tail
	inc	hl
	inc	hl
	inc	hl ; beginning of tail
	ld	de,individual_executor.tail
	push	bc
	ldir
	pop	hl

	ld	bc,individual_executor_size
	add	hl,bc
	push	hl
	ld	bc,individual_executor
	push	bc
	call	flash_relocate
	pop	bc,bc
	add	hl,de
	or	a,a
	sbc	hl,de
	ret	z

	inc	hl

	; temp - set breakpoint for individual executor
	;push	hl
	;pop	de
	;inc	de
	;scf
	;sbc	hl,hl
	;ld	(hl),3
	;push	de
	;pop	hl
	;dec	hl

	ld	bc,(ix+6)
	ld	ix,hook_setters
	add	ix,bc
	add	ix,bc
	add	ix,bc
	ld	ix,(ix)

	ld	iy,flags
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

check_hook:
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

main_executor:
	push	af,bc,de,hl,iy
.loop:
	ld	hl,(ix)	; check if we've reached the null terminator
	add	hl,de
	or	a,a
	sbc	hl,de
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
	ld	hl,8*3	; we don't care about anything else on the stack anymore
	add	hl,sp
	ld	sp,hl
	ld	hl,(scrapMem)
	ret
.exit:
	pop	iy,hl,de,bc,af
	ret

.execute:
main_executor_call_destination = $
	ld	ix,0
	add	ix,sp
	push	hl
	ld	a,(ix+19) ; restore original registers
	ld	bc,(ix+15)
	ld	de,(ix+12)
	ld	hl,(ix+9)
	ld	iy,(ix+6)
	pop	ix ; ix register is set to the address of the hook being called
	jp	(ix) ; todo: for some hook types, ix is not the address of the function 
main_executor_size = $-main_executor

individual_executor:
	db	'm', $83
	push	ix
	ld	ix,0
individual_executor_table = $-3
	call	0
individual_executor_jump = $-3
	pop	ix
individual_executor_size = $ - individual_executor
.tail:
	rb	localize_tail_size

libraryHookTail:
appChangeHookTail:
helpHookTail:
tokenHookTail:
USBActivityHookTail:
	dl	ret_tail_size
ret_tail:
	ret
ret_tail_size = $ - ret_tail

homescreenHookTail:
windowHookTail:
graphHookTail:
yEqualsHookTail:
regraphHookTail:
graphicsHookTail:
traceHookTail:
parserHookTail:
catalog1HookTail:
cxRedispHookTail:
menuHookTail:
catalog2HookTail:
	dl	ret_z_tail_size
ret_z_tail:
	cp	a,a
	ret
ret_z_tail_size = $ - ret_z_tail

rawKeyHookTail:
fontHookTail:
silentLinkHookTail:
	dl	ret_nz_tail_size
ret_nz_tail:
	or	a,a ; todo: actually reset z even if a is 0
	ret
ret_nz_tail_size = $ - ret_nz_tail

getKeyHookTail:
	dl	getkey_tail_size
getkey_tail:
	cp	a,$1b
	ret	nz
	or	a,a
	ld	a,b
	ret
getkey_tail_size = $ - getkey_tail

cursorHookTail:
	dl	cursor_tail_size
cursor_tail:
	cp	a,$24
	ret	nz
	ld	a,b
	or	a,a
	ret
cursor_tail_size = $ - cursor_tail

localizeHookTail:
	dl	localize_tail_size
localize_tail:
; mostly taken from Iambian's font thing
; see also https://pastebin.com/hhyLZSLL
	cp	a,$0a
	jr	nz,.not_menu_quasi
	ld	hl,(hl)
	cp	a,a
	ret
.not_menu_quasi:
	cp	a,$3a
	jr	z,.config_vars
	cp	a,$3b
	jr	nz,.not_data_types
.config_vars:
	or	a,a
	ld	a,b
	ret
.not_data_types:
	cp	a,$9F       ;catalog help. show "ARGUMENT FOR invNorm(" in fancy box
	ret	z
	cp	a,$C9       ;catalog help part 1 (token lookup)
	ret	z
	cp	a,$CA
	ret	z
	cp	a,$CB       ;catalog help part ? (draw help object)
	ret	z
	cp	a,$CC       ;catalog help part ?
	ret	z
	cp	a,$D8       ;catalog help part 2 (token render)
	ret	z
	cp	a,$7A       ;archive variable attempt failure message
	ret	z
	cp	a,$49
	jr	z,.bugged49     ;matrix editor thing
	cp	a,$4A
	jr	z,.bugged4A     ;matrix editor thing
	cp	a,$4B
	jr	z,.bugged4B     ;matrix editor thing
	cp	a,$4C
	jr	z,.bugged4C     ;matrix editor thing. A really buggy bug there.
	or	a,a

	push	bc ; reset z flag
	ld	b,a
	xor	a,a
	inc	a
	ld	a,b
	pop	bc
	ret
.bugged49:
	ld	a,10
	ret
.bugged4A:
	ld	a,13
	ret
.bugged4B:
	ld	a,12
	ret
.bugged4C:  ;bugged. Used in matrix editor.
	ld	a,$0A
	ld	(curCol),a
	ld	a,(currListHighlight)
	call	_DispListElementOffLA
	ld	a,(curCol)
	cp	a,$0B
	ret	nz
	ld	a,$20
	jp	_PutC
localize_tail_size = $ - localize_tail


main_executor_location:
	rl	1

initted:
	db	0
existing_checked:
	db	0
database_modified:
	db	0

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
hook_addresses:
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
hook_exec_tails: ; Hooks to be called if no user hooks want to return
	dl	cursorHookTail
	dl	libraryHookTail
	dl	rawKeyHookTail
	dl	getKeyHookTail
	dl	homescreenHookTail
	dl	windowHookTail
	dl	graphHookTail
	dl	yEqualsHookTail
	dl	fontHookTail
	dl	regraphHookTail
	dl	graphicsHookTail
	dl	traceHookTail
	dl	parserHookTail
	dl	appChangeHookTail
	dl	catalog1HookTail
	dl	helpHookTail
	dl	cxRedispHookTail
	dl	menuHookTail
	dl	catalog2HookTail
	dl	tokenHookTail
	dl	localizeHookTail
	dl	silentLinkHookTail
	dl	USBActivityHookTail

temp_db_name:
	db	AppVarObj,"HOOKTMP",0
db_name:
	db	AppVarObj,"HOOKSDB",0
temp_var_name:
	db	AppVarObj,"TMP",0,0,0,0,0
