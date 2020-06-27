#include <stdbool.h>
#include <string.h>
#include <tice.h>

#undef NDEBUG
#include <debug.h>
#include <fileioc.h>
#include "hookman.h"

// These aren't relocated, since we don't need to test them after the program exits
extern hook_t hook_1, hook_2, hook_3, hook_os;

extern bool hook_1_run, hook_2_run, hook_3_run, hook_os_run;

uint8_t trigger_key_hook(uint8_t a);
void set_key_hook(hook_t *hook);

bool check_hook(uint24_t type);
void clear_hook(uint24_t type);

#define ASSERT(cond) if(!(cond)) {dbg_sprintf((char*)dbgout, "Assertion failed on line %u\n", __LINE__); return false;} else dbg_sprintf((char*)dbgout, "Assertion passed on line %u\n", __LINE__)
#define ASSERT_EQUAL(var, known) if(var != known) {dbg_sprintf((char*)dbgout, "Assertion failed on line %u, value 0x%X\n", __LINE__, (int)var); return false;} else dbg_sprintf((char*)dbgout, "Assertion passed on line %u\n", __LINE__)

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

    clear_hook(RAW_KEY);

    ASSERT_EQUAL(check_hook(RAW_KEY), false);

    err = is_hook_installed(0xFF0001, &installed);
    ASSERT_EQUAL(err, HOOK_SUCCESS);
    ASSERT_EQUAL(installed, false);

    // Install test hooks
    err = install_hook(0xFF0001, &hook_1, RAW_KEY, 10, "Test Hook 1");
    ASSERT_EQUAL(err, HOOK_SUCCESS);

    err = is_hook_installed(0xFF0001, &installed);
    ASSERT_EQUAL(err, HOOK_SUCCESS);
    ASSERT_EQUAL(installed, true);

    err = is_hook_installed(0xFF0002, &installed);
    ASSERT_EQUAL(err, HOOK_SUCCESS);
    ASSERT_EQUAL(installed, false);

    err = is_hook_installed(RAW_KEY, &installed);
    ASSERT_EQUAL(err, HOOK_SUCCESS);
    ASSERT_EQUAL(installed, false);

    ASSERT_EQUAL(check_hook(RAW_KEY), true);

    err = install_hook(0xFF0002, &hook_2, RAW_KEY, 20, "Test Hook 2");
    ASSERT_EQUAL(err, HOOK_SUCCESS);

    err = is_hook_installed(0xFF0002, &installed);
    ASSERT_EQUAL(err, HOOK_SUCCESS);
    ASSERT_EQUAL(installed, true);

    err = install_hook(0xFF0003, &hook_3, RAW_KEY, 25, "Test Hook 2");
    ASSERT_EQUAL(err, HOOK_SUCCESS);

    err = is_hook_installed(0xFF0002, &installed);
    ASSERT_EQUAL(err, HOOK_SUCCESS);
    ASSERT_EQUAL(installed, true);

    // Check all of the get/is functions
    err = get_hook_priority(0xFF0001, &priority);
    ASSERT_EQUAL(err, HOOK_SUCCESS);
    ASSERT_EQUAL(priority, 10);
    err = get_hook_type(0xFF0001, &type);
    ASSERT_EQUAL(err, HOOK_SUCCESS);
    ASSERT_EQUAL(type, RAW_KEY);
    err = get_hook_by_id(0xFF0001, &hook);
    ASSERT_EQUAL(err, HOOK_SUCCESS);
    ASSERT_EQUAL(hook, &hook_1);
    err = get_hook_description(0xFF0001, &description);
    ASSERT_EQUAL(err, HOOK_SUCCESS);
    ASSERT(strcmp(description, "Test Hook 1") == 0);
    err = is_hook_enabled(0xFF0001, &installed);
    ASSERT_EQUAL(err, HOOK_SUCCESS);
    ASSERT_EQUAL(installed, true);

    // Change priority
    err = set_hook_priority(0xFF0001, 15);
    ASSERT_EQUAL(err, HOOK_SUCCESS);
    err = get_hook_priority(0xFF0001, &priority);
    ASSERT_EQUAL(err, HOOK_SUCCESS);
    ASSERT_EQUAL(priority, 15);

    // Test reinstalling with the same ID
    err = install_hook(0xFF0001, &hook_1, RAW_KEY, 10, "Test Hook 1 - Alt");
    ASSERT_EQUAL(err, HOOK_SUCCESS);

    err = get_hook_priority(0xFF0001, &priority);
    ASSERT_EQUAL(err, HOOK_SUCCESS);
    ASSERT_EQUAL(priority, 15);

    err = get_hook_description(0xFF0001, &description);
    ASSERT_EQUAL(err, HOOK_SUCCESS);
    ASSERT(strcmp(description, "Test Hook 1 - Alt") == 0);

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

    // Check that imported OS hooks behave correctly
    set_key_hook(&hook_os);
    a = trigger_key_hook(0x9F);
    ASSERT_EQUAL(a, 0xA0);
    ASSERT_EQUAL(hook_1_run, false);
    ASSERT_EQUAL(hook_2_run, false);
    ASSERT_EQUAL(hook_3_run, false);
    ASSERT_EQUAL(hook_os_run, true);

    refresh_hooks();
    err = is_hook_installed(RAW_KEY, &installed);
    ASSERT_EQUAL(err, HOOK_SUCCESS);
    ASSERT_EQUAL(installed, true);
    err = get_hook_priority(RAW_KEY, &priority);
    ASSERT_EQUAL(err, HOOK_SUCCESS);
    ASSERT_EQUAL(priority, 255);
    err = get_hook_type(RAW_KEY, &type);
    ASSERT_EQUAL(err, HOOK_SUCCESS);
    ASSERT_EQUAL(type, RAW_KEY);

    a = trigger_key_hook(0x9F);
    ASSERT_EQUAL(a, 0xA0);
    ASSERT_EQUAL(hook_1_run, true);
    ASSERT_EQUAL(hook_2_run, true);
    ASSERT_EQUAL(hook_3_run, true);
    ASSERT_EQUAL(hook_os_run, true);

    // Disable a hook
    err = disable_hook(0xFF0001);
    ASSERT_EQUAL(err, 0);

    err = is_hook_enabled(0xFF0001, &installed);
    ASSERT_EQUAL(err, HOOK_SUCCESS);
    ASSERT_EQUAL(installed, false);

    a = trigger_key_hook(0x9A);
    ASSERT_EQUAL(a, 0x9A);
    ASSERT_EQUAL(hook_1_run, false);
    ASSERT_EQUAL(hook_2_run, true);
    ASSERT_EQUAL(hook_3_run, true);

    // Re-enable hook
    err = enable_hook(0xFF0001);
    ASSERT_EQUAL(err, 0);

    err = is_hook_enabled(0xFF0001, &installed);
    ASSERT_EQUAL(err, HOOK_SUCCESS);
    ASSERT_EQUAL(installed, true);

    a = trigger_key_hook(0x9A);
    ASSERT_EQUAL(a, 0x9B);
    ASSERT_EQUAL(hook_1_run, true);
    ASSERT_EQUAL(hook_2_run, false);
    ASSERT_EQUAL(hook_3_run, false);

    // Hook validation
    err = check_hook_validity(0xFF0001);
    ASSERT_EQUAL(err, HOOK_SUCCESS);

    *(uint8_t*)&hook_1 = 0x00;

    err = check_hook_validity(0xFF0001);
    ASSERT_EQUAL(err, HOOK_ERROR_INVALID_USER_HOOK);

    *(uint8_t*)&hook_1 = 0x83;

    err = check_hook_validity(0xFF0001);
    ASSERT_EQUAL(err, HOOK_SUCCESS);

    // Uninstall hook
    err = uninstall_hook(0xFF0001);
    ASSERT_EQUAL(err, 0);

    err = is_hook_installed(0xFF0001, &installed);
    ASSERT_EQUAL(err, HOOK_SUCCESS);
    ASSERT_EQUAL(installed, false);

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
