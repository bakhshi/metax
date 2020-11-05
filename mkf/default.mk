
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

$(call check_variable,setup_file)
$(call check_variable,projects)
$(call check_variable,mkf_path)
$(call check_variable,make_flags)
$(call check_variable,bin_dir)
$(call check_variable,src_dir)
$(call check_variable,lib_dir)
$(call check_variable,obj_dir)
$(call check_variable,inc_dir)
$(call check_variable,pc_dir)
$(call check_variable,pkg_dir)

.PHONY: default
default: check $(projects)


.PHONY: $(projects)
$(projects): create_directories
	@$(MAKE) $(make_flags) -C $@

f:=$(bin_dir) $(lib_dir) $(lib64_dir) $(lib32_dir) $(lib64_dir)/$(pc_dir) $(lib32_dir)/$(pc_dir) $(obj_dir) $(inc_dir) $(lib_dir)/$(pc_dir) $(obj32_dir) $(obj64_dir)
f+=$(patsubst $(src_dir)/%, $(obj_dir)/%, $(projects))
f+=$(patsubst $(src_dir)/%, $(inc_dir)/%, $(projects))
f+=$(patsubst $(src_dir)/%, $(pkg_dir)/%, $(projects))
f+=$(patsubst $(src_dir)/%, $(obj64_dir)/%, $(projects))
f+=$(patsubst $(src_dir)/%, $(obj32_dir)/%, $(projects))
f:=$(addprefix $(project_root)/, $(f))

.PHONY: create_directories
create_directories:
	@mkdir -p -m 775 $(f)

.PHONY: depends
depends:
	@$(foreach i, $(projects),  \
		$(echo) "make[1]: Entering Directory \`$i'"; \
		$(MAKE) --no-print-directory -s depends -C $i; \
		$(echo) "make[1]: Leaving Directory \`$i'";)

include $(mkf_path)/preconditions.mk
