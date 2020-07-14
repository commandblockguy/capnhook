#include <stddef.h>
#include <string.h>
#include <fileioc.h>
#include "hookman.h"

#define DB_NAME "HOOKSDB"
#define DB_TEMP_NAME "HOOKTMP"

#undef NDEBUG // since it's broken in llvm
#include <debug.h>

#define CURRENT_VERSION 0

typedef struct {
    uint24_t version;
} database_header_t;

const database_header_t header = {
    CURRENT_VERSION
};

typedef struct {
    uint24_t id;
    hook_t *hook;			// pointer to the user hook
    uint8_t type;			// enum for which OS hook to use
    uint8_t priority;		// process lowest priorities first
    bool enabled;
    char description[1];
} user_hook_entry_t;

user_hook_entry_t *get_next(user_hook_entry_t *entry);
user_hook_entry_t *get_entry_by_id(uint24_t id, ti_var_t db);
hook_error_t remove_entry(user_hook_entry_t *entry, ti_var_t db);
bool user_hook_valid(hook_t *hook);
hook_error_t init(void);
hook_error_t open_db(ti_var_t *slot);
hook_error_t insert_existing(void);
void mark_database_modified(void);
void *flash_relocate(void *data, size_t size);
void sort_by_priority(hook_t **hooks, uint8_t *priorities, uint8_t num_hooks);
hook_error_t install_hooks(void);

void install_main_executor(void);
void install_individual_executor(hook_type_t type, hook_t **table);
bool check_hook(uint24_t type);
void clear_hook(uint24_t type);

#ifndef NDEBUG
void debug_print_db(void);
#endif

extern hook_t **hook_addresses[HOOK_NUM_TYPES];

// Global state
bool initted = false;
bool existing_checked = false;
bool database_modified = false;

void sort_by_priority(hook_t **hooks, uint8_t *priorities, uint8_t num_hooks) {
    for(uint8_t i = 0; i < num_hooks - 1; i++) {
        uint8_t min_index = i;
        for(uint8_t j = i + 1; j < num_hooks; j++) {
            if(priorities[j] < priorities[min_index]) min_index = j;
        }
        if(min_index != i) {
            // Swap hooks at min_index and i
            hook_t *hook = hooks[min_index];
            uint8_t priority = priorities[min_index];
            hooks[min_index] = hooks[i];
            priorities[min_index] = priorities[i];
            hooks[i] = hook;
            priorities[i] = priority;
        }
    }
}

hook_error_t install_hooks(void) {
    install_main_executor();

    ti_var_t db = ti_Open(DB_NAME, "r");
    ti_Seek(sizeof(database_header_t), SEEK_SET, db);
    user_hook_entry_t *entries = ti_GetDataPtr(db);
    ti_Seek(0, SEEK_END, db);
    user_hook_entry_t *end = ti_GetDataPtr(db);

    //todo: faster
    for(int type = 0; type < HOOK_NUM_TYPES; type++) {
        uint8_t number_hooks = 0;
        hook_t *hooks[256];
        uint8_t priorities[256];
        for(user_hook_entry_t *entry = entries; entry < end; entry = get_next(entry)) {
            if(entry->type == type && user_hook_valid(entry->hook) && entry->enabled) {
                hooks[number_hooks] = entry->hook;
                priorities[number_hooks] = entry->priority;
                number_hooks++;
                if(number_hooks == 255) {
                    dbg_sprintf(dbgout, "Maximum number of hooks exceeded for type %u\n", type);
                    break;
                }
            }
        }
        if(number_hooks) {
            hook_t **table;
            sort_by_priority(hooks, priorities, number_hooks);
            hooks[number_hooks] = NULL;
            table = flash_relocate(hooks, sizeof(hooks[0]) * (number_hooks + 1));
            if(!table) {
                dbg_sprintf(dbgout, "Failed to relocate table for type %u\n", type);
                continue;
            }
            install_individual_executor(type, table);
        }
    }
    ti_Close(db);

    return HOOK_SUCCESS;
}

user_hook_entry_t *get_next(user_hook_entry_t *entry) {
    return (user_hook_entry_t*)&entry->description[strlen(entry->description) + 1];
}

bool user_hook_valid(hook_t *hook) {
    return *(uint8_t*)hook == 0x83;
}

// todo: probably use a more sophisticated way of doing this, as Mateo mentioned
// if so, maybe also provide that method to users rather than leaving memory allocation up to them
void *flash_relocate(void *data, size_t size) {
    void *install_location;
    if(!ti_ArchiveHasRoom(size)) return NULL;
    ti_var_t slot = ti_Open("TMP", "w");
    if(!slot) return NULL;
    if(!ti_Write(data, size, 1, slot)) return NULL;
    if(!ti_SetArchiveStatus(true, slot)) return NULL;
    ti_Rewind(slot);
    install_location = ti_GetDataPtr(slot);
    ti_Close(slot);

    ti_Delete("TMP");

    dbg_sprintf(dbgout, "Relocated %u bytes from %p to %p\n", size, data, install_location);

    return install_location;
}

#ifndef NDEBUG
void debug_print_db(void) {
    dbg_sprintf(dbgout, "----- DATABASE UPDATED -----\n");
    ti_var_t db = ti_Open(DB_TEMP_NAME, "r");
    if(!db) {
        db = ti_Open(DB_NAME, "r");
    }
    void *ptr = ti_GetDataPtr(db);
    database_header_t header;
    ti_Read(&header, sizeof(header), 1, db);

    user_hook_entry_t *start = ti_GetDataPtr(db);
    ti_Seek(0, SEEK_END, db);
    user_hook_entry_t *end = ti_GetDataPtr(db);

    dbg_sprintf(dbgout, "DB: %p | Size: %u | Version: %u\n", ptr, ti_GetSize(db), header.version);
    for(user_hook_entry_t *current = start; current < end; current = get_next(current)) {
        dbg_sprintf(dbgout, "id: %06X | hook: %p | type: %2u | priority: %3u | enabled: %c | description: %s\n",
                current->id, current->hook, current->type, current->priority, current->enabled ? 'T' : 'F', current->description);
    }
    ti_Close(db);
}
#endif
