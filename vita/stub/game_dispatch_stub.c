/* Vita Phase 0 stub for generated/slus_006.64_dispatch.c.
 * Every predicate answers "not native/not handled" so the dirty-RAM
 * interpreter path is always taken, exactly like a BIOS-only runtime.
 * ZERO game-derived content: no addresses, no code, no data. */
#include <stdint.h>

#include "cpu_state.h"

int psx_dispatch_game_compiled(CPUState *cpu, uint32_t addr)
{
    (void)cpu;
    (void)addr;
    return 0;
}

int psx_game_address_in_text(uint32_t addr)
{
    (void)addr;
    return 0;
}

int psx_game_is_function_entry(uint32_t addr)
{
    (void)addr;
    return 0;
}

int psx_game_text_native_ok(uint32_t addr)
{
    (void)addr;
    return 0;
}

int psx_game_text_native_ok_full(uint32_t addr)
{
    (void)addr;
    return 0;
}
