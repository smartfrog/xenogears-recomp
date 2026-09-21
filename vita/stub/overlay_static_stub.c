/* Vita Phase 0 stub for the AOT static overlay dispatch shards
 * (overlays_static_dispatch*.c). No statically linked overlay is known,
 * so both predicates decline and overlay execution stays in the
 * dirty-RAM interpreter. ZERO game-derived content. */
#include <stdint.h>

#include "cpu_state.h"

int psx_overlay_dispatch(CPUState *cpu, uint32_t addr)
{
    (void)cpu;
    (void)addr;
    return 0;
}

int psx_overlay_static_image_known(uint32_t addr)
{
    (void)addr;
    return 0;
}

int psx_overlay_static_can_dispatch(uint32_t addr)
{
    (void)addr;
    return 0;
}

void psx_overlay_static_get_stats(uint64_t *checks, uint64_t *hits,
                                  uint64_t *variant_misses,
                                  uint64_t *address_misses)
{
    if (checks) *checks = 0;
    if (hits) *hits = 0;
    if (variant_misses) *variant_misses = 0;
    if (address_misses) *address_misses = 0;
}

void psx_overlay_static_image_get_stats(uint64_t *image_checks,
                                        uint64_t *image_hits,
                                        uint64_t *image_misses)
{
    if (image_checks) *image_checks = 0;
    if (image_hits) *image_hits = 0;
    if (image_misses) *image_misses = 0;
}

int psx_overlay_static_artifact_code_write_overlaps(
    const uint8_t sha256[32], uint32_t base, uint32_t size,
    uint32_t address, uint32_t write_size)
{
    (void)sha256; (void)base; (void)size; (void)address; (void)write_size;
    return -1;
}
