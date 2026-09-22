# Vita Phase 2 — Vita3K SIGSEGV loop root cause (2026-09-22)

## Symptom

On Vita3K (v0.2.1 4096-ca2daeae, aarch64/Asahi host, OpenGL backend) the guest
boots, renders the publisher splash, then ~30 s after `Game started` starts
printing `Unhandled SIGSEGV at pc 0x0000ffff…b388` (≈44/s) forever. The guest
keeps partially alive (memcards get written) and the window freezes on the last
presented frame. On real hardware (fw 3.65) the same build does **not** crash —
it just boots slowly — so the fault is an emulator/guest interaction, not a
module defect.

## Evidence (captured by ptrace at the first SIGSEGV delivery stop)

- Faulting PC: `libc __memcpy_generic`, instruction `stp q2,q3,[x3,#48]`
  (libc+0xab388).
- `si_addr = 0x460c04000`, `si_code = SEGV_ACCERR` (mapped but not writable).
- Registers: `x0=x3=x8=0x460c03fc0` (dst), `x1=0x48949da60` (src),
  `x2=0x470` (len), `x17=` memcpy entry, `x30` (LR) = Vita3K+0xC1A9E0.
- LR resolves to the `sceClibMemcpy` HLE bridge; the frame chain is
  `memcpy ← sceClibMemcpy bridge ← call_import ← ThreadState::run_loop`.
- Guest mapping: host = guest + `0x400000000`, so dst = guest `0x60c03fc0`,
  i.e. the last 0x40 bytes of a 12 MB writable CDRAM region. The next 1.39 MB
  (`0x460c04000–0x460d68000`) is `r--`, `Private_Dirty` — it was written and
  then `mprotect(PROT_READ)`-ed.
- The write is a row copy from SDL2's GXM texture upload path
  (`VITA_GXM_UpdateTexture` → `LockTexture` → row `memcpy`, dst advances by the
  texture pitch, len = `rect->w * bpp` ≈ 0x370–0x470).

## Mechanism

1. SDL2's Vita GXM renderer allocates a CDRAM texture pool and uploads textures
   with plain `memcpy` rows. VitaSDK's newlib routes `memcpy` to the
   `sceClibMemcpy` **import**, so the copy runs inside the emulator.
2. Vita3K's OpenGL texture cache protects guest texture pages read-only to track
   guest writes (`renderer::TextureCache::cache_and_bind_texture` →
   `MemState::add_protect(..., MemPerm::ReadOnly, callback)`).
3. Writes that hit a protected page are expected to fault **in JIT'd guest
   code**, where the emulator's fault path (`handle_access_violation`) calls the
   texture callback, unprotects the range and retries.
4. Here the write happens in **native HLE code**, so the emulator cannot
   attribute the fault to guest code: it logs `Unhandled SIGSEGV`, returns, the
   `stp` retries, faults again — an infinite loop. The guest thread never
   completes the upload, so the game never advances past the splash.
5. Why protection is active although `hashless-texture-cache: false` is set:
   `GLState::late_init` calls `texture_cache.init(true, …)` — the GL backend
   **hardcodes `true`** and ignores the config value (same code in v0.2.1 and
   master).

The texture's data begins at a non-page-aligned address (`0x60c03fc0`), so
`range_protect_begin = align(data_addr, host_page_size)` leaves the first 0x40
bytes writable while the following pages are protected; the first row upload
crosses that boundary and faults.

## Recommended fixes

- **Vita3K (preferred, one line):** pass the config value in
  `GLState::late_init` (`texture_cache.init(cfg.hashless_texture_cache, …)`)
  instead of hardcoded `true`. Alternatively make `handle_access_violation`
  reachable for faults raised in native HLE code (e.g. page-safe HLE
  `sceClibMemcpy`/`sceClibMemset`), or use the Vulkan backend.
- **Guest-side fallback (do not ship by default):** define our own
  `memcpy`/`memset` so copies execute as JIT'd guest code and the protection
  faults resolve. Rejected as a default because the firmware
  `sceClibMemcpy` is likely faster than a naive loop, and hardware boot time is
  already the bottleneck.

## Related observations

- The runtime's `signal(SIGSEGV, …)` install is HLE'd away on Vita (the host
  handler stays the emulator's), so `psx_last_run_report.json` is never
  produced for host faults. The install is now skipped on `__vita__` anyway.
- Guest stderr is invisible in Vita3K; the runtime now mirrors it to
  `ux0:/data/xenogears-recomp/runtime.log` and emits `[xg-phase]` boot
  timestamps for the hardware performance pass.
