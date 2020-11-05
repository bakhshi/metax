
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

$(call check_variable,lib_dir)
$(call check_variable,lib_name)
$(call check_variable,obj_files)
$(call check_variable,archiver)
$(call check_variable,archiver_flags)

lp:=$(project_root)/$(lib_dir)
archive_file:=$(lp)/lib$(lib_name).a

.PHONY: archive
archive: $(archive_file) 

$(archive_file): $(obj_files) 
	$(echo) "AR	$(lib_name)"
	rm -f $@
	$(archiver) $(archiver_flags) $@ $(obj_files) 

