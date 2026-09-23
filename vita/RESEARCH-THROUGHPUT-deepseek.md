# Research: Vita guest-throughput — DeepSeek report

*Provenance: A/B research trial (opencode-go/deepseek-v4.1-flash#max), 2026-09-23, read-only.
Labels: [M] measured, [S] sourced (URL/file:line), [I] inferred. Recovered verbatim from the agent's final report.*

## Q1 — Framework audit

### 1.1 What the per-instruction timing work actually is

Every guest instruction in the *generated* code is preceded by a call into the timing model, and every 4th by an icache call:

| emitted per guest insn | site | freq [M] | runtime body [M] |
|---|---|---|---|
| `psx_cyc_step(cpu, mask)` | `code_generator.cpp:2073-2080`, `full_function_emitter.cpp:758-764` | 1.00 | local copy in each shard: **42 Thumb-2 insns** |
| `psx_cyc_charge(1)` (from base) | `psx_cyc.h:93-153` | ~0.6-1.0 | **71 insns** local copy (typical batched path ~15-20) |
| `psx_icache_fetch(cpu, addr)` | `code_generator.cpp:2092-2098` | 0.38 | **259 insns** total; hit path ~25 (incl. a 6-insn 64-bit telemetry counter) |
| `psx_check_interrupts_at(cpu, pc)` + `psx_cyc_batch_flush()` | `code_generator.cpp:274-286` | 0.11 | wrapper 34 insns + call into `psx_check_interrupts` (**6 596 B**, `interrupts.c:15`) |
| `psx_slice_block(...)` | `code_generator.cpp:2059-2063` | 0.15 | 16 insns, parked off (`cpu_state.h:176-189`) |

`psx_cyc_step` = `psx_cyc_base` (read_absorb decrement or charge) + `psx_cyc_deps` (ctz over a gen-time GPR mask) + `psx_cyc_lds` (5 byte loads/stores into `CPUState`) — `psx_cyc.h:156-208`. Loads run the same sequence inlined inside `psx_cyc_load_word/half` (`psx_cyc.h:234-296`).

**[M] Budget check on the shipped A/B build** (`xgr-vita-thr-ds`, `PSX_VITA_HOT_PATH=1`, `PSX_NO_NATIVE_PROVENANCE=1`, `PSX_ENABLE_BLOCK_CYCLES=1`): shard 00 object = 51 242 Thumb-2 insns for 4 805 guest insns (**10.7 static host insns/guest insn**, 30.2 B/guest insn), 4 805 `psx_cyc_step` calls, 1 811 icache fetches, 545 IRQ checks, 720 slice calls. Across the 40 main-EXE shards: 107 519 guest insns, **12.0 % loads, 9.7 % stores, 41.6 % icache-fetch sites**. Dynamic total ≈ 10.7 (generated) + 50 (step) + 0.38×25 (icache) + 0.11×(34+~40) (IRQ) + 0.22×~30 (mem ops) ≈ **95-130 host insns/guest insn**.

### 1.2 Existing coarse/relaxed timing knobs (the framework already has several)

| knob | what it does | default | source |
|---|---|---|---|
| `PSX_CODEGEN_CYCLE_PER_INSN=0` | recompiler emits **one block-up-front `psx_advance_cycles(N)`** instead of per-instruction `psx_cyc_step` (plus `emit_mid_block_cycle_charge` for mid-block labels) | per-instruction (ON) | `code_generator.cpp:38-49,255-272,2104-2109`; same in BIOS emitter `full_function_emitter.cpp:29-34,923` |
| `[recompiler] load_charge_batch(_funcs)` | per-**function** stack-local charge accumulator; load charges skip deadline probes until IRQ/MMIO/exit | off | `code_generator.h:117-120`, `config_loader.cpp:1519-1532`, `code_generator.cpp:3177-3183` |
| `psx_cyc_bb_defer_begin/end` | per-**function** charge batching (always on with block cycles) | on | `psx_cyc.h:57-62`, prologue at `code_generator.cpp:3167-3184` |
| `PSX_ENABLE_BLOCK_CYCLES=0` | compiles the whole model out (no step/icache/slice calls; legacy flat wait-state path, `psx_cyc.h:259-263`) | on (set by `runtime.cmake:2043`) | `runtime.cmake:2038-2045` |
| `PSX_LOAD_DELAY=0` | runtime bisect gate: disables the load-delay interlock timing (no ReadAbsorb/fudge) | on | `memory.c:2263-2269` |
| `PSX_MMIO_WAIT=0` | disables device-region read waits | on | `memory.c:2201-2205` |
| `PSX_ICACHE=0` | icache model inert (call still emitted) | on | `psx_icache.c:15-22` |
| `[runtime] idle_skip=true` / `PSX_IDLE_SKIP=1` | fast-forwards provable poll loops to the next device deadline | off for Xenogears | `psx_cycles.c:354-412`, `config_schema.md:386`, `LOAD_TIME_ZERO.md:104-112` |
| `PSX_PRECISE_SLICE=1` | per-block IRQ slicing (parked) | off | `cpu_state.h:176-189` |
| `PSX_PGO=generate/use`, `hot_funcs`, `XG_GENERATED_TU_OPT` | PGO, `__attribute__((hot))`, per-target opt level for generated TUs | — | `runtime.cmake:16,1564+`, `CMakeLists.txt:506-512` |

**Answer: yes, two coarse modes exist** — `PSX_CODEGEN_CYCLE_PER_INSN=0` (block-up-front, documented as unable to absorb mult/div/GTE stalls, `code_generator.cpp:39-45`) and `PSX_ENABLE_BLOCK_CYCLES=0` (whole model out). Neither is wired to a *game config* key; both are env/CMake. `load_charge_batch` is the only per-function emitter knob and is undocumented in `config_schema.md`.

### 1.3 What an emitter batching change would touch

* **Game emitter**: `code_generator.cpp` — `translate_basic_block()` (1969-2130: slice preamble, block charge, `emit_pre_timing`/`emit_pre_icache` lambdas, call sites at 2272-2273, 2439-2441, 2568-2569), `emit_interrupt_check()` (274-286), `emit_mid_block_cycle_charge()` (255-272), VLC prologue (3167-3184).
* **BIOS emitter**: `full_function_emitter.cpp` — `emit_insn_interlock` (758-764), `emit_icache_fetch` (779-784), `emit_irq_check` (174-183), block charge (923).
* **Runtime model**: `psx_cyc.h` (base/deps/lds/step, load fast paths), `psx_cycles.h` (batch/local publish), `psx_instr_cost.h` (`psx_instr_base_cycles` = identity 1 cycle, `psx_cyc_dep_res_mask`), `psx_icache.{h,c}`, `cpu_state.h` (`psx_slice_block`).
* **Config plumbing**: `CodeGenConfig::load_charge_batch_funcs` (`code_generator.h:117`), parser at `config_loader.cpp:1519-1532`; the overlay pipeline already injects codegen modes through the recompiler subprocess env (`PSX_CPS=1` at `psxrecomp/tools/compile_overlays.py:4233,6685,7399`).

Two viable designs, both compatible with the framework's own stated target ("Recompiler emits exact accumulated cycle charges (collapses to a constant per pure-compute block)" — `FAITHFUL_TIMING_PLAN.md:55-74`):
* **D1 (safe, exact-by-construction)**: per block, emit a `.rodata` array of per-instruction dep/res masks + load flags; one call per block into a runtime loop that replays base/deps/lds in the same order. ~15-20 insns/guest insn instead of ~74 → total ~45-60 (≈2.0-2.4x).
* **D2 (aggressive)**: gen-time simulation of the pipeline state machine over the finite set of reachable entry states, emitting a per-block `(charge, next_class)` table (~5-10 insns/guest insn) → ≈2.5-3x. Higher divergence risk; verify with the existing PSX_COSIM cycle audit (`psxrecomp/docs/internal/PRECISE_IRQ_SLICE.md`, `cycle_compare.py`).

### 1.4 Desktop-regression risk and containment

`generated/*.c` is **shared** between desktop and Vita (`CMakeLists.txt:497-530`); overlays are regenerated during the CMake build from the disc, main-EXE C is generated manually. So an emitter change that alters emitted text must be **default-off** (new env/config key), with a **separate output dir** for the Vita shards. Overlays already have the env-injection seam. Gate: byte-identical desktop regen with the key unset; `PSX_COSIM` diff + `psx_cyc_batch_test`/`psx_icache_*` tests; the 6+8+29+9 python suites.

### 1.5 Sibling titles' timing configuration

Tomba 2 is the canonical idle-skip case (`psx_cycles.c:356`; live A/B 765 M guest cycles skipped, 2.19 s → 1.73 s, `LOAD_TIME_ZERO.md:104-112`); MotK uses `idle_skip=true`, `hot_funcs` and the VLC `load_charge_batch` path (`FAITHFUL_TIMING_PLAN.md:2919-2942,3319-3337`); Tomba 1/MMX6/Ape Escape are the regen smoke set (`FAITHFUL_TIMING_PLAN.md:96-99`). **Xenogears enables none of these** — no `idle_skip`, no `load_charge_batch`, no `hot_funcs` in `game.toml` [M].

### 1.6 Two things that contradict the then-current diagnosis

1. **The software rasterizer runs synchronously inside guest execution.** `gpu_write_gp0` → `gpu_write_gp0_body` (`gpu.c:9526`) → `gp0_exec_*` (4449+) → `gr_draw_*` (`gpu.c:4466,4696,4778`) → `gpu_sw_renderer.c` `sw_draw_*`. The GP0 "ring" is a diagnostic recorder only (`gpu.c:6030-6087`), not a deferred queue. Therefore its cost is **invisible to `vblank_body`**, and the earlier inference "rasterizer is NOT the bottleneck" is **unsound**. Xenogears is a fully-3D title.
2. **Autocompile is an inert stub on Vita** (`autocompile.c:2-45`), so any overlay code not in the AOT set runs in the dirty-RAM interpreter forever. The FMV window shows `dirty_interp=0.09 Minsn/s` = **31 % of guest instructions interpreted** [M, log line 86].

## Q2 — Vita prior art (web)

* **PCSX-ReARMed is the proof of feasibility**: "run PS1 games at full speed on the Vita. Tested over 50 games" (r/VitaPiracy 1vo4dif — body fetch 403, only the snippet is verifiable).
* **Design** (Ari64 dynarec, faster than Lightrec on ARM32 — [libretro blog, Feb 2020](https://www.libretro.com/index.php/pcsx-rearmed-now-has-dynarec-support-across-multiple-platforms/)):
  * guest cycle count lives in a **host register** (`HOST_CCREG`); per-instruction costs are **compile-time constants accumulated between branches** (`cinfo[i].ccadj`, `new_dynarec.c:8329-8342`), added once per branch with a timeslice compare (`:5449-5468,5831`).
  * **guest GPRs are register-mapped** with dirty-register write-back at branches (`wb_dirtys`, `branch_regs[i].regmap`), not memory-resident.
  * idle-loop fast-forward (`:5439-5447`); vsync polling hack (`:5853`).
  * **global clock multiplier**: `Config.cycle_multiplier = 10000 / psx_clock` (`frontend/menu.c:319`), core option **"PSX cpu clock (30-100, default 57)"** → multiplier 1.75, `CLOCK_ADJUST(x)=x*175/100`. I.e. the reference ARM PS1 emulator **charges 1.75 guest cycles per instruction by default** — the effective guest CPU is underclocked to buy host headroom. [S] ([docs.libretro.com](https://docs.libretro.com/library/pcsx_rearmed/))
  * "Threaded Rendering (disabled|sync|async): runs GPU commands in a thread"; the ARM32 build uses the **NEON GPU renderer**. [S]
* No Xenogears-on-Vita PCSX-ReARMed report found. No published host-instructions-per-guest-instruction numbers for Ari64 (looked; unverified).
* No dedicated vitaGL/vita2d 2D-console-offload write-up worth citing.

## Q3 — Static-recompilation prior art (web)

* **N64Recomp** (`github.com/N64Recomp/N64Recomp`): "Instructions are processed one-by-one… **This translation is very literal**" — **no cycle accounting in the recompiled code at all**; only Status among COP0 registers is handled (`src/recompilation.cpp:417-441`).
* **N64ModernRuntime** (`ultramodern/src/timer.cpp`): the CPU counter is **derived from the host wall clock** — `counter_per_ms = 46'875`, `time_now()` = `high_resolution_clock` delta, dedicated OSTimer thread; VI thread drives frame timing; there is even a `speed_multiplier` constant. A shipping static-recomp project virtualizes time from the host clock instead of counting per instruction. [S]
* No published per-instruction host-cost numbers for static recompilation found. [unverified]
* The only ARM PS1 numbers sourceable: libretro interpreter-vs-dynarec table (desktop x86: 190-250 vs 279-642 fps ≈ 2.5x) — not transferable to A9.

## Q4 — Cortex-A9 microarchitecture (applicable facts)

| property | value | source |
|---|---|---|
| L1I | 32 KB, 4-way, VIVT, 64-bit fetch; **32-byte line** | [7-cpu](https://www.7-cpu.com/cpu/Cortex-A9.html), [Zynq TRM](https://docs.amd.com/r/en-US/ug585-zynq-7000-SoC-TRM/Level-1-Caches) |
| L1D | 32 KB, 4-way, 32-byte line, 4-entry merging store buffer, 2 linefill buffers | 7-cpu |
| L2 | **2 MB shared** (PL310/Primelink) per Copetti; the 512 KB figure is Zynq's — **single-sourced, needs confirmation** | [copetti.org](https://copetti.org/writings/consoles/playstation-vita/) |
| branch prediction | BTAC 512 entries 2-way, GHB 4 K entries 2-bit; **mispredict = 11 cycles** | 7-cpu |
| latency | L1 4 cycles, L2 ~23, RAM 37 + ~110 ns; 2×ALU, 2-issue, OOO via renaming, 56-entry PRF | 7-cpu |
| clocks | 444 MHz default, user-managed 500 MHz [M, log] | log |
| NEON | 16×128-bit regs but **64-bit datapath** (128-bit ops take 2 cycles) → useless for scalar guest emulation, useful for bulk data (rasterizer, MDEC IDCT, present conversion) | [SO](https://stackoverflow.com/questions/7269946/) [I] |
| SMP | 4 cores; the guest is a single thread → **only host-side device work is parallelizable**; measured `vblank_body` bounds the *present* path, **not** the rasterizer | [M]+[I] |

**Hard arithmetic that governs everything**: realtime = 33.8688 M guest cycles/s at 500 MHz host = **14.8 host cycles per guest instruction** (at the current 1 cycle/instruction). The measured 0.99 MHz = **505 host cycles/guest instruction** [M: 500e6/0.99e6]. Required improvement is 34x; at PCSX-ReARMed's default 1.75 cycles/instruction the budget would be ~26 host cycles/instruction (17x to find). Any design spending >15 host cycles/guest instruction **cannot** reach 100 % realtime. With ~95-130 dynamic host instructions/guest instruction [M], reaching realtime would require an impossible IPC ≈ 7-9 on a 2-issue core — **the instruction count must come down 5-8x, not 2x.**

## Q5 — Synthesis: ranked paths

**Step 0 (do this first, ~1-2 days): attribute the 505 cycles.** The gap between ~100 dynamic instructions and 505 cycles (IPC ≈ 0.2) is unexplained and it decides which lever wins. Add Vita-only per-frame host-time accumulators around (a) the trampoline/generated-code loop, (b) `gr_draw_*`/`gp0_exec_*`, (c) `mdec_*`, (d) present; print beside `vblank_body`; add `g_dirty_window_dispatches`/`g_dirty_ram_insns_run` per scene (already exist: `dirty_ram_interp.c:3112,3387`); keep the `[xg-phase] calls` line. **Footprint-bound evidence**: removing 35 % of emitted text (dead provenance) cut dynamic instructions by only ~2 % yet measured ~1.15-1.35x — that gain can only come from fewer I-fetch stalls. Code footprint is the highest-confidence lever.

| # | lever | expected ×|confidence|effort |
|---|---|---|---|
| 1 | **Per-block timing batching (D1 then D2)** | 2.0-2.4x (D1) / 2.5-3x (D2), conf. med-high, effort **high** |
| 2 | **`PSX_ENABLE_BLOCK_CYCLES=0` on the Vita target only** | 2.5-3x [I], conf. low-med, effort **very low** — run as the *upper-bound measurement* first |
| 3 | **Rasterizer: measure, then thread it (SMP) / NEON it** | unknown, possibly 1.5-4x if it is the missing 390 cycles, conf. low-med, effort med |
| 4 | **Fix interpreted overlay coverage for the FMV** | up to ~3x on the FMV window, conf. low-med, effort low-med |
| 5 | **Guest-clock multiplier (relaxation knob)** | 1.2-1.75x for slack-rich scenes, conf. med, effort **very low** (PCSX-ReARMed default 57 % clock precedent) |
| 6 | **Footprint/locality: PGO (`PSX_PGO`), `hot_funcs`, selective -O2** | 1.1-1.3x, conf. med, effort low-med |
| 7 | **idle_skip=true A/B** (config-only) | 1.0-1.3x on wait-loop-heavy scenes, conf. low, effort trivial |
| 8 | **Block chaining** | ≤1.1x — **already done** (goto-linked; handoff ≤4 % of cost) |
| 9 | **NEON for guest CPU execution** | ~1.0x — skip |
| 10 | **Guest GPRs in host registers** | 1.2-1.6x, conf. low, effort **very high** |

**Honest bottom line.** Levers 1+2+5+6 compound to roughly 5-10x (≈5-10 MHz); levers 3+4 are the only ones that can plausibly close the FMV gap, and only if the profile confirms the rasterizer/MDEC hypothesis. **Nothing in the incremental list reaches 25-34 MHz.** The structural gap to a dynarec: (i) per-instruction timing state machine vs compile-time constants per block with the counter in a register, (ii) guest state in memory vs host registers, (iii) 32 MB of emitted code vs a small runtime-generated hot set. Reaching ≥25 MHz on a 500 MHz A9 realistically means adopting those three properties — an emitter rewrite, not a tuning pass. The alternative that *is* known to work on this exact hardware is PCSX-ReARMed's configuration: dynarec + threaded NEON GPU + 57 % guest clock.

## Could not verify

* Where the 505 cycles/instruction go (no device profiling access) — candidates: I-fetch/L2-DRAM stalls, the synchronous software rasterizer, MMIO/device sync, interpreter share.
* The FMV 0.29 MHz root cause (31 % interpreted cannot alone explain 1 960 ms/frame; would need ~4 000 cycles/interpreted-insn).
* Vita L2 size (2 MB Copetti vs 512 KB Zynq figure) — single-sourced.
* PCSX-ReARMed-on-Vita primary measurements (Reddit 403; only snippet + libretro docs). No Xenogears-specific Vita report.
* Whether the A/B ×1.15-1.35 figure was measured on hardware or predicted (no hardware log with the `calls` line existed).
* The exact `read_absorb[32]`/`read_fudge` corner-case semantics for a D2-style gen-time simulation — full state-machine enumeration needed; PSX_COSIM is the only proof path.
