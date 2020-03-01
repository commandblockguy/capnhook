/*
 *--------------------------------------
 * Program Name:
 * Author:
 * License:
 * Description:
 *--------------------------------------
*/

#include <stddef.h>
#include <fileioc.h>
#undef NDEBUG
#include <debug.h>
#include "hookman.h"

void *flash_relocate(void *data, size_t size);

void user_hooks(void);

extern size_t user_hook_size;

int main(void) {
    hook_t *user_hook_loc;
    ti_CloseAll();

    dbg_sprintf(dbgout, "\n\n\nProgram started.\n");

    ti_Delete("HOOKDB");

    user_hook_loc = flash_relocate(user_hooks, user_hook_size * 3);

    dbg_sprintf(dbgout, "Copied user hooks to %p\n", user_hook_loc);

    install_hook(1, user_hook_loc, RAW_KEY, 0, "A->B");
    install_hook(2, user_hook_loc + user_hook_size, RAW_KEY, 0, "C->D");
    install_hook(3, user_hook_loc + 2 * user_hook_size, RAW_KEY, 0, "E->F");

    return 0;
}
