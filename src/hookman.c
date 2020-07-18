#include "hookman.h"

void sort_by_priority(hook_t **hooks, uint8_t *priorities, uint8_t num_hooks) {
    for(uint8_t i = 1; i < num_hooks; i++) {
        hook_t *hook = hooks[i];
        uint8_t priority = priorities[i];
        int24_t j;
        for(j = i - 1; j >= 0 && priorities[j] > priority; j--) {
            hooks[j + 1] = hooks[j];
            priorities[j + 1] = priorities[j];
        }
        hooks[j + 1] = hook;
        priorities[j + 1] = priority;
    }
}
