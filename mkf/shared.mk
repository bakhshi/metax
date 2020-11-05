
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

$(call check_variable,lib_dir)
$(call check_variable,lib_name)
$(call check_variable,obj_files)
$(call check_variable,linker)
$(call check_variable,linker_flags)
$(call check_variable,striper)
$(call check_variable,striper_flags)
$(call check_variable,dynamic_lib_ext)
$(call check_variable,dynamic_lib_flag)
$(call check_variable,install_name)

lp:=$(project_root)/$(lib_dir)
so:=$(lp)/lib$(lib_name).$(dynamic_lib_ext)

pkg_libs_file := $(project_root)/$(pkg_dir)/$(lib_name)/pkg_libs.mk
project_makefile := $(project_root)/$(src_dir)/$(lib_name)/makefile

-include $(pkg_libs_file)

.PHONY: shared 
shared: $(so) 

$(so): $(obj_files)
	$(echo) "LDS	$(lib_name)"
	rm -f $@
ifeq ($(shell uname),Darwin)
	$(linker) $^  $(linker_flags) $(l) $(dynamic_lib_flag) $(install_name)/$(shell basename $@) -o $@
else
	$(linker) $^ $(l) $(linker_flags) $(dynamic_lib_flag) -o $@
endif
ifeq ($(config_type),release)
	$(striper) $(striper_flags) $@
endif

$(pkg_libs_file) : $(project_makefile)
ifneq ($(words $(libs)),0)
	$(echo) "GLF	$(lib_name)"
	$(echo) -n "l := " > $@
	for i in $(libs); \
	do \
		$(echo) -n `pkg-config --libs $$i` >> $@; \
		$(echo) -n " " >> $@; \
	done 
endif
