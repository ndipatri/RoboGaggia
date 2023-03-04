# (c) Copyright 2021 Aaron Kimball
#
# Sample Makefile for application or library.

# Alternatively, if you are compiling a library, specify lib_name.
# This example creates `libyourlib.a`:
#
lib_name := tinycollections

# Uncomment definitions to direct header files for this library to be installed
# in architecture-specific or mcu-specific subdirectories of default /include/
# target of `make install`.
#arch_specific_h := 1
#mcu_specific_h := 1

# Uncomment the following line to install your lib's headers to /default/include/path/${lib_name}
#use_header_suffix_dir := 1

# Alternatively, use the following line to install your lib's headers to a different subpath
# of the default include path:
#include_install_dir_suffix := somedirname

# Other libraries you depend on. If you depend on libfoo.a, write 'foo'.
# If a library 'foo' depends on another library 'bar', you must enumerate both
# (`libs := foo bar`).  Further, you *must* list foo /before/ bar, otherwise you will see
# errors in the link phase about missing symbols.
#libs := wire

# List all directories of source files. The current directory '.' is included by default.
src_dirs := src src/tc

# Add further include or lib paths with:
# include_dirs += (dir1) (dir2) (dir3)...
# lib_dirs += (some_dir)
# These dir names each be prefixed with -I or -L as appropriate. Useful macros to reference
# are $(include_root), $(arch_include_root), and $(mcu_include_root) for where lib-specific
# headers may have been installed as a subdir. These dirs are themselves already on the -I
# path, and the mcu-specific lib dir is already on the -L path.

# Add any other compilation flags necessary:
# XFLAGS += (...)

# Finally, include the main makefile library.
# This creates targets like 'all', 'install', 'upload', 'clean'...
include ../arduino-makefile/arduino.mk

# Use `make config` to inspect the build configuration.
# Use `make help` to see a list of available targets.

# Add support for our local unit test suite
test: library
	$(MAKE) -C test all

.PHONY: test
