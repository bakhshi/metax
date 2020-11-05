
#
# Copyright © 2005-2010 Instigate CJSC, Armenia
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free
# Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
#

$(call check_variable,project_root)
$(call check_variable,bin_dir)
$(call check_variable,linker)
$(call check_variable,exe_name)
$(call check_variable,obj_files)
$(call check_variable,linker_flags)

exe:=$(project_root)/$(bin_dir)/$(exe_name)

pkg_libs_file := $(project_root)/$(pkg_dir)/$(lib_name)/pkg_libs.mk
project_makefile := $(project_root)/$(src_dir)/$(lib_name)/makefile

ifeq ($(strip $(link_type)),static)
pkg_flag := --static
endif

-include $(pkg_libs_file)

ifneq ($(words $(cuda_files)),0)
l+=`pkg-config --libs cuda`
endif

.PHONY: link
link: $(exe)

$(exe): $(obj_files)
	$(echo) "LD	$(exe_name)"
	$(linker) $^ $(l) $(linker_flags) -o $@

$(pkg_libs_file) : $(project_makefile)
ifneq ($(words $(libs)),0)
	$(echo) "GLF	$(lib_name)"
	$(echo) -n "l := " > $@
	for i in $(libs); \
	do \
		$(echo) -n `pkg-config $(pkg_flag) --libs $$i` >> $@; \
		$(echo) -n " " >> $@; \
	done 
endif

