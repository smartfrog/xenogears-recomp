/* Vita module parameters. vita-elf-create copies these into the SCE module
 * info; the kernel applies them when it creates the user main thread. The
 * runtime keeps several large frames (overlay/debug captures, guest boot) on
 * that thread, so the 256 KiB default is replaced with 4 MiB. */
#include <psp2/types.h>

const SceSize sceUserMainThreadStackSize = 4 * 1024 * 1024;
