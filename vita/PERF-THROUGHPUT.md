# Vita guest-throughput pass — evidence, changes, prediction

*Task: `vita-thr-ds` (worktree `/home/fred/projects/xgr-vita-thr-ds`, parent branch `vita-thr-ds`,
submodule branch `vita-port`). Baseline: `4a95b63` / submodule `454be008`.*

## TL;DR

- The guest runs at 0.29–0.99 MHz (0.8–2.9 % of PS1 realtime) and `vblank_body` is 1.65–9.11 ms
  against 570–1960 ms frames, so the wall time is inside guest execution — confirmed.
- Disassembly says **the generated code is not the bottleneck**. Two measured facts:
  (a) the generated TUs make 2.45 out-of-line calls per guest instruction, worth ≤ 3 % of the
  measured per-instruction cost; (b) ~110–135 *host instructions* are executed per guest
  instruction, of which the **per-instruction timing model is ~60–70 %** and a **dead
  native-renderer provenance test is ~35 % of the emitted text**.
- Applied (both Vita-only, desktop byte-identical): `PSX_NO_NATIVE_PROVENANCE` and
  `PSX_VITA_HOT_PATH`. Measured: generated text −26 %, SELF 70.4 MB → 51.7 MB (−26.5 %),
  VPK 28.6 MB → 21.3 MB.
- Rejected with numbers: `-O2` on the generated TUs (+35 % text, and it does **not** inline the
  per-instruction helpers).
- Predicted throughput gain: **1.15–1.35×** (0.99 → 1.14–1.34 MHz early, 0.29 → 0.33–0.39 MHz FMV).
  This does **not** close the 20–50× gap; that needs an emitter-level change (see §6).
- Added helper-call counters to the `[xg-phase]` rate line so the next hardware run converts the
  rate into host cycles per guest instruction and settles AOT-vs-interpreter attribution.

## 1. What could be measured (and what could not)

| Instrument | Available | Note |
|---|---|---|
| Hardware run | no | orchestrator measures after delivery |
| Vita3K | partially | boots and runs, then wedges on the known display-queue bug |
| Host build (desktop) | no | build host has no X11/Wayland/SDL2/SDL3 — SDL3 fetch refuses to configure |
| Vita cross-compile of a single TU | yes | 11 s per shard, exact text size + exact call counts |
| Preprocessor diff vs pristine tree | yes | byte-identity proof for desktop |

All numbers below come from the **arm-vita-eabi** toolchain, the **exact** Vita command lines
(`-O3 -DNDEBUG -mthumb -fzero-initialized-in-bss -Os`, `PSX_ENABLE_BLOCK_CYCLES=1`,
`PSX_NO_DEBUG_TOOLS=1`), and the previous session's hardware log
(`/tmp/opencode/vita-runtime-04b.log`, build `0.4-perf1-adopted`).

## 2. Evidence per suspect

### 2.1 Suspect 1 — `-Os` killed inlining of the accessors: confirmed, worth ≤ 3 %

`arm-vita-eabi-objdump -d` on the released `slus_006.64_full_00.c.obj`
(4 805 guest instructions, 196 724 text bytes):

```
$ arm-vita-eabi-objdump -d full_00_Os.o | grep -cE '\s(bl|blx)\s'
11800
$ ... | sed -E 's/.*<([^>]+)>.*/\1/' | sort | uniq -c | sort -rn | head
   4805 psx_cyc_step              <- one per guest instruction
   1811 psx_icache_fetch
   1712 gte_native_provenance_cpu_alu
    720 psx_slice_block.constprop.0
    545 psx_cyc_batch_flush
    545 psx_check_interrupts_at
    311 ram_provenance_note_cpu_store
    204 gte_native_provenance_cpu_store
    182 gte_native_provenance_cpu_load
    100 psx_gte_stall
     96 psx_cyc_load_byte
     59 psx_cyc_load_half
     44 psx_cyc_load_word
```

2.45 calls per guest instruction at ~4–8 host cycles each (Thumb-2 `bl` + return, local targets,
no veneers for these) is **≈ 12–18 cycles out of the 500–1200 measured** → ≤ 3 %.
`always_inline` on the memory accessors would not pay: the accessors are already single shared
out-of-line copies, and inlining them at ~5 000 call sites per shard is a code-size disaster.

`-O2` does not change this. Measured on the same shard:

| config | text bytes | `bl` count | `psx_cyc_step` calls |
|---|---|---|---|
| `-Os` (released) | 196 724 | 11 800 | 4 805 |
| `-O2` | 273 524 (+39 %) | 11 915 | 4 805 |

**Rejected**: `-O2` buys nothing on the per-instruction path and costs 39 % more text on a
machine whose problem is a 32 KB L1i / 512 KB L2 against 32 MB of generated code.

### 2.2 Suspect 2 — icache: real, and it is a *code-size* problem

The emitted code is **41 bytes per guest instruction** (196 724 B / 4 805). One guest ALU
instruction at `-Os`:

```asm
    2564: ldr   r1, [r4, #72]     ; gpr[18]
    2566: movw  r3, #0x6550       ; \
    256a: movt  r3, #0x86e0       ;  | &g_gte_native_provenance_active
    256e: ldr   r3, [r3, #0]      ; /
    2570: lsls  r2, r1, #28
    2572: str   r2, [r4, #104]    ; gpr[26]
    2574: cmp   r3, #0
    2576: bne.w <gte_native_provenance_cpu_alu>   ; never taken on Vita
    257a: movs  r1, #9
    257c: mov   r0, r4
    257e: bl    <psx_cyc_step>
```

14 of those 36 bytes are the provenance flag test. It is **dead on the Vita build**: the flag is
written only by `gte_native_provenance_set_enabled(g_native_render_selected)` (main.cpp:17354) and
`ram_provenance_set_cpu_tracking(g_native_render_selected)` (main.cpp:17362), and
`native_render_mode_resolve()` returns `GUEST_RENDER_RENDER_ORIGINAL` unconditionally under
`__vita__` (the Vita target is configured `XG_RENDER_NATIVE=OFF`). Removing it:

| | text bytes | calls |
|---|---|---|
| baseline `-Os` | 196 724 | 11 800 |
| `-DPSX_NO_NATIVE_PROVENANCE` | **145 048 (−26.3 %)** | 9 284 |

### 2.3 The actual dominant cost — the per-instruction timing model

`psx_cyc_step` is emitted once per guest instruction. At `-Os` it is a 26-byte stub that calls
three more out-of-line functions; `psx_cyc_base` (9 instrs), `psx_cyc_deps` (5–20, a
`ctz` loop over the GPR mask) and `psx_cyc_lds` (12, five stores to `CPUState`) — plus
`psx_cyc_charge` (~25–30 on the batched path) for the base cycle. `psx_cyc_lds` alone is:

```asm
       0: ldrb.w r3, [r0, #603]   ; ld_which_t
       4: ldr.w  r1, [r0, #604]   ; ld_absorb
       8: adds   r2, r0, r3
       a: strb.w r1, [r2, #568]   ; read_absorb[ld_which_t] = ld_absorb
       e: ldrb.w r2, [r0, #601]   ; read_absorb_which
      12: strb.w r3, [r0, #602]   ; read_fudge = ld_which_t
      16: and.w  r3, r3, #31
      1a: orrs   r3, r2
      1c: strb.w r3, [r0, #601]
      20: movs   r3, #32
      22: strb.w r3, [r0, #603]
      26: bx     lr
```

Per-guest-instruction budget (call frequency × body, from the shard-00 counts and the
`-Os` disassembly):

| emitted helper | freq / guest insn | body | host instrs / guest insn |
|---|---|---|---|
| `psx_cyc_step` (base+deps+lds) | 1.00 | 49 + 3 calls + 3 prologues | 55–65 |
| `psx_cyc_charge` (batched path) | 0.6–1.0 | 25–30 | 20–30 |
| `psx_icache_fetch` → `_miss` (hit path) | 0.38 | ~20 | 8 |
| `psx_check_interrupts_at` → `psx_check_interrupts` (**6 596 B**) | 0.12 | 50–100 | 6–12 |
| `psx_slice_block` / `psx_cyc_bb_defer_*` | 0.27 | ~10 | 3 |
| generated code proper (ALU + gpr + block glue) | 1.00 | ~16 | 16 |
| memory accessors + `cpu->write_word` (indirect) | 0.075 | 30–50 | 3 |
| **total** | | | **≈ 110–135** |

The measured cost is 500–1200 host cycles per guest cycle; at the timing model's ~1 guest cycle
per instruction that is 500–1200 cycles per guest instruction, i.e. IPC 0.1–0.25. The
instruction count and the memory footprint (32 MB text + 64 MB data vs 32 KB L1i / 512 KB L2) are
therefore both levers, and the changes below attack both. The counters in §5 will settle the
remaining split between the two.

### 2.4 Suspect 3 — TLB: not measured, no lever taken

`bss` is 64 034 928 B (61 MiB, mostly the 12 MB `g_dirty_ram_pc_table`, 8 MB `ram`, 8 MB
`mod_gpu_dma_memory` and the diagnostic rings). Nothing here is touched per instruction, so no
change was made; the 4 KB-page footprint of 96 MB of text+data against the A9's 512-entry main
TLB remains a candidate that only on-device counters can confirm.

### 2.5 Lever 4 — dispatch audit: nothing to win

`psx_dispatch_game_compiled()` is reached only on a guest *function* handoff (178 sites per
4 805 instructions = 3.7 %): one O(1) table lookup (`k_psx_game_dispatch_index[offset>>2]`),
a 64-byte identity memcmp, a range check, an interrupt check and the vsync-query HLE attempt.
Blocks themselves are `goto`-linked inside a shard, so there is no per-block dispatch to
optimize. Estimated ≤ 4 % of the budget; left alone.

## 3. Changes applied

Both are Vita-only and preprocessor-proven no-ops elsewhere.

1. **`PSX_NO_NATIVE_PROVENANCE`** (`pgxp_hooks.h`), defined on the Vita runtime target:
   `GTE_NATIVE_PROVENANCE_{LOAD,STORE,ALU,COP2}` and `CPU_RAM_PROVENANCE_STORE` expand to
   `((void)0)`. −26 % generated text, −1 load+branch per ALU/load/store, −1 call per store.
2. **`PSX_VITA_HOT_PATH`** (`psx_cyc.h`), same target: `psx_cyc_base`, `psx_cyc_deps` and
   `psx_cyc_lds` become `always_inline`, so the single shared copy of `psx_cyc_step` (and of the
   load/store helpers) is self-contained. `psx_cyc_step` itself stays out-of-line — one call per
   guest instruction instead of four. Measured cost: **+136 bytes** on shard 00.
3. **`XG_GENERATED_TU_OPT`** (CMake cache option, default empty): the generated-TU optimization
   level is now overridable without editing CMakeLists; empty keeps the desktop command lines
   exactly as they were, and Vita still defaults to `-Os`.
4. **Helper-call telemetry**: `psx_vita_perf.h` + four `__vita__`-guarded counters
   (`blocks`, `svc`, `irq`, `icache`) and a new `[xg-phase] calls ...` line next to the rate line.

## 4. Measurements

| artifact | before | after | delta |
|---|---|---|---|
| `slus_006.64_full_00.c.obj` text | 196 724 | 145 184 | −26.2 % |
| `field-overlay_01.c.obj` text | 179 780 | 115 452 | −35.8 % |
| `field-overlay_03.c.obj` text | 168 572 | 107 208 | −36.4 % |
| ELF `.text` | 38 880 320 | 31 461 298 | −19.1 % |
| SELF (`eboot.bin`) | 70 364 114 | 51 706 514 | −26.5 % |
| VPK | 28 626 678 | 21 331 009 | −25.5 % |
| `bss` | 64 034 928 | 64 034 928 | 0 (61 MiB < 96 MiB) |

Gates: `SELF ≤ 95 MB` ✓ (51.7 MB), `bss+data ≤ 96 MiB` ✓, zero `scePowerSet*` ✓ (unchanged code
path, and no new power API use), python suites 6 + 8 + 29 + 9 pass.

Desktop identity: the preprocessed source of the four modified runtime TUs
(`psx_cycles.c`, `interrupts.c`, `psx_icache.c`, `dirty_ram_interp.c`) and of
`slus_006.64_full_00.c` is **byte-identical** (path-normalized, code lines only) against the
pristine tree with the new defines absent; every `main.cpp` edit sits inside `#ifdef __vita__`;
`XG_GENERATED_TU_OPT` is empty by default, so no source-file property is set on desktop.

## 5. Prediction and what will confirm it

Expected effect on the dynamic instruction count: the provenance removal deletes ~2.3 host
instructions per guest instruction and the leaf inlining ~10–15 (three calls plus prologues), so
~110–135 → ~90–115, i.e. **−15 to −18 % instructions**, plus a 26 % smaller code footprint
(fewer L1i/L2 refills). Net predicted throughput: **1.15–1.35×**:

- early boot 0.99 MHz → **1.14–1.34 MHz** (3.4–4.0 % realtime)
- FMV 0.29 MHz → **0.33–0.39 MHz** (1.0–1.2 % realtime)

This is honest about the size of the win: **it does not close the 20–50× gap.**

### Measurement protocol for the next on-device run

1. Deploy the VPK, run at the operator's usual manual clock (500 MHz), let the intro play.
2. Pull `ux0:/data/xenogears-recomp/runtime.log`. The new line is printed next to each rate line:

   ```
   [xg-phase] calls blocks=+N/s svc=+N/s irq=+N/s icache=+N/s
   ```

3. Compute, for the same window as the rate line:

   ```
   guest_insns/s   = blocks/s × 5.61        (mean block size of the main EXE: 107 519 insns /
                                             19 174 blocks; overlays and BIOS shift this a few %)
   host_cycles/insn = (arm_MHz × 1e6) / guest_insns/s
   svc per frame    = svc/s   ÷ (guest_MHz × 1e6 / 564 480)
   irq per frame    = irq/s   ÷ (same)
   ```

4. Decision rules:
   - `host_cycles/insn ≳ 300` with `svc/s` ≪ guest_insns/s → memory-hierarchy bound: attack code
     footprint and hot/cold layout next (the 26 % we just removed is the first slice).
   - `host_cycles/insn ≈ 100–150` → instruction bound: the timing model dominates, and the
     only remaining lever is batching it per basic block (§6).
   - `dirty_interp` rising with `blocks/s` falling during the FMV → the FMV path is running in
     the dirty-RAM interpreter, and the AOT overlay coverage is the thing to fix.

## 6. What is left (needs an emitter change or an accuracy decision)

1. **Batch the timing model per basic block.** The emitter knows the block's instruction
   sequence, so the (base, deps, lds) sequence is a compile-time-known mask list. One call per
   block into a loop over a `.rodata` mask array replaces ~49 instructions per instruction with
   ~10, i.e. **−40 to −50 % of the whole budget** (predicted 1.8–2.2×). Cost: the emitter must
   emit the mask array and the block-level call, and the generated C changes for *all*
   platforms — the desktop non-regression rule means this needs a per-target codegen switch and
   a separate output directory for the Vita shards.
2. **Skip the PS1 icache model on Vita** (`psx_icache_fetch`, ~7 % of the budget). Pure timing
   refinement; guest-visible behaviour is unchanged except for cycle counts.
3. **`PSX_ENABLE_BLOCK_CYCLES` off** would remove the whole per-instruction model (~2.5–3×) and
   drop the runtime back to the legacy flat wait-state model. This is a product decision about
   timing fidelity, not a technical one — flagged, not taken.

## 7. Verification log

- Commits: submodule `vita-port` = `45199e11`; the parent `vita-thr-ds` commit that carries this
  document is the single commit on top of `4a95b63` (`git log -1` on branch `vita-thr-ds`).
  Both worktrees clean.
- Build: `bash build-vita.sh` (full mode, 6 jobs) → `build-vita/XenogearsRecomp.vpk`,
  sha256 `a5057ec131f20a357ae859232ec19d0ffaac2d99ec72829e350ae76fa3fb5641`,
  eboot.bin sha256 `4cb251301c0314a70f0dceaa29ec1d126105beaac6ed0a23952a0d2f96a56749`.
- Staged: `/home/fred/projects/xgr-tools/out/XenogearsRecomp-0.5-thr-ds.vpk` (21 331 009 B,
  byte-identical to the build output).
- Vita3K (known display-queue wedge; success criterion is the guest log):
  `bash /home/fred/projects/xgr-tools/bin/vita3k.sh XenogearsRecomp-0.5-thr-ds.vpk`, then
  `-r XGEN00001`; guest log shows

  ```
  [xg-phase] 1790144348.2xx bios handoff pc=0xBFC00000
  [xg-phase] 1790144348.277 first frame presented
  [xg-phase] rate arm=444 MHz frames=+120 (3.14 Hz) guest=+67737600 cyc
             (1.77 MHz, 5.2% realtime) dirty_interp=0.06 Minsn/s vblank_body=163.03 ms/frame
  ```

  i.e. the build boots, presents and advances the guest for 150 frames (the previous build's
  Vita3K log stopped before the first rate line, so this one gets further). Vita3K rates are not
  comparable to hardware (dynarmic JIT, ~52 % of wall time inside the emulated present) and the
  emulator wedges a few seconds later — the process is still alive and spinning, no guest crash
  is reported — which is why the `calls` line could not be captured there. It is read on
  hardware instead. No `scePowerSet*` anywhere (grep-proven).

