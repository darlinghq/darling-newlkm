#ifndef _DARLING_LKM_DUCT_GCC_HAS_BUILTIN_H_
#define _DARLING_LKM_DUCT_GCC_HAS_BUILTIN_H_

#ifndef __clang__

#ifndef __has_builtin

// GCC < 10 needs a shim for __has_builtin

// list of builtins that Mach checks for
//   __builtin_assume
//   __builtin_add_overflow
//   __builtin_sub_overflow
//   __builtin_mul_overflow
//   __builtin_dynamic_object_size
//   __builtin___memcpy_chk
//   __builtin___memmove_chk
//   __builtin___strncpy_chk
//   __builtin___strncat_chk
//   __builtin___strlcat_chk
//   __builtin___strlcpy_chk
//   __builtin___strcpy_chk
//   __builtin___strcat_chk
//   __builtin___memmove_chk
//   __builtin_ia32_rdpmc

// GCC does not have `__builtin_assume`
//#define _hasbin___builtin_assume 1

#if __GNUC__ >= 5
	// GCC 5+ has builtin safe math functions
	#define _hasbin___builtin_add_overflow 1
	#define _hasbin___builtin_sub_overflow 1
	#define _hasbin___builtin_mul_overflow 1
#endif

// GCC does not have `__builtin_dynamic_object_size`
//#define _hasbin___builtin_dynamic_object_size 1

#if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 1)
	// GCC 4.1+ has builtin `chk` variants
	// (maybe before 4.1 they're available too, but i couldn't find them
	// in the older GCC source)
	#define _hasbin___builtin___memcpy_chk 1
	#define _hasbin___builtin___memmove_chk 1
	#define _hasbin___builtin___strncpy_chk 1
	#define _hasbin___builtin___strncat_chk 1
	#define _hasbin___builtin___strlcat_chk 1
	#define _hasbin___builtin___strlcpy_chk 1
	#define _hasbin___builtin___strcpy_chk 1
	#define _hasbin___builtin___strcat_chk 1
	#define _hasbin___builtin___memmove_chk 1
#endif

#if __GNUC__ >= 7
	// GCC 7+ has builtin 32-bit `rdpmc`
	// (same as with the `chk` builtins: might be available in older GCC,
	// but i couldn't find it in older GCC sources)
	#define _hasbin___builtin_ia32_rdpmc 1
#endif

#define __has_builtin_internal(x) defined(_hasbin_ ## x)
#define __has_builtin(x) __has_builtin_internal(x)

#endif

#endif // !__clang

#endif // _DARLING_LKM_DUCT_GCC_HAS_BUILTIN_H_
