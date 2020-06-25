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

/**
 * Registers the contents of the hook database with the OS
 *
 * This function is mostly for internal use, and is called by every function
 * that modifies the database.
 */
hook_error_t refresh_hooks(void);

/**
 * Installs a hook
 *
 * If another hook with the same ID is already installed, the old hook will be
 * replaced with the new one. The old hook's priority will be preserved.
 *
 * @param id A unique id for the hook. This must be unique among all programs
 * that use this library. You should register your ID on this page:
 * https://github.com/commandblockguy/capnhook/wiki/Hook-ID-Registry
 * @param hook A pointer to the hook to install. The hook should already be in
 * a persistent location such as the archive, as this function does not
 * relocate the hooks.
 * @param type The hook type determines what type of OS event the hook will
 * trigger on.
 * @param priority Hooks with lower priority values are called prior to hooks
 * with higher priority values.
 * @param description A human-readable description of the hook, up to 64 chars.
 * @return An error code or HOOK_SUCCESS
 */
hook_error_t install_hook(uint24_t id, hook_t *hook, hook_type_t type, uint8_t priority, const char *description);

/**
 * Uninstall a hook
 * @param id The hook ID
 * @return An error code or HOOK_SUCCESS
 */
hook_error_t uninstall_hook(uint24_t id);

/**
 * Sets the priority of a hook
 * @param id The hook ID
 * @param priority Hooks with lower priority values are called prior to hooks
 * with higher priority values.
 * @return An error code or HOOK_SUCCESS
 */
hook_error_t set_hook_priority(uint24_t id, uint8_t priority);

/**
 * Enable a hook
 * @param id The hook ID
 * @return An error code or HOOK_SUCCESS
 */
hook_error_t enable_hook(uint24_t id);

/**
 * Disable a hook
 * @param id The hook ID
 * @return An error code or HOOK_SUCCESS
 */
hook_error_t disable_hook(uint24_t id);

/**
 * Check if a hook is installed
 * @param id The hook ID
 * @param result Set to 1 if the hook is installed, or 0 if it is not
 * @return An error code or HOOK_SUCCESS
 */
hook_error_t is_hook_installed(uint24_t id, bool *result);

/**
 * Get a pointer to a hook
 * @param id The hook ID
 * @param result Set to a pointer to the hook
 * @return An error code or HOOK_SUCCESS
 */
hook_error_t get_hook_by_id(uint24_t id, hook_t **result);

/**
 * Get a hook's type
 * @param id The hook ID
 * @param result Set to the hook's type
 * @return An error code or HOOK_SUCCESS
 */
hook_error_t get_hook_type(uint24_t id, hook_type_t *result);

/**
 * Get the priority of a hook
 * @param id The hook ID
 * @param result Set to the hook's priority
 * @return An error code or HOOK_SUCCESS
 */
hook_error_t get_hook_priority(uint24_t id, uint8_t *result);

/**
 * Check if a hook is enabled
 * @param id The hook ID
 * @param result Set to 1 if the hook is enabled or 0 otherwise
 * @return An error code or HOOK_SUCCESS
 */
hook_error_t is_hook_enabled(uint24_t id, bool *result);

/**
 * Get the description of a hook
 * @param id The hook ID
 * @param result Set to a pointer to the hook description
 * @return An error code or HOOK_SUCCESS
 */
hook_error_t get_hook_description(uint24_t id, char **result);

/**
 * Check if a hook is valid.
 * A hook is valid if the first byte is 0x83
 * @param id The hook ID
 * @return HOOK_SUCCESS if hook is valid, HOOK_ERROR_INVALID_USER_HOOK if
 * invalid, or another error
 */
hook_error_t check_hook_validity(uint24_t id);

// todo: function to suspend updates? that is, don't actually write anything to flash until another function is called, at which point all changes are made
