project(libkxld)

add_compile_definitions(
	PRIVATE
)

set(DYLIB_INSTALL_NAME "/usr/lib/system/libkxld.dylib")
add_darling_library(libkxld
	kxld.c
	kxld_array.c
	kxld_copyright.c
	kxld_demangle.c
	kxld_dict.c
	kxld_kext.c
	kxld_object.c
	kxld_reloc.c
	kxld_sect.c
	kxld_seg.c
	kxld_splitinfolc.c
	kxld_srcversion.c
	kxld_sym.c
	kxld_symtab.c
	kxld_util.c
	kxld_uuid.c
	kxld_versionmin.c
	kxld_vtable.c
)
make_fat(libkxld)
target_link_libraries(libkxld system cxxabi)
set_target_properties(libkxld PROPERTIES OUTPUT_NAME "kxld")
