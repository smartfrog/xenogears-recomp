/* vita-sce-data-reserve.ld — reserve address space for vita-elf-create.
 *
 * vita-elf-create appends the SCE module data (module info, process/libc
 * params, exports, imports, FNID/VNID/FStub/VStub tables, relocations) at the
 * end of the segment that contains the ELF entry point — the R E text
 * segment. It requires that [text_end, text_end + sce_data_size) does not
 * overlap the next segment and aborts with
 *   "Cannot allocate N bytes for SCE data at end of segment 0; segment 1
 *    overlaps"
 * when the gap left by the linker script's DATA_SEGMENT_ALIGN(0x10000, ...)
 * is too small. That gap is 0x10000 - (text_end mod 0x10000), so it shrinks as
 * the recompiled game code grows.
 *
 * Growing the section to (next 64 KiB boundary + 4 KiB) makes the following
 * DATA_SEGMENT_ALIGN round up to the boundary after that, leaving a fixed
 * ~60 KiB gap regardless of the text size.
 *
 * The section needs an input section (vita/sce_data_reserve.c) so it inherits
 * read-only flags instead of the linker's writable default.
 */
SECTIONS
{
    .xg_vita_sce_reserve ALIGN(4) :
    {
        KEEP(*(.xg_vita_sce_reserve))
        . = ALIGN(0x10000) + 0x1000;
    }
}
INSERT AFTER .eh_frame;
