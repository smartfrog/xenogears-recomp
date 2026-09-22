/* sce_data_reserve.c — anchor for the Vita-only SCE data reserve section.
 *
 * The linker fragment vita/vita-sce-data-reserve.ld grows .xg_vita_sce_reserve
 * to the next 64 KiB boundary plus 4 KiB. An output section with no input
 * sections gets the linker's default (writable) flags, which would turn the
 * whole text segment RWX, so this read-only anchor gives the section its
 * flags. Nothing references the symbol: it exists only to be placed. */
__attribute__((section(".xg_vita_sce_reserve"), used))
const char xg_vita_sce_data_reserve_anchor[1] = {0};
