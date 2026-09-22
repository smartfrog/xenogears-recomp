/* Vita Phase 0 stub for the Xenogears native-renderer (native_renderer/)
 * functions the psxrecomp runtime references directly. On Vita the native
 * renderer is disabled (XG_RENDER_NATIVE=OFF semantics): every hook declines,
 * every configure/start call fails, and the runtime stays on the original
 * PS1 GPU software path. ZERO game-derived content. */
#include "xg_render_auth_runtime_hooks.h"
#include "xg_render_auth_runtime_invalidation.h"
#include "xg_render_auth_runtime_control.h"
#include "xg_render_auth_runtime_diagnostics.h"
#include "xg_render_motion.h"
#include "xg_render_movie_publisher.h"
#include "xg_render_native_target.h"
#include "xg_render_native_work.h"
#include "xg_render_presentation_host.h"
#include "xg_render_resource_repository.h"
#include "xg_render_runtime_host_services.h"
#include "xg_render_semantic_compositor.h"
#include "xg_render_semantic_presentation.h"
#include "xg_render_source_commit.h"
#include "xg_render_source_frame.h"
#include "xg_render_surface_graph.h"
#include "xg_render_vram_journal.h"
#include "xg_render_vram_resources.h"

bool g_psx_xg_render_auth_cold_enabled;

bool psx_xg_render_auth_cold_enabled(void) { return false; }
bool psx_xg_render_auth_cold_hook_relevant(uint32_t hook, uint32_t pc,
                                           uint32_t instruction_word)
{
    (void)hook; (void)pc; (void)instruction_word;
    return false;
}
void psx_xg_render_auth_cold_hook(CPUState *cpu, uint32_t hook, uint32_t pc,
                                  uint32_t instruction_word,
                                  uint32_t delay_slot_word)
{
    (void)cpu; (void)hook; (void)pc; (void)instruction_word; (void)delay_slot_word;
}
void psx_xg_render_auth_warm_hook(CPUState *cpu, uint32_t hook, uint32_t pc,
                                  uint32_t instruction_word,
                                  uint32_t delay_slot_word)
{
    (void)cpu; (void)hook; (void)pc; (void)instruction_word; (void)delay_slot_word;
}
bool psx_xg_render_auth_source_site_lookup(
    uint32_t pc, uint32_t instruction_word,
    PsxXgRenderSourceSiteMetadata *out_metadata)
{
    (void)pc; (void)instruction_word; (void)out_metadata;
    return false;
}
bool psx_xg_render_auth_cold_source_pc_relevant(uint32_t pc)
{
    (void)pc;
    return false;
}
uint32_t psx_xg_render_auth_cold_instruction_flags(
    uint32_t pc, uint32_t instruction_word)
{
    (void)pc; (void)instruction_word;
    return 0;
}
bool psx_xg_render_auth_cold_source_observe(
    PsxXgRenderSourceStage stage, uint32_t pc, uint32_t instruction_word,
    uint32_t auxiliary)
{
    (void)stage; (void)pc; (void)instruction_word; (void)auxiliary;
    return false;
}
bool psx_xg_render_auth_cold_source_observe_cpu(
    CPUState *cpu, PsxXgRenderSourceStage stage, uint32_t pc,
    uint32_t instruction_word, uint32_t auxiliary)
{
    (void)cpu; (void)stage; (void)pc; (void)instruction_word; (void)auxiliary;
    return false;
}
bool psx_xg_render_auth_resident_ft4_observe(
    CPUState *cpu, uint32_t stage, uint32_t pc, uint32_t instruction_word)
{
    (void)cpu; (void)stage; (void)pc; (void)instruction_word;
    return false;
}
bool psx_xg_render_auth_native_cutover_pc_relevant(uint32_t pc)
{
    (void)pc;
    return false;
}
bool psx_xg_render_auth_overlay_cutover_relevant(
    uint32_t pc, uint32_t instruction_word)
{
    (void)pc; (void)instruction_word;
    return false;
}
bool psx_xg_render_auth_native_cutover_post_pc_relevant(uint32_t pc)
{
    (void)pc;
    return false;
}
bool psx_xg_render_auth_native_ft4_bypass(
    CPUState *cpu, uint32_t pc, uint32_t instruction_word)
{
    (void)cpu; (void)pc; (void)instruction_word;
    return false;
}
bool psx_xg_render_auth_movie_standalone_start(CPUState *cpu)
{
    (void)cpu;
    return false;
}
bool psx_xg_render_auth_movie_standalone_stop(void) { return false; }
bool psx_xg_render_auth_movie_field_start(CPUState *cpu)
{
    (void)cpu;
    return false;
}
bool psx_xg_render_auth_movie_field_stop(void) { return false; }
bool psx_xg_render_auth_movie_frame_complete(CPUState *cpu)
{
    (void)cpu;
    return false;
}
bool psx_xg_render_auth_ft4_geometry_pop(
    PsxXgRenderFt4Geometry *out_geometry)
{
    (void)out_geometry;
    return false;
}
void psx_xg_render_auth_capture_model_ft3_link(CPUState *cpu)
{
    (void)cpu;
}
void psx_xg_render_auth_capture_clear_tile(CPUState *cpu)
{
    (void)cpu;
}
void psx_xg_render_auth_capture_logo_sprite(
    uint32_t command_address, uint8_t color)
{
    (void)command_address; (void)color;
}
void psx_xg_render_auth_capture_tile_write(
    CPUState *cpu, uint32_t command_address, uint32_t writer_pc,
    uint8_t color)
{
    (void)cpu; (void)command_address; (void)writer_pc; (void)color;
}
void psx_xg_render_auth_note_code_write(uint64_t previous_generation,
                                        uint64_t generation,
                                        uint32_t address, uint32_t size)
{
    (void)previous_generation; (void)generation; (void)address; (void)size;
}
void psx_xg_render_auth_native_bad_entry(uint32_t owner, uint32_t pc)
{
    (void)owner; (void)pc;
}
void psx_xg_render_auth_note_candidate_dispatch(
    const PsxXgRenderAuthCandidate *candidate)
{
    (void)candidate;
}
void psx_xg_render_auth_note_artifact_candidate(
    const PsxXgRenderAuthCandidate *candidate)
{
    (void)candidate;
}
void psx_xg_render_auth_loader_mismatch(uint32_t pc)
{
    (void)pc;
}

/* Remaining native-renderer API referenced by the runtime TUs.
 * Prototypes are included verbatim via the headers above; each
 * function declines or reports empty so every runtime caller
 * takes its disabled/native-off path. */

/* xg_render_auth_runtime_control.h */
bool psx_xg_render_auth_accept_native_draw(const GpuRenderSemantic *semantic)
{
    (void)semantic;
    return 0;
}

/* xg_render_auth_runtime_control.h */
void psx_xg_render_auth_before_gpu_submission(void)
{
}

/* xg_render_auth_runtime_control.h */
void psx_xg_render_auth_checkpoint_cancel(PsxXgRenderCheckpointRestore *restore)
{
    (void)restore;
}

/* xg_render_auth_runtime_control.h */
void psx_xg_render_auth_checkpoint_commit_boot_restore(PsxXgRenderCheckpointRestore *restore)
{
    (void)restore;
}

/* xg_render_auth_runtime_control.h */
uint32_t psx_xg_render_auth_checkpoint_failure_stage(void)
{
    return 0;
}

/* xg_render_auth_runtime_control.h */
bool psx_xg_render_auth_checkpoint_prepare(const void *checkpoint, size_t checkpoint_size, PsxXgRenderCheckpointRestore **out_restore)
{
    (void)checkpoint;
    (void)checkpoint_size;
    (void)out_restore;
    return 0;
}

/* xg_render_auth_runtime_control.h */
size_t psx_xg_render_auth_checkpoint_size(void)
{
    return 0;
}

/* xg_render_auth_runtime_control.h */
bool psx_xg_render_auth_checkpoint_write(void *out_checkpoint, size_t checkpoint_size)
{
    (void)out_checkpoint;
    (void)checkpoint_size;
    return 0;
}

/* xg_render_auth_runtime_control.h */
void psx_xg_render_auth_cold_enable(bool enabled)
{
    (void)enabled;
}

/* xg_render_auth_runtime_control.h */
void psx_xg_render_auth_complete_ordering_table(uint32_t start_addr, uint32_t transferred_words)
{
    (void)start_addr;
    (void)transferred_words;
}

/* xg_render_auth_runtime_control.h
 * The runtime calls configure unconditionally (the original render path still
 * installs inert auth hooks), so accept-and-ignore: returning failure here
 * aborts boot before the guest ever runs. */
bool psx_xg_render_auth_configure(GuestRenderRenderMode requested_render_mode, PsxXgRenderPresentationGate presentation_gate, void *presentation_user_data)
{
    (void)requested_render_mode;
    (void)presentation_gate;
    (void)presentation_user_data;
    return 1;
}

/* xg_render_auth_runtime_control.h */
bool psx_xg_render_auth_configure_native_view(bool enabled, uint16_t aspect_num, uint16_t aspect_den, uint16_t canonical_width, uint16_t canonical_height)
{
    (void)enabled;
    (void)aspect_num;
    (void)aspect_den;
    (void)canonical_width;
    (void)canonical_height;
    return 1;
}

/* xg_render_auth_runtime_control.h */
bool psx_xg_render_auth_describe_native_work(XgRenderSourceFrameDescription *description)
{
    (void)description;
    return 0;
}

/* xg_render_auth_runtime_diagnostics.h */
size_t psx_xg_render_auth_field_fragment_snapshot(PsxXgRenderPreScenePrimitiveSnapshot *out_snapshots, size_t capacity)
{
    (void)out_snapshots;
    (void)capacity;
    return 0;
}

/* xg_render_auth_runtime_diagnostics.h */
void psx_xg_render_auth_instrumentation_snapshot(PsxXgRenderAuthInstrumentation *out_instrumentation)
{
    (void)out_instrumentation;
}

/* xg_render_auth_runtime_diagnostics.h */
void psx_xg_render_auth_model_ft3_shadow_snapshot(PsxXgRenderModelFt3ShadowSnapshot *out_snapshot)
{
    (void)out_snapshot;
}

/* xg_render_auth_runtime_diagnostics.h */
void psx_xg_render_auth_model_ft4_shadow_snapshot(PsxXgRenderModelFt4ShadowSnapshot *out_snapshot)
{
    (void)out_snapshot;
}

/* xg_render_auth_runtime_control.h */
bool psx_xg_render_auth_movie_owner_active(void)
{
    return 0;
}

/* xg_render_auth_runtime_diagnostics.h */
void psx_xg_render_auth_movie_owner_diagnostics(PsxXgRenderMovieOwnerDiagnostics *out_diagnostics)
{
    (void)out_diagnostics;
}

/* xg_render_auth_runtime_control.h */
void psx_xg_render_auth_note_gpu_semantic_current(const GpuRenderSemantic *semantic)
{
    (void)semantic;
}

/* xg_render_auth_runtime_control.h */
bool psx_xg_render_auth_note_vram_event(uint64_t guest_vblank_sequence, uint64_t guest_cycle, const GpuVramEvent *event)
{
    (void)guest_vblank_sequence;
    (void)guest_cycle;
    (void)event;
    return 0;
}

/* xg_render_auth_runtime_control.h */
/* The real implementation returns true whenever the request is not a native
 * UI-OT submission; dma.c treats false as a fatal halt. */
bool psx_xg_render_auth_prepare_ui_ot(uint32_t start_addr)
{
    (void)start_addr;
    return 1;
}

/* xg_render_auth_runtime_diagnostics.h */
size_t psx_xg_render_auth_pre_scene_snapshot(PsxXgRenderPreScenePrimitiveSnapshot *out_snapshots, size_t capacity)
{
    (void)out_snapshots;
    (void)capacity;
    return 0;
}

/* xg_render_auth_runtime_diagnostics.h */
void psx_xg_render_auth_projected_lifecycle_snapshot(PsxXgRenderProjectedLifecycleSnapshot *out_snapshot)
{
    (void)out_snapshot;
}

/* xg_render_auth_runtime_control.h */
void psx_xg_render_auth_register_code_watches(void (*set_range)(uint32_t physical_address, uint32_t size))
{
    (void)set_range;
}

/* xg_render_auth_runtime_diagnostics.h */
void psx_xg_render_auth_resident_text_snapshot(PsxXgRenderResidentTextSnapshot *out_snapshot)
{
    (void)out_snapshot;
}

/* xg_render_auth_runtime_diagnostics.h */
void psx_xg_render_auth_runtime_snapshot(PsxXgRenderAuthRuntimeSnapshot *out_snapshot)
{
    (void)out_snapshot;
}

/* xg_render_auth_runtime_control.h */
void psx_xg_render_auth_scene_boundary(void)
{
}

/* xg_render_auth_runtime_control.h */
void psx_xg_render_auth_set_exec_phase_exchange(PsxXgRenderExecPhaseExchange exchange)
{
    (void)exchange;
}

/* xg_render_auth_runtime_control.h */
void psx_xg_render_auth_set_native_work_mode(bool enabled)
{
    (void)enabled;
}

/* xg_render_auth_runtime_control.h */
void psx_xg_render_auth_set_terrain_temporal_coverage(bool enabled)
{
    (void)enabled;
}

/* xg_render_auth_runtime_control.h */
bool psx_xg_render_auth_source_boundary(uint64_t guest_vblank_sequence, uint64_t guest_cycle)
{
    (void)guest_vblank_sequence;
    (void)guest_cycle;
    return 0;
}

/* xg_render_auth_runtime_diagnostics.h */
void psx_xg_render_auth_sprite_ft4_shadow_snapshot(PsxXgRenderSpriteFt4ShadowSnapshot *out_snapshot)
{
    (void)out_snapshot;
}

/* xg_render_auth_runtime_diagnostics.h */
void psx_xg_render_auth_static_artifact_diagnostics(uint64_t *out_attempts, uint64_t *out_successes, uint32_t *out_last_pc, uint32_t *out_last_blocker)
{
    (void)out_attempts;
    (void)out_successes;
    (void)out_last_pc;
    (void)out_last_blocker;
}

/* xg_render_auth_runtime_control.h */
uint64_t psx_xg_render_auth_timeline_invalidate(XgRenderTimelineInvalidationReason reason)
{
    (void)reason;
    return 0;
}

/* xg_render_auth_runtime_diagnostics.h */
void psx_xg_render_auth_tim_route_diagnostics(PsxXgRenderTimRouteDiagnostics *out_diagnostics)
{
    (void)out_diagnostics;
}

/* xg_render_auth_runtime_diagnostics.h */
void psx_xg_render_auth_world_terrain_water_shadow_snapshot(PsxXgRenderWorldTerrainWaterShadowSnapshot *out_snapshot)
{
    (void)out_snapshot;
}

/* xg_render_auth_runtime_diagnostics.h */
void psx_xg_render_auth_zoom_template_contract_snapshot(PsxXgRenderZoomTemplateContractSnapshot *out_snapshot)
{
    (void)out_snapshot;
}

/* xg_render_motion.h */
bool xg_render_motion_binding_valid(const XgRenderMotionDrawBinding *binding)
{
    (void)binding;
    return 0;
}

/* xg_render_motion.h */
bool xg_render_motion_evaluate(XgRenderMotionRef previous, XgRenderMotionRef current, double alpha, XgRenderMotionEvaluation *out)
{
    (void)previous;
    (void)current;
    (void)alpha;
    (void)out;
    return 0;
}

/* xg_render_motion.h */
XgRenderMotionProjectResult xg_render_motion_project(const XgRenderMotionEvaluation *evaluation, const XgRenderMotionDrawBinding *binding, double screen_delta[2][3][3], double native_delta[2][3][2])
{
    (void)evaluation;
    (void)binding;
    (void)screen_delta;
    (void)native_delta;
    return 0;
}

/* xg_render_motion.h */
bool xg_render_motion_view(XgRenderMotionRef ref, const XgRenderMotionPose **out)
{
    (void)ref;
    (void)out;
    return 0;
}

/* xg_render_movie_publisher.h */
void xg_render_movie_publisher_diagnostics(XgRenderMovieDiagnostics *out_diagnostics)
{
    (void)out_diagnostics;
}

/* xg_render_native_target.h */
bool xg_render_native_target_take(uint32_t command_id, XgRenderNativeOperation *out_operation)
{
    (void)command_id;
    (void)out_operation;
    return 0;
}

/* xg_render_native_work.h */
void xg_render_native_work_cancel_pending(void)
{
}

/* xg_render_native_work.h */
bool xg_render_native_work_configure(const XgRenderNativeWorkServices *services)
{
    (void)services;
    return 0;
}

/* xg_render_native_work.h */
bool xg_render_native_work_enabled(void)
{
    return 0;
}

/* xg_render_native_work.h */
bool xg_render_native_work_operation(const XgRenderNativeOperation *operation, uint64_t guest_cycle)
{
    (void)operation;
    (void)guest_cycle;
    return 0;
}

/* xg_render_presentation_host.h */
bool xg_render_presentation_host_destroy(XgRenderPresentationHost *host)
{
    (void)host;
    return 0;
}

/* xg_render_presentation_host.h */
bool xg_render_presentation_host_join(XgRenderPresentationHost *host)
{
    (void)host;
    return 0;
}

/* xg_render_presentation_host.h */
void xg_render_presentation_host_notify(XgRenderPresentationHost *host)
{
    (void)host;
}

/* xg_render_presentation_host.h */
bool xg_render_presentation_host_pump(XgRenderPresentationHost *host)
{
    (void)host;
    return 0;
}

/* xg_render_presentation_host.h */
bool xg_render_presentation_host_set_hold_presenter(XgRenderPresentationHost *host, XgRenderPresentationHoldPresenter present_hold)
{
    (void)host;
    (void)present_hold;
    return 0;
}

/* xg_render_presentation_host.h */
void xg_render_presentation_host_shutdown(XgRenderPresentationHost *host)
{
    (void)host;
}

/* xg_render_presentation_host.h */
bool xg_render_presentation_host_snapshot(XgRenderPresentationHost *host, XgRenderPresentationHostSnapshot *out_snapshot)
{
    (void)host;
    (void)out_snapshot;
    return 0;
}

/* xg_render_presentation_host.h */
XgRenderPresentationHost * xg_render_presentation_host_start(const XgRenderWorkerServices *worker_services, const XgRenderPresenterServices *presenter_services, uint64_t presentation_period_ns)
{
    (void)worker_services;
    (void)presenter_services;
    (void)presentation_period_ns;
    return 0;
}

/* xg_render_presentation_host.h */
bool xg_render_presentation_host_sync_source_clock(XgRenderPresentationHost *host, uint64_t guest_cycle, int64_t guest_time_offset_ns, bool realtime, bool rebase)
{
    (void)host;
    (void)guest_cycle;
    (void)guest_time_offset_ns;
    (void)realtime;
    (void)rebase;
    return 0;
}

/* xg_render_presentation_host.h */
bool xg_render_presentation_host_time_until_present(XgRenderPresentationHost *host, uint64_t *out_nanoseconds)
{
    (void)host;
    (void)out_nanoseconds;
    return 0;
}

/* xg_render_presentation_host.h */
bool xg_render_presentation_host_time_until_pump(XgRenderPresentationHost *host, uint64_t *out_nanoseconds)
{
    (void)host;
    (void)out_nanoseconds;
    return 0;
}

/* xg_render_semantic_presentation.h */
bool xg_render_presentation_trace_get(uint64_t trace_sequence, XgRenderPresentationTraceEvent *out_event)
{
    (void)trace_sequence;
    (void)out_event;
    return 0;
}

/* xg_render_semantic_presentation.h */
uint64_t xg_render_presentation_trace_total(void)
{
    return 0;
}

/* xg_render_semantic_presentation.h */
bool xg_render_presenter_present_hold(const XgRenderPresenterServices *services)
{
    (void)services;
    return 0;
}

/* xg_render_resource_repository.h */
XgRenderResourceResult xg_render_resource_acquire_snapshot(XgRenderResourceHandle handle, uint64_t content_digest)
{
    (void)handle;
    (void)content_digest;
    return 0;
}

/* xg_render_resource_repository.h */
XgRenderResourceCapabilityResult xg_render_resource_capability_validate(const XgRenderResourceProvenance *provenance, XgRenderResourceOwnerKind owner_kind, uint64_t owner_generation, XgRenderResourceCapabilityMetadata *out_metadata)
{
    (void)provenance;
    (void)owner_kind;
    (void)owner_generation;
    (void)out_metadata;
    return 0;
}

/* xg_render_resource_repository.h */
bool xg_render_resource_descriptor_validate(const XgRenderResourceDescriptor *descriptor)
{
    (void)descriptor;
    return 0;
}

/* xg_render_resource_repository.h */
uint64_t xg_render_resource_digest(const void *bytes, size_t byte_count)
{
    (void)bytes;
    (void)byte_count;
    return 0;
}

/* xg_render_resource_repository.h */
XgRenderResourceResult xg_render_resource_release(XgRenderResourceHandle handle)
{
    (void)handle;
    return 0;
}

/* xg_render_resource_repository.h */
void xg_render_resource_repository_diagnostics(XgRenderResourceDiagnostics *out_diagnostics)
{
    (void)out_diagnostics;
}

/* xg_render_resource_repository.h */
XgRenderResourceResult xg_render_resource_view(XgRenderResourceHandle handle, XgRenderResourceView *out_view)
{
    (void)handle;
    (void)out_view;
    return 0;
}

/* xg_render_runtime_host_services.h
 * Same contract as the real library: a non-NULL services struct is accepted;
 * the stub simply never calls back into it. */
bool xg_render_runtime_configure_host_services(const XgRenderRuntimeHostServices *services)
{
    if (services == NULL) return 0;
    return 1;
}

/* xg_render_semantic_compositor.h */
void xg_render_semantic_compositor_diagnostics(XgRenderSemanticCompositorDiagnostics *out_diagnostics)
{
    (void)out_diagnostics;
}

/* xg_render_semantic_presentation.h */
void xg_render_semantic_presentation_diagnostics(XgRenderPresentationDiagnostics *out_diagnostics)
{
    (void)out_diagnostics;
}

/* xg_render_source_commit.h */
XgRenderSourceCommitResult xg_render_source_commit_copy_native_operation(XgRenderSourceCommitHandle commit, size_t index, XgRenderNativeOperation *out_operation)
{
    (void)commit;
    (void)index;
    (void)out_operation;
    return 0;
}

/* xg_render_source_commit.h */
XgRenderSourceCommitResult xg_render_source_commit_draw_copy(XgRenderSourceCommitHandle commit, size_t index, XgSemanticDrawRecord *out_draw)
{
    (void)commit;
    (void)index;
    (void)out_draw;
    return 0;
}

/* xg_render_source_commit.h */
XgRenderSourceCommitResult xg_render_source_commit_header_copy(XgRenderSourceCommitHandle commit, XgRenderSourceCommitHeader *out_header)
{
    (void)commit;
    (void)out_header;
    return 0;
}

/* xg_render_source_commit.h */
XgRenderSourceCommitResult xg_render_source_commit_motion_resource_copy(XgRenderSourceCommitHandle commit, size_t index, XgRenderMotionRef *out_resource)
{
    (void)commit;
    (void)index;
    (void)out_resource;
    return 0;
}

/* xg_render_source_commit.h */
XgRenderSourceCommitResult xg_render_source_commit_pass_copy(XgRenderSourceCommitHandle commit, size_t index, XgSemanticPassRecord *out_pass)
{
    (void)commit;
    (void)index;
    (void)out_pass;
    return 0;
}

/* xg_render_source_commit.h */
XgRenderSourceCommitResult xg_render_source_commit_resource_copy(XgRenderSourceCommitHandle commit, size_t index, XgSemanticResourceRef *out_resource)
{
    (void)commit;
    (void)index;
    (void)out_resource;
    return 0;
}

/* xg_render_source_commit.h */
XgRenderSourceCommitResult xg_render_source_commit_surface_edge_copy(XgRenderSourceCommitHandle commit, size_t index, XgSemanticSurfaceEdge *out_edge)
{
    (void)commit;
    (void)index;
    (void)out_edge;
    return 0;
}

/* xg_render_source_commit.h */
XgRenderSourceCommitResult xg_render_source_commit_temporal_coverage_copy(XgRenderSourceCommitHandle commit, size_t index, XgSemanticResourceRef *out_coverage)
{
    (void)commit;
    (void)index;
    (void)out_coverage;
    return 0;
}

/* xg_render_source_commit.h */
XgRenderSourceCommitResult xg_render_source_commit_temporal_publication_copy(XgRenderSourceCommitHandle commit, size_t index, XgRenderTemporalPublication *out_publication)
{
    (void)commit;
    (void)index;
    (void)out_publication;
    return 0;
}

/* xg_render_source_commit.h */
XgRenderSourceCommitResult xg_render_source_commit_ui_glyph_placement_copy(XgRenderSourceCommitHandle commit, size_t index, XgSemanticUiGlyphPlacementRecord *out_placement)
{
    (void)commit;
    (void)index;
    (void)out_placement;
    return 0;
}

/* xg_render_source_commit.h */
XgRenderSourceCommitResult xg_render_source_commit_ui_glyph_run_copy(XgRenderSourceCommitHandle commit, size_t index, XgSemanticUiGlyphRunRecord *out_glyph_run)
{
    (void)commit;
    (void)index;
    (void)out_glyph_run;
    return 0;
}

/* xg_render_source_commit.h */
XgRenderSourceCommitResult xg_render_source_commit_ui_node_copy(XgRenderSourceCommitHandle commit, size_t index, XgSemanticUiNodeRecord *out_node)
{
    (void)commit;
    (void)index;
    (void)out_node;
    return 0;
}

/* xg_render_source_frame.h */
void xg_render_source_frame_clear_host_callbacks(void)
{
}

/* xg_render_source_frame.h */
bool xg_render_source_frame_configure_host_callbacks(const XgRenderSourceFrameHostCallbacks *callbacks)
{
    (void)callbacks;
    return 0;
}

/* xg_render_source_frame.h */
void xg_render_source_frame_snapshot(XgRenderSourceFrameSnapshot *out_snapshot)
{
    (void)out_snapshot;
}

/* xg_render_surface_graph.h */
XgRenderSurfaceGraphResult xg_render_surface_graph_copy_publications(XgRenderSurfacePublication *out_publications, size_t publication_capacity, size_t *out_publication_count)
{
    (void)out_publications;
    (void)publication_capacity;
    (void)out_publication_count;
    return 0;
}

/* xg_render_surface_graph.h */
void xg_render_surface_graph_snapshot(XgRenderSurfaceGraphSnapshot *out_snapshot)
{
    (void)out_snapshot;
}

/* xg_render_source_commit.h */
bool xg_render_temporal_components_compatible(const XgRenderTemporalCoverageHeader *previous, const XgRenderTemporalComponent *a, const XgRenderTemporalCoverageHeader *current, const XgRenderTemporalComponent *b)
{
    (void)previous;
    (void)a;
    (void)current;
    (void)b;
    return 0;
}

/* xg_render_source_commit.h */
bool xg_render_temporal_coverage_view(XgSemanticResourceRef coverage, XgRenderTemporalCoverageView *out_view)
{
    (void)coverage;
    (void)out_view;
    return 0;
}

/* xg_render_vram_journal.h */
size_t xg_render_vram_journal_copy_mutations(XgRenderVramMutation *out_mutations, size_t mutation_capacity, uint64_t *out_mutation_total)
{
    (void)out_mutations;
    (void)mutation_capacity;
    (void)out_mutation_total;
    return 0;
}

/* xg_render_vram_journal.h */
void xg_render_vram_journal_snapshot(XgRenderVramJournalSnapshot *out_snapshot)
{
    (void)out_snapshot;
}

/* xg_render_vram_resources.h */
size_t xg_render_vram_resources_publications(XgRenderVramResourcePublicationSnapshot *out_publications, size_t capacity)
{
    (void)out_publications;
    (void)capacity;
    return 0;
}

/* xg_render_vram_resources.h */
size_t xg_render_vram_resources_retirements(XgRenderVramResourceRetirementSnapshot *out_retirements, size_t capacity)
{
    (void)out_retirements;
    (void)capacity;
    return 0;
}

/* xg_render_vram_resources.h */
uint64_t xg_render_vram_resources_retirement_total(void)
{
    return 0;
}

/* xg_render_vram_resources.h */
void xg_render_vram_resources_snapshot(XgRenderVramResourceSnapshot *out_snapshot)
{
    (void)out_snapshot;
}

/* xg_render_semantic_presentation.h */
bool xg_render_worker_source_deadline(XgRenderSourceCommitHandle commit, uint64_t *out_deadline_ns)
{
    (void)commit;
    (void)out_deadline_ns;
    return 0;
}
