#include <stdbool.h>
#include <string.h>
#include <tice.h>

#undef NDEBUG
#include <debug.h>
#include <fileioc.h>
#include "hookman.h"

extern hook_t hook_1, hook_2, hook_3;

extern bool hook_1_run, hook_2_run, hook_3_run;

uint8_t trigger_key_hook(uint8_t a);

#define ASSERT(cond) if(!(cond)) {dbg_sprintf((char*)dbgout, "Assertion failed on line %u\n", __LINE__); return false;}
#define ASSERT_EQUAL(var, known) if(var != known) {dbg_sprintf((char*)dbgout, "Assertion failed on line %u, value 0x%X\n", __LINE__, (int)var); return false;}

bool check_tests(void) {
    uint8_t priority;
    uint8_t type;
    char *description;
    hook_t *hook;
    hook_error_t err;
    bool installed;
    // Assume that the calc has been freshly reset
    // Delete old hook db
    ti_CloseAll();
    ti_Delete("HOOKDB");

    err = is_hook_installed(1, &installed);
    ASSERT_EQUAL(err, HOOK_SUCCESS);
    ASSERT_EQUAL(installed, false);

    // Install test hooks
    err = install_hook(1, &hook_1, RAW_KEY, 10, "Test Hook 1");
    ASSERT_EQUAL(err, HOOK_SUCCESS);

    err = is_hook_installed(1, &installed);
    ASSERT_EQUAL(err, HOOK_SUCCESS);
    ASSERT_EQUAL(installed, true);

    err = is_hook_installed(2, &installed);
    ASSERT_EQUAL(err, HOOK_SUCCESS);
    ASSERT_EQUAL(installed, false);

    err = install_hook(2, &hook_2, RAW_KEY, 20, "Test Hook 2");
    ASSERT_EQUAL(err, HOOK_SUCCESS);

    err = is_hook_installed(2, &installed);
    ASSERT_EQUAL(err, HOOK_SUCCESS);
    ASSERT_EQUAL(installed, true);

    err = install_hook(3, &hook_3, RAW_KEY, 25, "Test Hook 2");
    ASSERT_EQUAL(err, HOOK_SUCCESS);

    err = is_hook_installed(2, &installed);
    ASSERT_EQUAL(err, HOOK_SUCCESS);
    ASSERT_EQUAL(installed, true);

    // Check all of the get/is functions
    err = get_hook_priority(1, &priority);
    ASSERT_EQUAL(err, HOOK_SUCCESS);
    ASSERT_EQUAL(priority, 10);
    err = get_hook_type(1, &type);
    ASSERT_EQUAL(err, HOOK_SUCCESS);
    ASSERT_EQUAL(type, RAW_KEY);
    err = get_hook_by_id(1, &hook);
    ASSERT_EQUAL(err, HOOK_SUCCESS);
    ASSERT_EQUAL(hook, &hook_1);
    err = get_hook_description(1, &description);
    ASSERT_EQUAL(err, HOOK_SUCCESS);
    ASSERT(strcmp(description, "Test Hook 1") == 0);
    err = is_hook_enabled(1, &installed);
    ASSERT_EQUAL(err, HOOK_SUCCESS);
    ASSERT_EQUAL(installed, true);

    // Change priority
    err = set_hook_priority(1, 15);
    ASSERT_EQUAL(err, HOOK_SUCCESS);
    err = get_hook_priority(1, &priority);
    ASSERT_EQUAL(err, HOOK_SUCCESS);
    ASSERT_EQUAL(priority, 15);

    // Manually trigger hook
    uint8_t a = trigger_key_hook(0);
    ASSERT_EQUAL(a, 0);
    ASSERT_EQUAL(hook_1_run, true);
    ASSERT_EQUAL(hook_2_run, true);
    ASSERT_EQUAL(hook_3_run, true);

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

    // Disable a hook
    err = disable_hook(1);
    ASSERT_EQUAL(err, 0);

    err = is_hook_enabled(1, &installed);
    ASSERT_EQUAL(err, HOOK_SUCCESS);
    ASSERT_EQUAL(installed, false);

    a = trigger_key_hook(0x9A);
    ASSERT_EQUAL(a, 0x9A);
    ASSERT_EQUAL(hook_1_run, false);
    ASSERT_EQUAL(hook_2_run, true);
    ASSERT_EQUAL(hook_3_run, true);

    // Re-enable hook
    err = enable_hook(1);
    ASSERT_EQUAL(err, 0);

    err = is_hook_enabled(1, &installed);
    ASSERT_EQUAL(err, HOOK_SUCCESS);
    ASSERT_EQUAL(installed, true);

    a = trigger_key_hook(0x9A);
    ASSERT_EQUAL(a, 0x9B);
    ASSERT_EQUAL(hook_1_run, true);
    ASSERT_EQUAL(hook_2_run, false);
    ASSERT_EQUAL(hook_3_run, false);

    // Hook validation
    err = check_hook_validity(1);
    ASSERT_EQUAL(err, HOOK_SUCCESS);

    *(uint8_t*)&hook_1 = 0x00;

    err = check_hook_validity(1);
    ASSERT_EQUAL(err, HOOK_ERROR_INVALID_USER_HOOK);

    *(uint8_t*)&hook_1 = 0x83;

    err = check_hook_validity(1);
    ASSERT_EQUAL(err, HOOK_SUCCESS);

    // Uninstall hook
    err = uninstall_hook(1);
    ASSERT_EQUAL(err, 0);

    err = is_hook_installed(1, &installed);
    ASSERT_EQUAL(err, HOOK_SUCCESS);
    ASSERT_EQUAL(installed, false);

    a = trigger_key_hook(0x9A);
    ASSERT_EQUAL(a, 0x9A);
    ASSERT_EQUAL(hook_1_run, false);
    ASSERT_EQUAL(hook_2_run, true);
    ASSERT_EQUAL(hook_3_run, true);

    return true;
}

void run_tests(void) {
    os_ClrHomeFull();

    if(check_tests()) {
        os_PutStrLine("PASS");
        dbg_sprintf(dbgout, "All tests passed.\n");
    } else {
        os_PutStrLine("FAIL");
    }

    while(!os_GetCSC());
}
