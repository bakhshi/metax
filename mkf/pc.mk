
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

ifeq ($(MAKELEVEL),0)
$(warning make cannot not be ran from this folder)
$(warning you need to do 'cd' to project root directory)
$(error stopping build ...)
endif

$(call check_variable,mkf_path)
$(call check_variable,project_root)
$(call check_variable,project_version)
$(call check_variable,inc_dir)
$(call check_variable,lib_dir)
$(call check_variable,pc_path)
$(call check_variable,lib_name)
$(call check_variable,lib_version)

pc_file:=$(pc_path)/$(lib_name).pc

ifeq ($(lib_version),)
lib_version:=$(project_version)
endif

.PHONY: pc
pc: $(pc_file)

$(pc_file): 
	$(echo) "" > $@
	$(echo) "prefix=$(project_root)" >> $@
	$(echo) -n "libdir=$$" >> $@
	$(echo) "{prefix}/$(lib_dir)" >> $@
	$(echo) -n "includedir=$$" >> $@
	$(echo) "{prefix}/$(inc_dir)" >> $@
	$(echo) "" >> $@
	$(echo) "Name: $(lib_name)" >> $@
	$(echo) "Version: $(lib_version)" >> $@
	$(echo) "Description: $(project_name) $(project_version)" >> $@
	$(echo) "Requires: $(libs)" >> $@
	$(echo) -n "Libs: " >> $@
ifneq ($(strip $(cpp_files)),)
ifeq ($(shell uname), Darwin)
	$(echo) -n "-Wl,-rpath,$$" >> $@
	$(echo) -n "{prefix}/lib " >> $@
	$(echo) -n "-L$$" >> $@
	$(echo) -n "{prefix}/lib " >> $@
	$(echo) -n "-Wl,-rpath,$$" >> $@
	$(echo) -n "{prefix}/lib32 " >> $@
	$(echo) -n "-L$$" >> $@
	$(echo) -n "{prefix}/lib32 " >> $@
	$(echo) -n "-Wl,-rpath,$$" >> $@
	$(echo) -n "{prefix}/lib64 " >> $@
	$(echo) -n "-L$$" >> $@
	$(echo) -n "{prefix}/lib64 " >> $@
	$(echo) -n " -l$(lib_name)" >> $@
else
	$(echo) -n "-Wl,-rpath=$$" >> $@
	$(echo) -n "{prefix}/lib " >> $@
	$(echo) -n "-L$$" >> $@
	$(echo) -n "{prefix}/lib " >> $@
	$(echo) -n "-Wl,-rpath=$$" >> $@
	$(echo) -n "{prefix}/lib32 " >> $@
	$(echo) -n "-L$$" >> $@
	$(echo) -n "{prefix}/lib32 " >> $@
	$(echo) -n "-Wl,-rpath=$$" >> $@
	$(echo) -n "{prefix}/lib64 " >> $@
	$(echo) -n "-L$$" >> $@
	$(echo) -n "{prefix}/lib64 " >> $@
	$(echo) -n " -l$(lib_name)" >> $@
endif
endif
	$(echo) "" >> $@
	$(echo) -n "Cflags: -I$$" >> $@
	$(echo) "{prefix}/$(inc_dir)" >> $@

