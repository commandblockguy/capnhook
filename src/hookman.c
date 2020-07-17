#include <stddef.h>
#include <string.h>
#include <fileioc.h>
#include "hookman.h"

#define DB_NAME "HOOKSDB"
#define DB_TEMP_NAME "HOOKTMP"

#undef NDEBUG // since it's broken in llvm
#include <debug.h>

typedef struct {
    uint24_t version;
} database_header_t;

typedef struct {
    uint24_t id;
    hook_t *hook;			// pointer to the user hook
    uint8_t type;			// enum for which OS hook to use
    uint8_t priority;		// process lowest priorities first
    bool enabled;
    char description[1];
} user_hook_entry_t;

user_hook_entry_t *get_next(user_hook_entry_t *entry);

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

user_hook_entry_t *get_next(user_hook_entry_t *entry) {
    return (user_hook_entry_t*)&entry->description[strlen(entry->description) + 1];
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
