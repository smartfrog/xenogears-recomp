# Research: Vita guest-throughput — MiMo report

*Provenance: A/B research trial (opencode/mimo-v2.6-flash-free), 2026-09-23, read-only.
Labels: `[measured]`/`[M]` = counted/read directly in the checkout; `[sourced]`/`[S]` = from cited URL; `[inferred]`/`[I]` = derived arithmetic. Recovered verbatim from the agent's final report.*

## Q1 — Framework audit: existing coarse/relaxed timing, knobs, emitter touch points, validation gates

### 1.1 The timing model's shape

- `runtime/src/psx_cycles.c:86-115` — legacy path "walked every device per charge", documented as having "pegged a core inside `psx_advance_cycles`" during a full prod build `[M]`; the modern path keeps an ordered event list. `PSX_DEADLINE_HARD_CAP = 16384` at `psx_cycles.c:110` `[M]`.
- `runtime/include/psx_cycles.h:82-157` — `psx_advance_cycles` is inlined into generated code, §1 + `GPR_DEPRES` + `DO_LDS` staging `[M]`.
- `runtime/include/psx_cyc.h:156-208` — `psx_cyc_base/deps/lds/step` hot path `[M]`.

### 1.2 Coarse/relaxed timing modes that ALREADY EXIST

**(a) `PSX_CODEGEN_CYCLE_PER_INSN=0` (default per-insn ON):**
- `recompiler/src/code_generator.cpp:38-49` — `codegen_cycle_per_insn()`, default `true`, env override `[M]`.
- `code_generator.cpp:255-272` — `emit_mid_block_cycle_charge` becomes a **no-op** when per-insn is off `[M]`.
- `code_generator.cpp:2031-2042` — per-block `block_exec_cycles` sum computed at codegen `[M]`.
- `code_generator.cpp:2066, 2073-2079` — `emit_pre_timing` → `psx_cyc_step` (loads skipped); `:2092-2099` `emit_pre_icache`; emission sites `:2272-2273, 2440-2442, 2568-2569` `[M]`.
- `recompiler/src/full_function_emitter.cpp:29-33` — `bios_cycle_per_insn()`; `:915-927` block-up-front charge; `:403-419` `block_cycles` map `[M]`.
- I.e. with `=0`, each basic-block leader emits one `psx_advance_cycles(block_cycles)` instead of per-insn charges **[I]**.

**(b) Known fidelity break of coarse mode (critical caveat):** `runtime/src/psx_cycles.c:651-664` — muldiv completion-stall **"REQUIRES per-instruction cycle charging … block-up-front charging breaks it"** `[M, verbatim comment]`. Same failure class for GTE stalls: `psx_gte_read/set` at `psx_cycles.c:718-776`, latency table `PSX_GTE_LAT_M1` + `psx_gte_cmd_latency` at `:778` `[M]`; `docs/source-gte-deferred-deadlines.md` documents a prior stale-clock double-count bug `[M]`. **The existing coarse mode is not a free ×2.**

**(c) Runtime batching already exists and is the template for the big lever:**
- `runtime/include/psx_cyc.h:47` — `PSX_CYC_BATCH_SOFT = 64` `[M]`.
- `psx_cyc.h:57-79` — `psx_cyc_bb_defer_begin/end` + `psx_cyc_local_begin` `[M]`.
- `psx_cyc.h:93-153` — `psx_cyc_charge`: defer → accumulate into `g_psx_cyc_batch`; local-acc path for `load_charge_batch_funcs` `[M]`.
- Emitted per generated function: `code_generator.cpp:3167-3183` (bb_defer guard + local-acc), `full_function_emitter.cpp:1683-1689` `[M]`.
- **IRQ edges force a flush**: `emit_interrupt_check` at `code_generator.cpp:274-290`, called at every branch exit `:2600-2632` `[M]`.
- Tests pinning this behavior: `recompiler/tests/test_fmv_cycle_batch_codegen.py` (asserts bb_defer guarded + IRQ-edge flush in generated C), `runtime/tests/test_psx_cyc_batch.c`, `test_gte_deferred.c`, `test_psx_cycle_event_boundaries.c`, `test_overlay_store_cycle_barrier.c` `[M]`.

**(d) Idle-skip detector (opt-in, OFF):** `runtime/src/psx_cycles.c:354-567`, gates `IDLE_STREAK_MIN=4`, `QUANTUM_MAX=32768`, `SKIP_MAX_CYCLES=1200000` at `:391-396` `[M]`; env `PSX_IDLE_SKIP` read at `:440`, surfaced via `main.cpp:14349-14357` `[M]`.

**(e) Interpreter tier has NO batch/defer mechanism** — `dirty_ram_interp.c` (4,266 lines) charges per-insn `psx_cyc_step` (`:123` via `interp_cyc_step`); block chaining via `g_dirty_interp_chain_target` (`:900, 3145, 3239, 3441, 3487, 3530, 3569`); `g_dirty_ram_insns_run++` at `:1523, 2834, 3385`; muldiv `:1920-1963` `[M]`.

### 1.3 Config knobs (exact keys and defaults)

| Knob | Where read | Default | In `game.toml`? |
|---|---|---|---|
| `[runtime] idle_skip` | `config_loader.cpp:517-518` | `false` (`config_loader.h:323`); documented `docs/config_schema.md:386` | **No** `[M]` |
| `[recompiler] load_charge_batch` | `config_loader.cpp:1520-1523` | `false` | No `[M]` |
| `[recompiler] load_charge_batch_funcs` | `config_loader.cpp:1524-1529`; **falls back to `hot_funcs`** (`:1531-1532`) | empty | No `[M]` |
| `[recompiler] hot_funcs` | `config_loader.cpp:1503-1517` → `__attribute__((hot))` at `code_generator.cpp:3101` | empty | No; **absent from `docs/config_schema.md`** `[M]` |
| `mod_function_entry_funcs` | `game.toml:28` = `["0x8004B54C"]` | — | Yes (only recomp key present) `[M]` |
| per-insn timing relaxation | **no TOML key** — gen-time env `PSX_CODEGEN_CYCLE_PER_INSN` only | per-insn ON | n/a `[M]` |

**Siblings:** no `game.toml` in any worktree sets `idle_skip` or timing keys `[M]`. ApeEscape/Tomba2/MegaManX6 are not present as checkouts — references only (`psxrecomp/docs/ecosystem-watch.md:354-357`, `FAITHFUL_TIMING_PLAN.md`) `[M]`. **Xenogears is the first title to face this.**

### 1.4 Env bisect gates (measurable levers, all already wired)

- `PSX_ICACHE=0` — `psx_icache.c:15-22` `[M]`
- `PSX_LOAD_DELAY=0` — `memory.c:2262-2285`; `FAITHFUL_TIMING_PLAN.md:3341` records **"hundreds of FPS"** on MotK with it `[M]` ⇒ strongest single evidence of timing-model headroom
- `PSX_MMIO_WAIT=0` — `memory.c:2201-2205` `[M]`
- `PSX_IDLE_SKIP=1` — `psx_cycles.c:440` `[M]`
- `PSX_PRECISE_SLICE=1` — parked **OFF**: `cpu_state.h:175-190`; `psx_slice_block` emitted at every leader regardless (`code_generator.cpp:2053-2065`) `[M]`
- `PSX_CPS` — default **ON**, opt-out `PSX_CPS=0`: `code_generator.cpp:203-205`, `full_function_emitter.cpp:160-170` `[M]`

### 1.5 Dispatch and chaining

- Dispatch lookup is **O(1) direct index-table**: `generated/slus_006.64_dispatch.c:18625-18631` — `k_psx_game_dispatch_index[offset >> 2]` on a 29-bit masked key `[M]`. (The emitter's "binary search" wording at `full_function_emitter.cpp:2138` is stale `[M]`.)
- `psx_dispatch_game_compiled` (`:18651-18667`) pays on **every** entry: identity gate (`game_identity.c:58`), dirty-range check, `psx_check_interrupts_dispatch_entry`, `psx_vsync_query_hle_try` (`load_accel.c:69`), then the call `[M]`.
- Every block leader registered as CPS continuation: `code_generator.cpp:3150-3162` `[M]`.
- Intra-function chaining already exists via `goto block_XXXX`: 1,829 occurrences in shard 00 `[M]`.
- Per-function entry hook `debug_server_log_call_entry` emitted unconditionally (`full_function_emitter.cpp:1691`) but compiles away under `PSX_NO_DEBUG_TOOLS` (`cpu_state.h:362-364`) `[M]`.

### 1.6 Codebase scale of the timing model

- `generated/` = 46 MB, 41 shards; `psx_cyc_step` occurrences = **107,519**; `psx_cyc_bb_defer_begin` = **1,830** `[M]`. ⇒ ratio ≈ **10 step-calls per bb_defer begin** `[I]`.
- Shard 00: 4,805 `psx_cyc_step`, 1,811 `psx_icache_fetch`, 46 muldiv, 115 GTE sites, 569 IRQ checks, 101 load calls `[M]`.

### 1.7 Validation gates any emitter change must pass

- `psxrecomp/docs/HOST_OPTIMIZATION_CONTRACT.md:33` — interleaved A/B, **≥5% retention rule**; `:38` — **byte-identical guest state**; `:41` — faithful path stays linked/selectable `[M]`.
- Rulers: `psxrecomp/tools/cycle_compare.py`, `psxrecomp/tools/cycle_testrom/` (note: under **psxrecomp/tools/**, not repo-root tools/) `[M]`.
- Beetle oracle costs: `FAITHFUL_TIMING_PLAN.md:3663-3668` — baseline 3, alu +1, load +5, load2 +11, div +38, mult +15 `[M]`; `psx_instr_cost.h` is the declared single source of truth `[M]`.
- PGO wired: `runtime/runtime.cmake:16-17, 1564-1575`; MotK evidence — 311 `.gcda`, median ~39→~51 `[M]`.

### 1.8 In-repo prior evidence for idle-skip

`docs/LOAD_TIME_ZERO.md:105-112` — Tomba live A/B: **4,599 skips / 765M guest cycles elided; warm main burst 2.19 s → 1.73 s (~21%), zero CD overwrites** `[M]`. Forced **off** under netplay (`docs/ROLLBACK_MOTK_HOOKUP.md:93, 451, 572`) `[M]`.

### 1.9 Exact emitter touch points for segment-folding, with the fence map

| Fence (must not be crossed by a folded segment) | Location |
|---|---|
| muldiv start/complete stall | emit `code_generator.cpp:797-828`; absorb `psx_cycles.c:651-664` |
| GTE command/read latency | `code_generator.cpp:1817-1821`; `psx_cycles.c:718-778` |
| Load-delay slots | `psx_cyc_load_*` → `memory.c:2272+` (gate `:2262-2285`) |
| IRQ edge / interrupt check | `code_generator.cpp:274-290` (must flush batch) |
| MMIO device sync | `memory.c` `psx_devices_mmio_sync` + `PSX_MMIO_WAIT` (`:2201-2205`) |
| icache miss accounting | `emit_pre_icache` `code_generator.cpp:2092-2099` / `full_function_emitter.cpp:779-786` |

Files: `recompiler/src/{code_generator,full_function_emitter,strict_translator}.cpp`, `runtime/include/{psx_cyc.h,psx_cycles.h,psx_instr_cost.h}`, `runtime/src/psx_cycles.c` `[I]`.
Contract-safe design: a **runtime-guarded fast path** (`if no pending intra-block event → charge segment sum; else per-insn fallback`) preserves byte-identical guest state and keeps the faithful path selectable per `HOST_OPTIMIZATION_CONTRACT.md:38,41` `[I]`.
Gates: `cycle_compare.py`, `cycle_testrom`, `test_fmv_cycle_batch_codegen.py`, `test_psx_cyc_batch.c`, `test_gte_deferred.c`, `test_psx_cycle_event_boundaries.c`, desktop A/B `[M list]`.

## Q2 — Vita / PCSX-ReARMed prior art `[all S]`

- libretro announcement — ari64 ARM dynarec "much faster than Lightrec"; Lightrec's **"Cycle Timing Check mode" is "significantly slower than 'Max performance'"**; desktop dynarec vs interpreter (Final Doom 621 vs 246 fps). https://www.libretro.com/index.php/pcsx-rearmed-now-has-dynarec-support-across-multiple-platforms/
- `pcsx_rearmed_psxclock`, range **30-100, default 57**, "**Lower value = less work**". https://docs.libretro.com/library/pcsx_rearmed/
- Lightrec README — IR optimization passes, lazy + threaded compilation, run-time profiling (policies static AOT already matches/exceeds). https://raw.githubusercontent.com/pcercuei/lightrec/master/README.md
- **Beetle PSX analogues**: `beetle_psx_dynarec_eventcycles` (128-1024, "higher = faster"); `beetle_psx_gte_overclock` = "1 cycle per instruction and additionally eliminates all memory access or cache fetch latency … **Currently unstable**"; `beetle_psx_cpu_dynarec` — "CPU timing is less accurate". https://docs.libretro.com/library/beetle_psx/ — every mature PS1 emulator ships *opt-in* "less accurate timing = faster" modes and keeps faithful timing the default.
- Vita platform facts (Copetti): quad A9 ≤500 MHz, 2-issue OOO, 8-11-stage pipeline, **L2 = 2 MB**, NEON 16×128-bit, Venezia DSP. https://copetti.org/writings/consoles/playstation-vita/
- Adrenaline PS1 = POPS on the PSP Allegrex (MIPS), Vita ARM side handles sound/FMW — a Sony precedent for splitting PS1 work across two processors. https://www.assemblergames.com/threads/33128/ (t=22362).

## Q3 — Static-recomp prior art `[S]`

- **N64Recomp** — pure literal translation, zero cycle counting; pacing downstream. https://raw.githubusercontent.com/N64Recomp/N64Recomp/main/README.md
- **N64ModernRuntime** — "VI timing" in a runtime layer over cycle-less recompiled code. https://github.com/N64Recomp/N64ModernRuntime (landing text only — pacing source not opened; mechanism unconfirmed).
- **Zelda64Recomp** — RT64 framerate-interpolation slider; "**Changing framerate has no effect on gameplay**" (render-rate decoupled from simulation). https://github.com/Zelda64Recomp/Zelda64Recomp
- Warning: those titles have no load-delay/muldiv/GTE interlock semantics to preserve, whereas PS1 does (§1.2b).

## Q4 — Cortex-A9 + SMP

`[S]` (7-cpu.com): L1I/D 32 KB 4-way, 32 B lines, BTAC 512×2, GHB 4K, return stack 8, **mispredict 11 cycles**, 9-stage, 2-issue renaming OOO. Vita L2 2 MB `[S, Copetti]`.

`[I]`: the timing model's scalar byte/word-array state streams with poor reuse — the 60-70 % share pays **memory-latency tax on top of instruction-count tax**, which is why segment folding beats micro-tuning. Dispatch/goto-heavy CFG makes the 11-cycle mispredict penalty real.
`[M]`: gpu_gl_renderer threads are `__vita__`-guarded OFF; **no affinity or `0x80000` usage anywhere in the runtime**; MDEC already NEON (`mdec.c:22-26`); present-path conversion not NEON but ~2-5 ms and not the bottleneck.
**Amdahl** `[I]`: ~99 % single-threaded native guest execution ⇒ offloading rasterizer/SPU/MDEC caps at ≈ **1.1-1.4×**.
`[S]` 4th core: gbatemp 569845 — games restricted to 3 of 4 cores; 4th reachable via `sceKernelChangeThreadCpuAffinityMask()` mask **`0x80000`**; user reports "doesn't really do a lot". https://gbatemp.net/threads/569845/

## Q5 — Ranked plan

| # | Lever | Multiplier × confidence / effort |
|---|---|---|
| 1 | **Emitter segment-folding of per-insn timing** (extend bb_defer from 1,830 sites to straight-line runs between fences) | **×1.8-2.2 × high / high-effort** — the only lever attacking the dominant term |
| 2 | **Enable idle-skip** (`[runtime] idle_skip=true` / `PSX_IDLE_SKIP=1`) | **×~1.05-1.2 × high / ≈zero** |
| 3 | **Kill the FMV interpreter share** — batch timing in `dirty_ram_interp.c` and/or extend AOT coverage | **×~1.15-1.4 on FMV × medium / medium** (31 % interpreted `[I]`) |
| 4 | **PGO + `hot_funcs`** | **×1.15-1.35 × high / low** |
| 5 | **Dispatch/CPS hot-path trimming** (hoist the 4 per-entry gates; CPS tail cost) | **×1.05-1.15 × medium / low-medium** |
| 6 | **Coarse mode `PSX_CODEGEN_CYCLE_PER_INSN=0`** — only with compensating fences | **×~1.5-1.8 if fenced × medium / medium** — unshippable alone; prefer #1 |
| 7 | **Fidelity-tradeoff env gates as a Vita "performance" preset** (`PSX_LOAD_DELAY=0`, `PSX_ICACHE=0`, `PSX_MMIO_WAIT=0`) | **potentially large × low-medium / ≈zero** — diagnostic; ship only as explicit non-default preset (Beetle precedent) |
| 8 | **SMP: 4th core (`0x80000`) for GPU/SPU/MDEC** | **×≤1.1-1.4 × low-medium / medium** (Amdahl) |
| 9 | **NEON** | **×~1.0-1.05 × low / low** — MDEC already NEON; timing model scalar |
| 10 | **Deadline-cap relaxation** (Beetle eventcycles analogue) | **×small × high / low** — model already O(events) |

**Recommended sequence:** #2 (today, zero risk) → #4 → #1 with #3 folded in → #5 → #7 only as a labeled fidelity preset. **#1+#2+#4+#3** plausibly reach ≥25 MHz `[I]`.

## Open questions

1. Does a Vita-only "performance preset" fall under `HOST_OPTIMIZATION_CONTRACT.md` exemptions, or must it ship as a separate non-faithful build? (Rule 6-7 suggests build-time flavor — decide before #7.)
2. Idle-skip correctness for Xenogears specifically — no run exists; what acceptance test (anchors, skip counts, FMV/desync)?
3. **Fence granularity**: measured distribution of straight-line distances between muldiv/GTE/load/IRQ fences in `generated/` — if median segment length is 2-3 instructions, #1's ×1.8-2.2 shrinks. **Count before implementing.**
4. PGO on Vita: does the flow support cross-instrumentation, or must profiles come from desktop (representativeness)?
5. 4th-core payoff: gbatemp says "doesn't really do a lot" — one instrumented experiment before code, given the Amdahl cap.
6. `hot_funcs` undocumented but parses — intended public API?

## Could not verify (explicit)

- ari64 `new_dynarec` internals: raw.githubusercontent 404s — cycle-counting mechanism unverified.
- psx-place CapUnlocker page: 403.
- N64ModernRuntime pacing source not opened.
- Reddit threads unfetchable (snippets only).
- DuckStation/PCSX-Redux static-recomp timing options: deliberately excluded.
- L2 1 MB (7-cpu reference) vs 2 MB (Copetti/Vita) — uncrossed.
- `tools/test_generate_emulator_field_pacing_patch.py`: referenced by root CMakeLists but not found under repo-root `tools/` — possible stale registration.
- `cycle_compare.py`/`cycle_testrom` live under `psxrecomp/tools/`, not repo-root — easy path mistake.
- Nothing was built or run; all `[M]` labels are static reading, not execution.
