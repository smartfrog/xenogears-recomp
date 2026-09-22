# XenogearsRecomp — Portage PS Vita : état et reprise

*Dernière mise à jour : 23 sept. 2026, ~00h45 (session du 22-23 sept). Orchestrateur : agent build-orchestrator (opencode).*

## Objectif

Porter XenogearsRecomp (recompilation statique PS1 → natif, repo fork `smartfrog/xenogears-recomp`, upstream `OpokXeno/xenogears-recomp`) sur PS Vita. Stratégie validée avec l'utilisateur : « simple d'abord » — renderer SDL software à résolution native PS1, puis compliqué ensuite.

## État en une phrase

**Le jeu tourne sur Vita hardware (fw 3.65) sans crash** — splash + FMV intro rendus — mais à ~0,3-1 MHz de vitesse guest PS1 (0,8-2,9 % du temps réel, soit ~1-2 fps). Le boot est passé de 288 s à 3,8 s. Le prochain chantier est la vitesse d'exécution du code natif.

## Chaîne validée (toutes gate PASS)

Branche : `vita-perf-first-pass` dans le worktree `/home/fred/projects/xgr-vita-perf` (tête = `2f7da58`).
Submodule `psxrecomp` : branche `vita-port` dans le même worktree (tête = `454be008`).

| Tâche | Parent | Submodule | Contenu |
|---|---|---|---|
| vita-runtime-prep | `0b18d19` | `884e143f` | build-vita.sh, mode --runtime-stub, adaptations `__vita__` du runtime, fibers ARMv7 |
| vita-full-build | `ed6ccdd` | (=) | build complet avec vrai jeu, gates `if(NOT VITA)` xg_*, `-Os` sur les 279 TUs générées (SELF 71 Mo < 95 Mo) |
| vita-bss-diet | `17ab025` | `3e0c41cb` | bss 798 Mo → 61,6 Mo (anneaux diagnostiques réduits, histogramme absent sur Vita, linker script sce-data-reserve) |
| vita-boot-prep | `d28d09d` | `04798f74`+`57f784a4` | fix stack (chunks 64 Ko + sceUserMainThreadStackSize 4 Mo), layout app0:/ux0:, VPK avec game.toml, fix host_path device paths |
| vita-sigsegv-hunt | `ccfb432` | `753f48b9`+`a3c7fd36` | root-cause SIGSEGV Vita3K (bug émulateur : texture cache mprotect + HLE memcpy — doc `docs/xenogears/runtime/vita-sigsegv-root-cause.md`), runtime.log guest, marqueurs [xg-phase] |
| vita-perf-first-pass | `2f7da58` | `3e197eb5`+`454be008` | **fix hash disque entier au boot** (288 s → 3,8 s hardware), télémétrie rate, redirection stdout+stderr, fix pacing (adoption MiMo) |

**IMPORTANT** : la branche `vita-port` du submodule n'existe que LOCALEMENT (jamais poussée). Les worktrees se la passent par `git fetch <worktree-voisin>/psxrecomp vita-port:vita-port`. À terme : forker OpokXeno/psxrecomp sur GitHub pour l'héberger.

## Fichiers du jeu (identité vérifiée)

- `game/slus_006.64` — SHA256 `dc0b2dd786203d4cce5927c5a3fc85a18f39a3f7406078860076ebb0bbae7119` (US authentique)
- `game/disc1.bin/.cue` — piste unique MODE2/2352, 305 586 secteurs (converti depuis MDF Alcohol 2448 o/secteurs)
- Sur la console : `ux0:/data/xenogears-recomp/disc1.{cue,bin}` (déjà en place)

## Environnement (tout vérifié fonctionnel)

- **VitaSDK 2026.08** : `$HOME/vitasdk` (natif aarch64), SDL2+libvita2d+zlib via vdpm, toolchain `$HOME/vitasdk/share/vita.toolchain.cmake`
- **cmake 4.4.3 + ninja** : `$HOME/.local/bin` (via pip)
- **Compilateurs C++ hôtes** (le système n'a pas de g++) : `/home/fred/projects/xgr-tools/env/bin/aarch64-conda-linux-gnu-{gcc,g++}` (conda gcc 16.2)
- **Vita3K isolé** : `bash /home/fred/projects/xgr-tools/bin/vita3k.sh` (ux0 virtuel sous `xgr-tools/vita3k/data/Vita3K/Vita3K/ux0`, disque déjà stagé). **Il wedge au splash** (bug émulateur diagnostiqué, fix une-ligne connu : `texture_cache.init(cfg.hashless_texture_cache, …)` — nécessite un build depuis sources, tâche `vita3k-patch` en option)
- **VPK livrés** : `xgr-tools/out/` — le dernier est `XenogearsRecomp-0.4-perf1-adopted.vpk` (= `2f7da58`), déployé sur la console comme `ux0:/downloads/XenogearsRecomp-0.4-perf1.vpk`
- **Build** : host recompiler d'abord (commande dans les prompts passés / cf. worktrees), puis `bash build-vita.sh` (~15-30 min). Identités SHA à passer : `dc0b2dd…7119` (les deux)
- **Modèles (`.opencode/orchestrator.md`)** : `complex: opencode/mimo-v2.6-flash-free`, `normal: opencode-go/deepseek-v4.1-flash#max`. **A/B réalisé** : DeepSeek a gagné (confiance haute) sur implémentation complexe — le router sur ces tâches. MiMo : bon pour investigation courte (expérience témoin exemplaire) mais télémétrie incomplète et livraisons de rapport peu fiables.

## Vita hardware

- fw 3.65, VitaShell FTP `192.168.2.106:1337` anonymous, upload VPK → `ux0:/downloads/`. **FTP down pendant qu'une app tourne** (= test de vie de l'app)
- L'utilisateur gère les horloges **à la main** (boost 500 MHz). **POLITIQUE DURE : zéro `scePowerSet*` dans le code** (télémétrie Get* autorisée)
- L'app survit au cycle veille/réveil, mais le rendu peut se figer après (wedge GQM display queue sans timeout — SDL2 `VITA_GXM_RenderPresent` → `sceGxmDisplayQueueAddEntry` ; self-heal inopérant car present asynchrone — fix candidat pour une tâche future)
- Logs à récupérer après run : `ux0:/data/xenogears-recomp/runtime.log` (+ `psx_last_run_report.json` en cas de crash, + `.psp2dmp` système dans `ux0:/data/`)

## Données de perf hardware (le cœur du dossier)

Run 0.4 (relances 23:53 et 23:58, boost manuel 500 MHz, ~30 min, FMV intro) :

```
rate #1 : frames=+120 (1.75 Hz) guest=0.99 MHz (2.9% realtime) dirty_interp=0.03 Minsn/s vblank_body=1.65 ms
rate #3 : frames=+480 (0.51 Hz) guest=0.29 MHz (0.8% realtime) dirty_interp=0.09 Minsn/s vblank_body=9.11 ms
```

**Diagnostic établi** :
- `dirty_interp ≈ 0` → le jeu NE tourne PAS dans l'interpréteur (gating AOT OK — hypothèse morte)
- `vblank_body` négligeable → le rasterizer software n'est PAS le goulot
- → **~99 % du temps = exécution du code natif lui-même, à ~500-1700 cycles hôte/insn guest** (20-50× trop lent pour un A9)

**Suspects prioritaires** :
1. `-Os` a tué l'inlining des accesseurs mémoire dans les TUs générées (chaque load/store guest = call non inliné, chaînes de dépendances) — vérifier par objdump
2. Icache thrashing (40 Mo de code / L1 32 Ko, dispatch indirect par bloc)
3. TLB

**Plan de la tâche suivante `vita-guest-throughput`** (complex → DeepSeek) :
1. objdump des chemins chauds du binaire Vita : les accesseurs mémoire sont-ils inlinés dans les shards ?
2. Mix ciblé `-Os`/`-O2` sur les shards chauds (le `-O3` complet = 140 Mo > limite ~95 Mo ; `-O2` complet à mesurer) et/ou `always_inline` sur les accesseurs
3. **PGO du framework** (`PSX_PGO=generate/use` déjà câblé dans runtime.cmake) : profil généré sur desktop, appliqué à la compile Vita → localité + inlining au chaud
4. Itération avec mesure par lignes `rate` sur console (build → FTP → run → log)

## Protocoles établis (à conserver)

- **Quality gate** à chaque livraison : review indépendante (sub-agent code-reviewer) + contrôles mécaniques directs (nm/size/grep/git) — le rapport d'un agent ne vaut jamais preuve
- **A/B modèles** : worktrees jumeaux, même brief, puis **UNE review comparative unique** (juge anonymisant A/B), adoption hybride possible (le gagnant intègre les morceaux validés du perdant, puis re-gate)
- Max 2 rounds de rework par tâche ; brancher les worktrees depuis le commit validé du prédécesseur ; submodule `vita-port` transmis par fetch local inter-worktrees
- **Bug opencode connu** : les réponses des sub-agents ne remontent pas toujours — les récupérer via la base SQLite `~/.local/share/opencode/opencode.db` (table `session_message`, data JSON → content[].text)
- L'utilisateur veut : pas de tweak système en code, tests sur hardware quand c'est nécessaire (pas d'itération à la main), itérer sur émulateur quand il est réparé

## En souffrance (backlog)

1. **Nettoyage** : worktree `xgr-vita-perf-mimo` (perdant de l'A/B) à supprimer ; branches `vita-perf-first-pass-mimo` supprimables
2. **Advisories review** (pour la passe de simplification finale) : FATAL_ERROR `VITA+BUILD_TESTING`, digest dans le message build-vita.sh, replay sémantique Vita, stub sans module params, psx_window_icon re-anchoring, caches morts app0:, crash_trace duplication chemin, marker rate après reset compteur, test manquant pour `requires_disc_digest()`, message commit submodule lacunaire
3. **Fusion finale** : toute la chaîne `vita-perf-first-pass` → `master` + submodule vers un fork GitHub + passe code-simplifier + nettoyage worktrees — quand la campagne perf sera aboutie
4. **`vita3k-patch`** (optionnel mais recommandé pour itérer vite) : builder Vita3K depuis sources avec le fix texture-cache
5. Fix suspend/resume (display queue timeout) — candidat tâche dédiée
6. Le « 0 fps post-veille » : probablement le point 5 ; non reproduit proprement

## Récap express pour reprendre

1. Worktree de travail : `/home/fred/projects/xgr-vita-perf` (propre, tout est commité)
2. Prochaine tâche : `vita-guest-throughput` (brief esquissé ci-dessus, à affiner avec les données objdump)
3. Mesure hardware : pousser VPK par FTP → run 10-15 min → tirer `runtime.log` → lire les lignes `rate`
4. Seuil à battre : guest 0,99 MHz → 33,87 MHz (100 % temps réel). Chaque gain se lit directement dans les lignes rate.
