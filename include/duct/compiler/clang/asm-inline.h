// workaround for Clang choking on Linux headers that use `asm __inline(...)`
#ifdef __clang__
#undef asm_inline
#define asm_inline asm
#endif
