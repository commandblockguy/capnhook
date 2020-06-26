#include <stddef.h>
#include <string.h>
#include <fileioc.h>
#include "hookman.h"

#define DB_NAME "HOOKDB"
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
    char description[64];
} user_hook_entry_t;

user_hook_entry_t *get_entry_by_id(uint24_t id, ti_var_t db);
bool user_hook_valid(hook_t *hook);
hook_error_t copy_db(ti_var_t *slot);
hook_error_t write_db(ti_var_t db);
void *flash_relocate(void *data, size_t size);
void install_main_executor(void);
void install_individual_executor(hook_type_t type, hook_t **table);

#ifndef NDEBUG
void debug_print_db(void);
#endif

hook_error_t refresh_hooks(void) {
#ifndef NDEBUG
    debug_print_db();
#endif

    install_main_executor();

    ti_var_t db = ti_Open(DB_NAME, "r");
    ti_Seek(sizeof(database_header_t), SEEK_SET, db);
    user_hook_entry_t *entries = ti_GetDataPtr(db);
    ti_Seek(0, SEEK_END, db);
    user_hook_entry_t *end = ti_GetDataPtr(db);

    //todo: faster
    for(int type = 0; type < NUM_HOOK_TYPES; type++) {
        uint8_t number_hooks = 0;
        hook_t *hooks[256];
        //todo: check if another hook is active, and if so add it as a minimum priority hook - guess that also means that I'll have to add a marker so that we don't add ourself as a minimum priority hook, which would be entertaining but detrimental to proper function
        for(user_hook_entry_t *entry = entries; entry < end; entry++) {
            if(entry->type == type && user_hook_valid(entry->hook) && entry->enabled) {
                hooks[number_hooks] = entry->hook;
                number_hooks++;
                if(number_hooks == 255) break;
            }
        }
        hooks[number_hooks] = NULL;
        if(number_hooks) {
            hook_t **table = flash_relocate(hooks, sizeof(hooks[0]) * (number_hooks + 1));
            if(!table) continue;
            install_individual_executor(type, table);
        }
    }
    ti_Close(db);

    return HOOK_SUCCESS;
}

hook_error_t install_hook(uint24_t id, hook_t *hook, hook_type_t type, uint8_t priority, const char *description) {
    if(type >= NUM_HOOK_TYPES) return HOOK_ERROR_UNKNOWN_TYPE;
    if(!user_hook_valid(hook)) return HOOK_ERROR_INVALID_USER_HOOK;

    size_t description_length = strlen(description);
    if(description_length > 63) return HOOK_ERROR_DESCRIPTION_TOO_LONG;

    ti_var_t db;
    hook_error_t error = copy_db(&db);
    if(error) return error;

    user_hook_entry_t entry = {id, hook, type, priority, true, {0}};

    strcpy(entry.description, description);

    user_hook_entry_t *existing = get_entry_by_id(id, db);
    if(existing) {
        // Replace the existing entry
        entry.priority = existing->priority;
        memcpy(existing, &entry, sizeof(entry));
    } else {
        // Write to the end of the file
        if(!ti_Write(&entry, sizeof(entry), 1, db)) {
            ti_Close(db);
            return HOOK_ERROR_INTERNAL_DATABASE_IO;
        }
    }

    return write_db(db);
}

hook_error_t uninstall_hook(uint24_t id) {
    ti_var_t db;
    hook_error_t error = copy_db(&db);
    if(error) return error;

    user_hook_entry_t *db_end = ti_GetDataPtr(db);
    user_hook_entry_t *to_remove = get_entry_by_id(id, db);

    if(!to_remove) {
        ti_Close(db);
        return HOOK_SUCCESS;
    }

    if(db_end - to_remove != 1) {
        // Copy the last entry to the entry to remove
        memcpy(to_remove, db_end - 1, sizeof(user_hook_entry_t));
    }

    // Remove last entry
    size_t current_size = ti_GetSize(db);
    size_t new_size = current_size - sizeof(user_hook_entry_t);
    if(ti_Resize(new_size, db) != sizeof(user_hook_entry_t)) {
        ti_Close(db);
        return HOOK_ERROR_INTERNAL_DATABASE_IO;
    }

    return write_db(db);
}

hook_error_t set_hook_priority(uint24_t id, uint8_t priority) {
    ti_var_t db;
    hook_error_t error = copy_db(&db);
    if(error) return error;

    user_hook_entry_t *entry = get_entry_by_id(id, db);

    if(!entry) return HOOK_ERROR_NO_MATCHING_ID;

    entry->priority = priority;

    return write_db(db);
}

hook_error_t enable_hook(uint24_t id) {
    ti_var_t db;
    hook_error_t error = copy_db(&db);
    if(error) return error;

    user_hook_entry_t *entry = get_entry_by_id(id, db);

    if(!entry) return HOOK_ERROR_NO_MATCHING_ID;

    if(!user_hook_valid(entry->hook)) return HOOK_ERROR_INVALID_USER_HOOK;

    entry->enabled = true;

    return write_db(db);
}

hook_error_t disable_hook(uint24_t id) {
    ti_var_t db;
    hook_error_t error = copy_db(&db);
    if(error) return error;

    user_hook_entry_t *entry = get_entry_by_id(id, db);

    if(!entry) return HOOK_SUCCESS;

    entry->enabled = false;

    return write_db(db);
}

hook_error_t is_hook_installed(uint24_t id, bool *result) {
    ti_var_t db = ti_Open(DB_NAME, "r");
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

hook_error_t get_hook_by_id(uint24_t id, hook_t **result) {
    ti_var_t db = ti_Open(DB_NAME, "r");
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

hook_error_t get_hook_type(uint24_t id, hook_type_t *result) {
    ti_var_t db = ti_Open(DB_NAME, "r");
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

hook_error_t get_hook_priority(uint24_t id, uint8_t *result) {
    ti_var_t db = ti_Open(DB_NAME, "r");
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

hook_error_t is_hook_enabled(uint24_t id, bool *result) {
    ti_var_t db = ti_Open(DB_NAME, "r");
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

hook_error_t get_hook_description(uint24_t id, char **result) {
    ti_var_t db = ti_Open(DB_NAME, "r");
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

hook_error_t check_hook_validity(uint24_t id) {
    ti_var_t db = ti_Open(DB_NAME, "r");
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

user_hook_entry_t *get_entry_by_id(uint24_t id, ti_var_t db) {
    ti_Seek(sizeof(database_header_t), SEEK_SET, db);
    user_hook_entry_t *start = ti_GetDataPtr(db);

    ti_Seek(0, SEEK_END, db);
    user_hook_entry_t *end = ti_GetDataPtr(db);

    for(user_hook_entry_t *current = start; current < end; current++) {
        if(current->id == id) {
            return current;
        }
    }

    return NULL;
}

bool user_hook_valid(hook_t *hook) {
    return *(uint8_t*)hook == 0x83;
}

hook_error_t copy_db(ti_var_t *slot) {
    ti_var_t new = ti_Open(DB_TEMP_NAME, "w+");
    if(!new) {
        *slot = 0;
        return HOOK_ERROR_INTERNAL_DATABASE_IO;
    }

    ti_var_t old = ti_Open(DB_NAME, "r");
    if(!old) {
        // Created a new, empty database
        ti_Write(&header, sizeof(database_header_t), 1, new);
        *slot = new;
        return HOOK_SUCCESS;
    }

    void *data_ptr = ti_GetDataPtr(old);
    size_t db_size = ti_GetSize(old);

    if(!data_ptr) {
        ti_Close(new);
        ti_Close(old);
        *slot = 0;
        return HOOK_ERROR_INTERNAL_DATABASE_IO;
    }

    database_header_t db_header = {0xFFFFFF};
    ti_Read(&db_header, sizeof(database_header_t), 1, old);
    if(db_header.version > CURRENT_VERSION) {
        ti_Close(new);
        ti_Close(old);
        *slot = 0;
        return HOOK_ERROR_UNKNOWN_DATABASE_VERSION;
    }

    if(!ti_Write(data_ptr, db_size, 1, new)) {
        ti_Close(new);
        ti_Close(old);
        *slot = 0;
        return HOOK_ERROR_INTERNAL_DATABASE_IO;
    }

    ti_Close(old);

    *slot = new;
    return HOOK_SUCCESS;
}

hook_error_t write_db(ti_var_t db) {
    if(!db) return HOOK_ERROR_INTERNAL_DATABASE_IO;

    size_t size = ti_GetSize(db);

    if(!ti_ArchiveHasRoom(size)) return HOOK_ERROR_NEEDS_GC;
    if(!ti_SetArchiveStatus(true, db)) return HOOK_ERROR_DATABASE_CORRUPTED;
    ti_Delete(DB_NAME);
    ti_Close(db);
    if(ti_Rename(DB_TEMP_NAME, DB_NAME)) return HOOK_ERROR_DATABASE_CORRUPTED;

    return refresh_hooks();
}

// todo: probably use a more sophisticated way of doing this, as Mateo mentioned
// if so, maybe also provide that method to users rather than leaving memory allocation up to them
void *flash_relocate(void *data, size_t size) {
    void *install_location;
    if(!ti_ArchiveHasRoom(size)) return NULL;
    ti_var_t slot = ti_Open("TMP", "w");
    ti_Write(data, size, 1, slot);
    ti_SetArchiveStatus(true, slot);
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
    ti_var_t db = ti_Open(DB_NAME, "r");
    void *ptr = ti_GetDataPtr(db);
    database_header_t header;
    ti_Read(&header, sizeof(header), 1, db);
    user_hook_entry_t current_entry;
    dbg_sprintf(dbgout, "DB: %p | Size: %u | Version: %u | Entry size: %u\n", ptr, ti_GetSize(db), header.version, sizeof(current_entry));
    for(int i = 0; ti_Read(&current_entry, sizeof(current_entry), 1, db); i++) {
        dbg_sprintf(dbgout, "Entry %u: id: %06X | hook: %p | type: %2u | priority: %3u | enabled: %c | description: %s\n",
                i, current_entry.id, current_entry.hook, current_entry.type, current_entry.priority, current_entry.enabled ? 'T' : 'F', current_entry.description);
    }
    ti_Close(db);
}
#endif
