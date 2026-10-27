#include "vim.h"

char_u *default_vim_dir = (char_u *)"/data/share/vim";
char_u *default_vimruntime_dir = (char_u *)"/data/share/vim/vim92";
char_u *all_cflags = (char_u *)"clang --target=aarch64-unknown-none-elf -O2";
char_u *all_lflags = (char_u *)"ld.lld -static";
char_u *compiled_user = (char_u *)"";
char_u *compiled_sys = (char_u *)"Lume AArch64";
