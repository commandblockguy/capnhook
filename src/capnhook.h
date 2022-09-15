#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

enum {
    HOOK_TYPE_CURSOR = 0,
    HOOK_TYPE_LIBRARY,
    HOOK_TYPE_RAW_KEY,
    HOOK_TYPE_GET_CSC,
    HOOK_TYPE_HOMESCREEN,
    HOOK_TYPE_WINDOW,
    HOOK_TYPE_GRAPH,
    HOOK_TYPE_Y_EQUALS,
    HOOK_TYPE_FONT,
    HOOK_TYPE_REGRAPH,
    HOOK_TYPE_GRAPHICS,
    HOOK_TYPE_TRACE,
    HOOK_TYPE_PARSER,
    HOOK_TYPE_APP_CHANGE,
    HOOK_TYPE_CATALOG1,
    HOOK_TYPE_HELP,
    HOOK_TYPE_CX_REDISP,
    HOOK_TYPE_MENU,
    HOOK_TYPE_CATALOG2,
    HOOK_TYPE_TOKEN,
    HOOK_TYPE_LOCALIZE,
    HOOK_TYPE_SILENT_LINK,
    HOOK_TYPE_USB_ACTIVITY,
    HOOK_NUM_TYPES,
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

struct hook_info {
    /**
     * A unique id for the hook. This must be unique among all programs
     * that use this library. You should register your ID on this page:
     * https://github.com/commandblockguy/capnhook/wiki/Hook-ID-Registry
     */
    uint24_t id;
    /**
     * Determines what type of OS event the hook will trigger on.
     */ 
    hook_type type;
    /**
     * A pointer to a hook, as described in
     * https://wikiti.brandonw.net/index.php?title=83Plus:OS:Hooks
     * 
     * This hook should not include chaining code, since that is handled
     * automatically by capnhook. Instead, it should reset bit 0 of (flags-10)
     * when returning a value to the OS, or set the bit if other hooks of the
     * same type should be processed.
     */
    void *pointer;
    /**
     * The type byte and name of a variable. If not empty, `pointer` will be
     * used as an offset relative to the start of this variable rather than an
     * absolute pointer.
     */
    char variable[9];
    /**
     * Hooks with lower priority values are called before hooks with higher
     * priorities.
     */
    uint8_t priority;
    /**
     * Whether this hook is currently enabled.
     */
    bool enabled;
    /**
     * A human-readable description of the hook, up to 255 characters long.
     */
    const char *description;
    const uint8_t reserved[16];
}

/**
 * Apply changes made to the hook database
 * @return An error code or HOOK_SUCCESS
 */
hook_error_t hook_Sync(void);

/**
 * Discard all changes to the hook database since the last hook_Sync()
 * @return An error code or HOOK_SUCCESS
 */
hook_error_t hook_Discard(void);

/**
 * Installs a hook.
 *
 * If another hook with the same ID is already installed, the old hook will be
 * replaced with the new one. The old hook's priority will be preserved. // todo: mixed feelings about this last bit
 *
 * @param info The hook info struct.
 * @return An error code or HOOK_SUCCESS
 */
hook_error_t hook_Install(const hook_info *info);

/**
 * Uninstall a hook.
 * @param id The hook ID
 * @return An error code or HOOK_SUCCESS
 */
hook_error_t hook_Uninstall(uint24_t id);

/**
 * Get a hook by its ID.
 * 
 * @param id The hook ID
 * @param info The hook info
 */
hook_error_t hook_Get(uint24_t id, hook_info *info);

/**
 * Iterate through all hooks one at a time.
 *
 * @param info Set info->id to -1 to start scanning for hooks from the
 * beginning. The info for each hook will be copied into it.
 * @return HOOK_SUCCESS if @param id was set to the next hook ID,
 * HOOK_ERROR_NO_MATCHING_HOOK once all IDs have already been seen, or another
 * error
 */
hook_error_t hook_Next(hook_info *info);
