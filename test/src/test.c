#include <stdbool.h>
#include <string.h>
#include <tice.h>

#undef NDEBUG
#include <debug.h>
#include <fileioc.h>
#include "../../src/capnhook.h"

// These aren't relocated, since we don't need to test them after the program exits
extern hook_t hook_1, hook_2, hook_3, hook_os;

extern size_t hook_1_size;

extern bool hook_1_run, hook_2_run, hook_3_run, hook_os_run;

uint8_t trigger_key_hook(uint8_t a);
void set_key_hook(hook_t *hook);

bool check_hook(uint24_t type);
void clear_hook(uint24_t type);

void debug_print_db(void);

#define ASSERT(cond) if(!(cond)) {dbg_sprintf((char*)dbgout, "Assertion failed on line %u\n", __LINE__); return false;} else dbg_sprintf((char*)dbgout, "Assertion passed on line %u\n", __LINE__)
#define ASSERT_EQUAL(var, known) if(var != known) {dbg_sprintf((char*)dbgout, "Assertion failed on line %u, value 0x%X\n", __LINE__, (int)var); return false;} else dbg_sprintf((char*)dbgout, "Assertion passed on line %u\n", __LINE__)

// todo: test installing hooks with an actual size

bool check_tests(void) {
    uint8_t priority;
    uint8_t type;
    size_t size;
    char *description;
    hook_t *hook;
    hook_error_t err;
    bool installed;
    // Assume that the calc has been freshly reset
    // Delete old hook db
    ti_CloseAll();
    ti_Delete("HOOKSDB");
    ti_Delete("HOOKTMP");
    debug_print_db();

    clear_hook(HOOK_TYPE_RAW_KEY);

    ASSERT_EQUAL(check_hook(HOOK_TYPE_RAW_KEY), false);

    err = hook_IsInstalled(0xFF0001, &installed);
    ASSERT_EQUAL(err, HOOK_SUCCESS);
    ASSERT_EQUAL(installed, false);

    // Install test hooks
    ASSERT(hook_1_size > 0);
    err = hook_Install(0xFF0001, &hook_1, hook_1_size, HOOK_TYPE_RAW_KEY, 10, "Test Hook 1");
    ASSERT_EQUAL(err, HOOK_SUCCESS);
    debug_print_db();

    err = hook_IsInstalled(0xFF0001, &installed);
    ASSERT_EQUAL(err, HOOK_SUCCESS);
    ASSERT_EQUAL(installed, true);

    err = hook_IsInstalled(0xFF0002, &installed);
    ASSERT_EQUAL(err, HOOK_SUCCESS);
    ASSERT_EQUAL(installed, false);

    err = hook_IsInstalled(HOOK_TYPE_RAW_KEY, &installed);
    ASSERT_EQUAL(err, HOOK_SUCCESS);
    ASSERT_EQUAL(installed, false);

    ASSERT_EQUAL(hook_Sync(), HOOK_SUCCESS);
    ASSERT_EQUAL(ti_Open("HOOKTMP", "r"), 0);
    ASSERT_EQUAL(check_hook(HOOK_TYPE_RAW_KEY), true);

    err = hook_Install(0xFF0002, &hook_2, 0, HOOK_TYPE_RAW_KEY, 20, "Test Hook 2");
    ASSERT_EQUAL(err, HOOK_SUCCESS);
    debug_print_db();

    err = hook_IsInstalled(0xFF0002, &installed);
    ASSERT_EQUAL(err, HOOK_SUCCESS);
    ASSERT_EQUAL(installed, true);

    err = hook_Install(0xFF0003, &hook_3, 0, HOOK_TYPE_RAW_KEY, 25, "Test Hook 3");
    ASSERT_EQUAL(err, HOOK_SUCCESS);
    debug_print_db();

    err = hook_IsInstalled(0xFF0002, &installed);
    ASSERT_EQUAL(err, HOOK_SUCCESS);
    ASSERT_EQUAL(installed, true);

    // Check all of the get/is functions
    err = hook_GetPriority(0xFF0001, &priority);
    ASSERT_EQUAL(err, HOOK_SUCCESS);
    ASSERT_EQUAL(priority, 10);
    err = hook_GetType(0xFF0001, &type);
    ASSERT_EQUAL(err, HOOK_SUCCESS);
    ASSERT_EQUAL(type, HOOK_TYPE_RAW_KEY);
    err = hook_GetSize(0xFF0001, &size);
    ASSERT_EQUAL(err, HOOK_SUCCESS);
    ASSERT_EQUAL(size, hook_1_size);
    err = hook_GetHook(0xFF0001, &hook);
    ASSERT_EQUAL(err, HOOK_SUCCESS);
    ASSERT_EQUAL(memcmp(hook, &hook_1, size), 0);
    err = hook_GetDescription(0xFF0001, &description);
    ASSERT_EQUAL(err, HOOK_SUCCESS);
    ASSERT(strcmp(description, "Test Hook 1") == 0);
    err = hook_IsEnabled(0xFF0001, &installed);
    ASSERT_EQUAL(err, HOOK_SUCCESS);
    ASSERT_EQUAL(installed, true);

    // Change priority
    err = hook_SetPriority(0xFF0001, 15);
    ASSERT_EQUAL(err, HOOK_SUCCESS);
    debug_print_db();
    err = hook_GetPriority(0xFF0001, &priority);
    ASSERT_EQUAL(err, HOOK_SUCCESS);
    ASSERT_EQUAL(priority, 15);

    // Test reinstalling with the same ID
    err = hook_Install(0xFF0001, &hook_1, 0, HOOK_TYPE_RAW_KEY, 10, "Test Hook 1 - Alt");
    ASSERT_EQUAL(err, HOOK_SUCCESS);
    debug_print_db();

    err = hook_GetPriority(0xFF0001, &priority);
    ASSERT_EQUAL(err, HOOK_SUCCESS);
    ASSERT_EQUAL(priority, 15);

    err = hook_GetDescription(0xFF0001, &description);
    ASSERT_EQUAL(err, HOOK_SUCCESS);
    ASSERT(strcmp(description, "Test Hook 1 - Alt") == 0);

    ASSERT_EQUAL(hook_Sync(), HOOK_SUCCESS);

    // Manually trigger hook
    uint8_t a = trigger_key_hook(0);
    ASSERT_EQUAL(a, 0);
    ASSERT_EQUAL(hook_1_run, true);
    ASSERT_EQUAL(hook_2_run, true);
    ASSERT_EQUAL(hook_3_run, true);
    ASSERT_EQUAL(hook_os_run, false);

    // Check if priorities are processed properly
    a = trigger_key_hook(0x9A);
    ASSERT_EQUAL(a, 0x9B);
    ASSERT_EQUAL(hook_1_run, true);
    ASSERT_EQUAL(hook_2_run, false);
    ASSERT_EQUAL(hook_3_run, false);

    a = trigger_key_hook(0x9C);
    ASSERT_EQUAL(a, 0x9D);
    ASSERT_EQUAL(hook_1_run, true);
    ASSERT_EQUAL(hook_2_run, true);
    ASSERT_EQUAL(hook_3_run, false);

    // todo: re-enable somehow?
    // Check that imported OS hooks behave correctly
//    set_key_hook(&hook_os);
//    a = trigger_key_hook(0x9F);
//    ASSERT_EQUAL(a, 0xA0);
//    ASSERT_EQUAL(hook_1_run, false);
//    ASSERT_EQUAL(hook_2_run, false);
//    ASSERT_EQUAL(hook_3_run, false);
//    ASSERT_EQUAL(hook_os_run, true);

//    existing_checked = false;
//    ASSERT_EQUAL(hook_Sync(), HOOK_SUCCESS);
//    debug_print_db();
//    err = hook_IsInstalled(HOOK_TYPE_RAW_KEY, &installed);
//    ASSERT_EQUAL(err, HOOK_SUCCESS);
//    ASSERT_EQUAL(installed, true);
//    err = hook_GetPriority(HOOK_TYPE_RAW_KEY, &priority);
//    ASSERT_EQUAL(err, HOOK_SUCCESS);
//    ASSERT_EQUAL(priority, 255);
//    err = hook_GetType(HOOK_TYPE_RAW_KEY, &type);
//    ASSERT_EQUAL(err, HOOK_SUCCESS);
//    ASSERT_EQUAL(type, HOOK_TYPE_RAW_KEY);

//    a = trigger_key_hook(0x9F);
//    ASSERT_EQUAL(a, 0xA0);
//    ASSERT_EQUAL(hook_1_run, true);
//    ASSERT_EQUAL(hook_2_run, true);
//    ASSERT_EQUAL(hook_3_run, true);
//    ASSERT_EQUAL(hook_os_run, true);

    // Disable a hook
    err = hook_Disable(0xFF0001);
    ASSERT_EQUAL(err, 0);
    debug_print_db();

    err = hook_IsEnabled(0xFF0001, &installed);
    ASSERT_EQUAL(err, HOOK_SUCCESS);
    ASSERT_EQUAL(installed, false);

    ASSERT_EQUAL(hook_Sync(), HOOK_SUCCESS);

    a = trigger_key_hook(0x9A);
    ASSERT_EQUAL(a, 0x9A);
    ASSERT_EQUAL(hook_1_run, false);
    ASSERT_EQUAL(hook_2_run, true);
    ASSERT_EQUAL(hook_3_run, true);

    // Re-enable hook
    err = hook_Enable(0xFF0001);
    ASSERT_EQUAL(err, 0);
    debug_print_db();

    err = hook_IsEnabled(0xFF0001, &installed);
    ASSERT_EQUAL(err, HOOK_SUCCESS);
    ASSERT_EQUAL(installed, true);

    ASSERT_EQUAL(hook_Sync(), HOOK_SUCCESS);

    a = trigger_key_hook(0x9A);
    ASSERT_EQUAL(a, 0x9B);
    ASSERT_EQUAL(hook_1_run, true);
    ASSERT_EQUAL(hook_2_run, false);
    ASSERT_EQUAL(hook_3_run, false);

    // Hook validation
    err = hook_CheckValidity(0xFF0001);
    ASSERT_EQUAL(err, HOOK_SUCCESS);

    *(uint8_t*)&hook_1 = 0x00;

    err = hook_CheckValidity(0xFF0001);
    ASSERT_EQUAL(err, HOOK_ERROR_INVALID_USER_HOOK);

    *(uint8_t*)&hook_1 = 0x83;

    err = hook_CheckValidity(0xFF0001);
    ASSERT_EQUAL(err, HOOK_SUCCESS);

    // Uninstall hook
    err = hook_Uninstall(0xFF0001);
    ASSERT_EQUAL(err, 0);
    debug_print_db();

    err = hook_IsInstalled(0xFF0001, &installed);
    ASSERT_EQUAL(err, HOOK_SUCCESS);
    ASSERT_EQUAL(installed, false);

    ASSERT_EQUAL(hook_Sync(), HOOK_SUCCESS);

    a = trigger_key_hook(0x9A);
    ASSERT_EQUAL(a, 0x9A);
    ASSERT_EQUAL(hook_1_run, false);
    ASSERT_EQUAL(hook_2_run, true);
    ASSERT_EQUAL(hook_3_run, true);

    return true;
}

int main(void) {
    os_ClrHomeFull();

    if(check_tests()) {
        os_PutStrLine("PASS");
        dbg_sprintf(dbgout, "All tests passed.\n");
    } else {
        os_PutStrLine("FAIL");
    }

    while(!os_GetCSC());
}

#define DB_NAME "HOOKSDB"
#define DB_TEMP_NAME "HOOKTMP"

typedef struct {
    uint24_t version;
} database_header_t;

typedef struct {
    uint24_t id;
    hook_t *hook;			// pointer to the user hook
    uint8_t type;			// enum for which OS hook to use
    uint8_t priority;		// process lowest priorities first
    bool enabled;
    uint24_t size;
    char description[1];
} user_hook_entry_t;

user_hook_entry_t *get_next(user_hook_entry_t *entry);

user_hook_entry_t *get_next(user_hook_entry_t *entry) {
    return (user_hook_entry_t*)&entry->description[strlen(entry->description) + 1];
}

void debug_print_db(void) {
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
        dbg_sprintf(dbgout, "id: %06X | hook: %p | type: %2u | priority: %3u | enabled: %c | size: %u | description: %s\n",
                    current->id, current->hook, current->type, current->priority, current->enabled ? 'T' : 'F', current->size, current->description);
    }
    ti_Close(db);
}
