
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

include $(mkf_path)/definitions.mk

$(call check_variable,project_root)
$(call check_variable,project_version)
$(call check_variable,install_path)
$(call check_variable,dbg_package)
$(call check_variable,opt_package)
$(call check_variable,build_type)
$(call check_variable,lib_dir)
$(call check_variable,pc_dir)

ifeq ($(strip $(build_type)),debug)
package:=$(dbg_package)
else
ifneq ($(strip $(build_type)),release)
$(error 'build_type' must be either debug or release)
endif
package:=$(opt_package)
endif

ifeq ($(filter lib,$(package)),lib) 
pcp:=$(project_root)/$(lib_dir)/$(pc_dir)
pcfs=$(wildcard $(pcp)/*.pc)
endif

pcf_install_path=$(install_path)/$(lib_dir)/$(pc_dir)

.PHONY: install
install : update_rpaths

.PHONY: update_rpaths
update_rpaths: install_package
	@$(echo) -n "Updating rpaths ... "
	@if [ -d $(install_path)/$(lib_dir) ]; then \
	for so in `ls $(install_path)/$(lib_dir) | grep -e "\.so"`; do \
	patchelf --set-rpath $(RPATHS)	\
		$(install_path)/$(lib_dir)/$$so > /dev/null; \
	done; \
	fi;
	@if [ -d $(install_path)/$(lib_dir) ]; then \
	for dylib in `ls $(install_path)/$(lib_dir) | grep -e "\.dylib"`; do \
	install_name_tool -rpath $(project_root)/$(lib_dir) \
	        $(install_path)/$(lib_dir)	\
		$(install_path)/$(lib_dir)/$$dylib > /dev/null; \
	done; \
	fi;
ifeq ($(shell uname),Darwin)
	@if [ -d $(install_path)/$(bin_dir) ]; then \
	for so in `ls $(install_path)/$(bin_dir)`; do \
	install_name_tool -rpath $(project_root)/$(lib_dir) \
	        $(install_path)/$(lib_dir)	\
			$(install_path)/$(bin_dir)/$$so > /dev/null; \
	done; \
	fi;
else
	@if [ -d $(install_path)/$(bin_dir) ]; then \
	for so in `ls $(install_path)/$(bin_dir)`; do \
	patchelf --set-rpath $(RPATHS) \
			$(install_path)/$(bin_dir)/$$so > /dev/null; \
	done; \
	fi;
endif
	@$(echo) "done"

.PHONY: install_package
install_package: default user_docs
	@$(echo) -n "Installing ... "
	@rm -rf $(install_path)
	@mkdir -p $(install_path)
	@cp -RfL $(package) $(install_path)
	@rm -rf `find $(install_path) -name .svn`
	@$(foreach pcf, \
	           $(pcfs), \
		   sed s/prefix=.*// $(pcf) > tmp_pc && \
		   $(echo) -n "prefix=$(install_path)" > \
		   $(pcf_install_path)/$(notdir $(pcf)) && \
		   cat tmp_pc >> $(pcf_install_path)/$(notdir $(pcf));)
	@rm -f tmp_pc
	@$(echo) "done"

