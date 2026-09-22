# AGENTS.md

XenogearsRecomp: static recompilation of Xenogears (USA, Disc 1, SLUS-00664) to native code via the pinned `psxrecomp` submodule (MIPS → C → native). A PS Vita port is in progress — see "Vita port" below before touching anything `__vita__`/`build-vita.sh`.

## Non-negotiables

- **Never commit game-derived data.** `game/`, `generated/`, `overlays/`, `overlay_captures.json`, `*.mcd`, boxart — all gitignored; keep it that way. The release VPK/log/screenshots of the running game are also game-derived.
- **Identity binding**: the whole pipeline authenticates against the pristine US EXE (`game/slus_006.64`, SHA256 `dc0b2dd7…7119`). Patched/translated/undub discs (e.g. FR TRAD) will fail manifest and overlay identity. Do not weaken the checks to make a patched disc pass.
- **Desktop non-regression**: all Vita changes are guarded (`#ifdef __vita__` / `if(VITA)`) such that non-Vita builds are behaviorally (ideally byte-) identical. Every quality gate checks this.
- **No system-state tweaks in Vita code**: zero `scePowerSet*` (read-only `scePowerGet*` telemetry is fine). The user manages console clocks manually.

## Setup facts an agent will otherwise get wrong

- Build inputs are **gitignored user files**: `game/slus_006.64` + a disc image (`game/disc1.cue`/`.bin`/`.iso`, or `XG_DISC=/path ./build.sh`), and `psxrecomp/bios/SCPH1001.BIN` — `build.sh` hard-fails without the retail BIOS even though the runtime defaults to the bundled OpenBIOS. The Vita flow (`build-vita.sh`) needs neither SCPH1001 nor build.sh.
- Clone needs `--recurse-submodules` (psxrecomp + recomp-ui + nested `psxrecomp/lib/*`); a missing submodule surfaces as configure FATAL_ERRORs far from the cause.
- **`.gitignore` gotchas**: `tools/*` is ignored with an explicit whitelist — a NEW file under `tools/` stays untracked until you add a `!tools/<name>` line. `docs/*` is ignored except `docs/xenogears/` — put working notes elsewhere (the Vita port uses `vita/`).
- The recompiler emits `generated/` (~46 MB, 40 shards) from the EXE; CMake FATAL_ERRORs if missing. Overlays are extracted/authenticated from the disc **during the CMake build**, not by build.sh.

## Commands (Linux/macOS host)

```sh
./build.sh                      # full desktop build: recompiler → BIOS backends → game C → runtime (Release, Ninja)
./build.sh build-dbg Debug      # debug build w/ TCP debug server + ImGui overlay
# Manual game-C regen after changing game.toml/seeds/annotations (recompiler must be built first):
psxrecomp/recompiler/build/psxrecomp-game --config game.toml --source-observation-plan native_renderer/xg_render_resident_plan.txt
```

Windows: `build.ps1` (clang-cl recommended), `regen.ps1`. Full reference: `README.md` (accurate; trust it over assumptions).

## Tests

```sh
python3 -m pytest tools/test_overlay_annotations.py          # 6 tests
python3 -m pytest tools/test_generate_emulator_field_pacing_patch.py
(cd tools && python3 -m pytest test_finalize_static_overlays.py)   # 8 tests — must run FROM tools/
python3 -m pytest tests/test_perfect_works_to_psxmod.py      # needs no game data
```

CTest names (BUILD_TESTING=ON): `overlay_annotation_catalog`, `emulator_field_pacing_patch`, `finalize_static_overlays`. Most other C/C++ test targets need a configured runtime build; the python suites above are the fast gate used for every Vita-branch delivery.

## Vita port (in progress — read `VITA_PORT_STATUS.md` first)

- Current worktree: `../xgr-vita-perf` (branch `vita-perf-first-pass`, head `2f7da58`); desktop `master` does NOT contain the port yet. `build-vita.sh` + `vita/` only exist on the port branch.
- The submodule's port branch `vita-port` is **local-only**: pass it between worktrees with `git fetch <sibling-worktree>/psxrecomp vita-port:vita-port`.
- Host toolchain (this machine has no system g++): `CC/CXX` from `/home/fred/projects/xgr-tools/env/bin/aarch64-conda-linux-gnu-{gcc,g++}`; cmake+ninja in `~/.local/bin`; VitaSDK 2026.08 at `~/vitasdk`.
- Cross-build: `bash build-vita.sh` (~15–30 min fresh; AOT overlays recompile from the disc each time). Mode `--runtime-stub` links without game C.
- Hardware loop: VPK → FTP `192.168.2.106:1337` (anonymous) → `ux0:/downloads/`; disc at `ux0:/data/xenogears-recomp/disc1.{cue,bin}`; pull `ux0:/data/xenogears-recomp/runtime.log` after runs (`[xg-phase]` markers + `rate` lines are the perf instrument). FTP is unreachable while an app is foreground — that doubles as an alive-check.
- Vita3K (`/home/fred/projects/xgr-tools/bin/vita3k.sh`, relative VPK paths, run with `-r XGEN00001`) wedges at first present — diagnosed emulator bug (`docs/xenogears/runtime/vita-sigsegv-root-cause.md` on the port branch); don't chase it.

## OpenCode specifics

- `.opencode/orchestrator.md` = model routing for delegated tasks (complex/normal pools).
- Known harness bug: finished sub-agent replies sometimes never propagate — recover them from `~/.local/share/opencode/opencode.db` (table `session_message`, JSON `data` → `content[].text`) instead of re-running work.
- Delivery protocol: every implementation gets an independent review + mechanical checks (nm/size/grep/git); agent reports are never accepted as proof.
