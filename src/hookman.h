#include <stdint.h>
#include <stdbool.h>

enum {
    CURSOR = 0,
    LIBRARY,
    RAW_KEY,
    GET_CSC,
    HOMESCREEN,
    WINDOW,
    GRAPH,
    Y_EQUALS,
    FONT,
    REGRAPH,
    GRAPHICS,
    TRACE,
    PARSER,
    APP_CHANGE,
    CATALOG1,
    HELP,
    CX_REDISP,
    MENU,
    CATALOG2,
    TOKEN,
    LOCALIZE,
    SILENT_LINK,
    USB_ACTIVITY,
    NUM_HOOK_TYPES,
    HOOK_TYPE_UNKNOWN = 0xFF
};
typedef uint8_t hook_type_t;

enum {
    HOOK_SUCCESS = 0,
    HOOK_ERROR_NO_MATCHING_ID,
    HOOK_ERROR_INVALID_USER_HOOK,
    HOOK_ERROR_UNKNOWN_TYPE,
    HOOK_ERROR_DESCRIPTION_TOO_LONG,
    HOOK_ERROR_INTERNAL_DATABASE_IO,
    HOOK_ERROR_NEEDS_GC,
    HOOK_ERROR_DATABASE_CORRUPTED,
    HOOK_ERROR_UNKNOWN_DATABASE_VERSION
};
typedef uint8_t hook_error_t;

typedef void hook_t;

hook_error_t refresh_hooks(void);

hook_error_t install_hook(uint24_t id, hook_t *hook, hook_type_t type, uint8_t priority, char *description);
hook_error_t uninstall_hook(uint24_t id);

hook_error_t set_hook_priority(uint24_t id, uint8_t priority);

hook_error_t enable_hook(uint24_t id);
hook_error_t disable_hook(uint24_t id);

hook_error_t is_hook_installed(uint24_t id, bool *result);
hook_error_t get_hook_by_id(uint24_t id, hook_t **result);
hook_error_t get_hook_type(uint24_t id, hook_type_t *result);
hook_error_t get_hook_priority(uint24_t id, uint8_t *result);
hook_error_t is_hook_enabled(uint24_t id, bool *result);
hook_error_t get_hook_description(uint24_t id, char **result);

hook_error_t check_hook_validity(uint24_t id);

// todo: function to suspend updates? that is, don't actually write anything to flash until another function is called, at which point all changes are made
