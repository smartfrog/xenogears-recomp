# Vita performance pass 1 — measurements, fixes, hooks

Scope: PS Vita port of XenogearsRecomp. This pass fixes the boot-latency
regression found by hardware instrumentation and adds the hooks needed to
characterize the remaining (emulation-throughput) bottleneck.

## Hardware data this pass is based on

Instrumented run on a real Vita (fw 3.65), launch 22:03:00:

| marker | wall clock | delta |
| --- | --- | --- |
| launch | 22:03:00.000 | — |
| `module init start` | 22:03:08.286 | 8.3 s (firmware loader: 1,221,151 `.rel.text` relocations) |
| `overlay worker start` | 22:03:08.366 | 0.08 s |
| `sdl video init done` | 22:07:56.330 | 4 m 48 s (upper bound — console was suspended once mid-window) |
| publisher splash | ~22:13 | remainder |

## Root cause of the 4 m 48 s window: whole-disc SHA-256 on every boot

`main()` → `mod_runtime_commit()` → `open_verified_disc()` →
`sha256_open_disc()`, which walks **every raw 2352-byte sector** of the disc
image (~703 MB) to produce the mod-resolution disc digest. It ran on every
boot, whether or not any installed mod consumes the digest.

Measured on Vita3K (Apple M1 Pro host) with fine-grained `[xg-phase]` markers:
`disc resolved` → `mods committed` = **7.47 s** — the entire "SDL bring-up"
window (SDL init itself is 13 ms: video subsystem 7 ms, gamecontroller 6 ms).

Fix (`psxrecomp/runtime/src/mod_packages.cpp`, `mod_runtime.cpp`):
`ModPackageManager::requires_disc_digest()` returns true only when some
installed/bundled package gates on the digest (a target, patch, or
indexed-file `disc_sha256`). When false, the digest is skipped and mod
resolution runs with an empty digest — behaviour-identical, because
`target_matches()` treats an empty `disc_sha256` as "no gate".

After the fix (same Vita3K host): `disc resolved` → `mods committed` =
**16 ms**; `module init start` → `bios handoff` = **0.99 s** (was 9.0 s).
Final fresh build, confirmation run: `module init start` → `first frame
presented` = **0.37 s** (was 9.0 s to handoff before the fix).

Artifact: `build-vita/XenogearsRecomp.vpk`, staged for hardware as
`out/XenogearsRecomp-0.3-perf1.vpk`.

## Fixes and hooks delivered

- `psxrecomp/runtime/src/main.cpp`
  - Create `ux0:/data/xenogears-recomp` **before** the stderr/stdout redirect
    (first-run markers were lost), probe-open before `freopen` so a failure
    cannot leave stderr on a freed FILE slot (newlib returns the slot to the
    stdio pool on `freopen` failure), and redirect **stdout** as well — the
    guard/interpreter/cadence diagnostics all go to stdout.
  - Fine-grained boot markers: `overlay worker joined`, `disc resolved`,
    `mods committed`, `bios resolved`, `disc identity done`, `text guard done`,
    `memcards done`, `debug services done`, `sdl init start`,
    `sdl video subsystem done`, `sdl gamecontroller done`, `window created`,
    `renderer created`, `present texture created`.
  - Read-only clock telemetry: `clocks arm=… gpu=… bus=… gpu_xbar=… (MHz,
    read-only)` from `scePowerGet*`. **No `scePowerSet*` call anywhere.**
  - Periodic rate marker (log-spaced checkpoints at 30/150/630/2550/… frames):
    `rate arm=… MHz frames=+N (Hz) guest=+C cyc (MHz, % realtime)
    dirty_interp=M insn/s vblank_body=ms/frame`. Separates emulation throughput
    from present cost and detects interpreted (non-native) guest execution.
- `psxrecomp/runtime/src/mod_runtime.cpp`, `mod_packages.{h,cpp}`: the digest
  skip described above.

## Audits (no code change)

### Present path

- Backing texture: `SDL_PIXELFORMAT_ARGB8888`, `SDL_TEXTUREACCESS_STREAMING`,
  640×512 (× supersampling), created once at startup; no per-frame create,
  destroy, or lock.
- SDL's Vita "VITA gxm" renderer supports only ABGR8888 / ARGB8888 / RGB565 /
  BGR565 (+YUV) — there is no RGB555/ARGB1555 texture format, so the PS1
  15-bit VRAM cannot be uploaded 1:1. The staging buffer is filled by
  `gpu_display_pixel_argb()` per pixel (320×240 = 76.8 K calls/frame) and
  uploaded with `SDL_UpdateTexture` (memcpy into the GXM texture).
- `SDL_RenderSetLogicalSize` is set once; `SDL_RenderCopy` scales/letterboxes
  on the GPU; `SDL_SetTextureScaleMode` is applied only when the
  nearest/linear choice changes. `SDL_RenderPresent` is asynchronous
  (`sceGxmDisplayQueueAddEntry`, no `sceGxmFinish`).
- Cost estimate: the per-pixel 15-bit conversion is ~2–5 ms/frame at
  444–500 MHz — under 1 % of a 2 fps frame. A batch row converter (like the
  existing `gpu_depth24_present_row`) could cut it, but it is not the
  bottleneck; documented, not applied.

### CPU saturation / spin-waits

- Frame pacing (`frame_pacing.c`) sleeps with `nanosleep` in ≤1 ms chunks
  (newlib `nanosleep` → `sceKernelDelayThread`, a real block) and spins only
  the final sub-2 ms before the deadline (~3–6 % of one core).
- No unbounded busy-wait in the main loop or the vblank/present path.
- Background threads: SDL audio pull thread (blocks in `sceAudioOutOutput`),
  SDL joystick (no thread; `VITA_JoystickDetect` is empty), debug-server phase
  sampler (1 ms `nanosleep` loop). `freeze_heartbeat` is Windows-only.
- Conclusion: saturation is the emulation workload itself (guest code +
  software PS1 GPU rasterizer), not a runtime wait. The `rate` marker will
  quantify it on the next hardware run.

### Relocations

`readelf -r` on the linked ELF: `.rel.text` 1,221,151 entries —
`R_ARM_THM_CALL` 949,422 (77.7 %), `R_ARM_ABS32` 153,490,
`R_ARM_THM_MOVT_ABS` 55,512, `R_ARM_THM_MOVW_ABS_NC` 55,510,
`R_ARM_THM_JUMP24` 7,117, `R_ARM_REL32` 100. The PC-relative majority is
preserved by the toolchain's `-Wl,-q` and is idempotent at load time (GNU ld
resolves out-of-range calls through ARM veneers and `vita-elf-create`
compensates the addend so `S + A` re-resolves to the veneer). No visibility /
`-Bsymbolic` / `-fPIC` lever exists: the executable is static and non-PIE, so
there are no dynamic/PLT/GOT relocations to remove. Measured loader cost is
8.3 s total, so the remaining upside is a few seconds at most; a prototype
stripper (dropping types 10/30 from `.rel.text`, 1,221,151 → 264,612 entries,
eboot 70.4 → 58.9 MB) was validated locally but deliberately not wired into
the default build.

## Next steps (perf pass 2)

1. Read the next hardware log: the `rate` lines give guest Hz, % PS1 realtime,
   `dirty_interp` Minsn/s and vblank-body ms/frame. If `dirty_interp` tracks
   the guest rate, the game is running interpreted (text-image guard/`native`
   gating) and that is the next fix.
2. If the guest is native and still slow, profile the software PS1 GPU
   rasterizer and the per-instruction bookkeeping on device.
3. Suspend/resume: the GXM display queue is asynchronous with no timeout, and
   the present self-heal only triggers on presents that block >250 ms. The
   post-suspend "0 fps" report is consistent with a wedged display queue that
   the runtime never notices; add a frame-advance watchdog on Vita.
