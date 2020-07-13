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
ti_var_t open_db_readonly(void);
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

hook_error_t hook_Discard(void) {
    ti_Delete(DB_TEMP_NAME);

    initted = false;
    database_modified = false;

    return HOOK_SUCCESS;
}

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

hook_error_t hook_Install(uint24_t id, hook_t *hook, hook_type_t type, uint8_t priority, const char *description) {
    if(type >= HOOK_NUM_TYPES) return HOOK_ERROR_UNKNOWN_TYPE;
    if(!user_hook_valid(hook)) return HOOK_ERROR_INVALID_USER_HOOK;

    size_t description_length = strlen(description);
    if(description_length > 255) return HOOK_ERROR_DESCRIPTION_TOO_LONG;

    ti_var_t db;
    hook_error_t err = open_db(&db);
    if(err) return err;

    user_hook_entry_t entry = {id, hook, type, priority, true};

    user_hook_entry_t *existing = get_entry_by_id(id, db);
    if(existing) {
        // Replace the existing entry
        entry.priority = existing->priority;
        remove_entry(existing, db);
        // todo: don't mark modified if exactly identical
    }
    ti_Seek(0, SEEK_END, db);
    // Write to the end of the file
    if(!ti_Write(&entry, sizeof(entry) - 1, 1, db) ||
       !ti_Write(description, strlen(description) + 1, 1, db)) {
        ti_Close(db);
        return HOOK_ERROR_INTERNAL_DATABASE_IO;
    }

    ti_Close(db);
    mark_database_modified();
    return HOOK_SUCCESS;
}

hook_error_t hook_Uninstall(uint24_t id) {
    ti_var_t db;
    hook_error_t error = open_db(&db);
    if(error) return error;

    user_hook_entry_t *to_remove = get_entry_by_id(id, db);

    if(!to_remove) {
        ti_Close(db);
        return HOOK_SUCCESS;
    }

    error = remove_entry(to_remove, db);

    ti_Close(db);
    mark_database_modified();
    return error;
}

// todo: check if the ID is an imported OS hook
hook_error_t hook_SetPriority(uint24_t id, uint8_t priority) {
    ti_var_t db;
    hook_error_t error = open_db(&db);
    if(error) return error;

    user_hook_entry_t *entry = get_entry_by_id(id, db);

    if(!entry) return HOOK_ERROR_NO_MATCHING_ID;

    entry->priority = priority;

    ti_Close(db);
    mark_database_modified();
    return HOOK_SUCCESS;
}

hook_error_t hook_Enable(uint24_t id) {
    ti_var_t db;
    hook_error_t error = open_db(&db);
    if(error) return error;

    user_hook_entry_t *entry = get_entry_by_id(id, db);

    if(!entry) return HOOK_ERROR_NO_MATCHING_ID;

    if(!user_hook_valid(entry->hook)) return HOOK_ERROR_INVALID_USER_HOOK;

    entry->enabled = true;

    ti_Close(db);
    mark_database_modified();
    return HOOK_SUCCESS;
}

hook_error_t hook_Disable(uint24_t id) {
    ti_var_t db;
    hook_error_t error = open_db(&db);
    if(error) return error;

    user_hook_entry_t *entry = get_entry_by_id(id, db);

    if(!entry) return HOOK_SUCCESS;

    entry->enabled = false;

    ti_Close(db);
    mark_database_modified();
    return HOOK_SUCCESS;
}

hook_error_t hook_IsInstalled(uint24_t id, bool *result) {
    ti_var_t db = open_db_readonly();
    if(!db) {
        *result = false;
        return HOOK_SUCCESS;
    }

    if(get_entry_by_id(id, db)) {
        *result = true;
    } else {
        *result = false;
    }

    ti_Close(db);
    return HOOK_SUCCESS;
}

hook_error_t hook_GetHook(uint24_t id, hook_t **result) {
    ti_var_t db = open_db_readonly();
    if(!db) {
        *result = NULL;
        return HOOK_ERROR_INTERNAL_DATABASE_IO;
    }

    user_hook_entry_t *entry = get_entry_by_id(id, db);

    if(!entry) {
        ti_Close(db);
        *result = NULL;
        return HOOK_ERROR_NO_MATCHING_ID;
    }

    *result = entry->hook;

    ti_Close(db);
    return HOOK_SUCCESS;
}

hook_error_t hook_GetType(uint24_t id, hook_type_t *result) {
    ti_var_t db = open_db_readonly();
    if(!db) {
        *result = HOOK_TYPE_UNKNOWN;
        return HOOK_ERROR_INTERNAL_DATABASE_IO;
    }

    user_hook_entry_t *entry = get_entry_by_id(id, db);

    if(!entry) {
        ti_Close(db);
        *result = HOOK_TYPE_UNKNOWN;
        return HOOK_ERROR_NO_MATCHING_ID;
    }

    *result = entry->type;

    ti_Close(db);
    return HOOK_SUCCESS;
}

hook_error_t hook_GetPriority(uint24_t id, uint8_t *result) {
    ti_var_t db = open_db_readonly();
    if(!db) {
        ti_Close(db);
        *result = 0;
        return HOOK_ERROR_INTERNAL_DATABASE_IO;
    }

    user_hook_entry_t *entry = get_entry_by_id(id, db);

    if(!entry) {
        ti_Close(db);
        *result = 0;
        return HOOK_ERROR_NO_MATCHING_ID;
    }

    *result = entry->priority;

    ti_Close(db);
    return HOOK_SUCCESS;
}

hook_error_t hook_IsEnabled(uint24_t id, bool *result) {
    ti_var_t db = open_db_readonly();
    if(!db) {
        *result = false;
        return HOOK_ERROR_INTERNAL_DATABASE_IO;
    }

    user_hook_entry_t *entry = get_entry_by_id(id, db);

    if(!entry) {
        ti_Close(db);
        *result = false;
        return HOOK_ERROR_NO_MATCHING_ID;
    }

    *result = entry->enabled;

    ti_Close(db);
    return HOOK_SUCCESS;
}

hook_error_t hook_GetDescription(uint24_t id, char **result) {
    ti_var_t db = open_db_readonly();
    if(!db) {
        *result = NULL;
        return HOOK_ERROR_INTERNAL_DATABASE_IO;
    }

    user_hook_entry_t *entry = get_entry_by_id(id, db);

    if(!entry) {
        ti_Close(db);
        *result = NULL;
        return HOOK_ERROR_NO_MATCHING_ID;
    }

    *result = entry->description;

    ti_Close(db);
    return HOOK_SUCCESS;
}

hook_error_t hook_CheckValidity(uint24_t id) {
    ti_var_t db = open_db_readonly();
    if(!db) {
        return HOOK_ERROR_INTERNAL_DATABASE_IO;
    }

    user_hook_entry_t *entry = get_entry_by_id(id, db);

    if(!entry) {
        ti_Close(db);
        return HOOK_ERROR_NO_MATCHING_ID;
    }

    if(!user_hook_valid(entry->hook)) {
        ti_Close(db);
        return HOOK_ERROR_INVALID_USER_HOOK;
    }

    ti_Close(db);
    return HOOK_SUCCESS;
}

user_hook_entry_t *get_next(user_hook_entry_t *entry) {
    return (user_hook_entry_t*)&entry->description[strlen(entry->description) + 1];
}

user_hook_entry_t *get_entry_by_id(uint24_t id, ti_var_t db) {
    ti_Seek(sizeof(database_header_t), SEEK_SET, db);
    user_hook_entry_t *start = ti_GetDataPtr(db);

    ti_Seek(0, SEEK_END, db);
    user_hook_entry_t *end = ti_GetDataPtr(db);

    for(user_hook_entry_t *current = start; current < end; current = get_next(current)) {
        if(current->id == id) {
            return current;
        }
    }

    return NULL;
}

hook_error_t remove_entry(user_hook_entry_t *entry, ti_var_t db) {
    void *next = get_next(entry);
    size_t current_size = next - (void*)entry;
    ti_Seek(0, SEEK_SET, db);
    void *start = ti_GetDataPtr(db);
    memmove(start + current_size, start, (void*)entry - start);
    if(ti_Resize(ti_GetSize(db) - current_size, db) != current_size)
        return HOOK_ERROR_INTERNAL_DATABASE_IO;
    return HOOK_SUCCESS;
}

bool user_hook_valid(hook_t *hook) {
    return *(uint8_t*)hook == 0x83;
}

hook_error_t open_db(ti_var_t *slot) {
    if(!initted) {
        hook_error_t err = init();
        if(err) return err;
    }
    *slot = ti_Open(DB_TEMP_NAME, "r+");
    if(!*slot) return HOOK_ERROR_INTERNAL_DATABASE_IO;
    return HOOK_SUCCESS;
}

ti_var_t open_db_readonly(void) {
    ti_var_t slot = ti_Open(DB_TEMP_NAME, "r");
    if(!slot) {
        slot = ti_Open(DB_NAME, "r");
    }
    return slot;
}

hook_error_t insert_existing(void) {
    ti_var_t db;
    hook_error_t error = open_db(&db);
    if(error) return error;

    // Use the type as an ID
    for(int type = 0; type < HOOK_NUM_TYPES; type++) {
        // Check if the hook type is enabled
        if(!check_hook(type)) continue;
        // Check if the hook pointer is valid
        if(*(uint8_t*)*(hook_addresses[type]) != 0x83) continue;
        // Check if this is already one of our own hooks
        if(*((char*)*hook_addresses[type] - 1) == 'm') continue;

        mark_database_modified();

        user_hook_entry_t entry = {type, *hook_addresses[type], type, 255, true, ""};

        user_hook_entry_t *existing = get_entry_by_id(type, db);
        if(existing) {
            remove_entry(existing, db);
        }
        // Write to the end of the file
        if(!ti_Write(&entry, sizeof(entry), 1, db)) {
            ti_Close(db);
            return HOOK_ERROR_INTERNAL_DATABASE_IO;
        }

        clear_hook(type);
    }

    ti_Close(db);
    return HOOK_SUCCESS;
}

void mark_database_modified(void) {
    database_modified = true;
#ifndef NDEBUG
    debug_print_db();
#endif
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
    ti_var_t db = open_db_readonly();
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
