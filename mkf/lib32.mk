
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
$(call check_variable,cpp_files)

compiler_flags := -m32 -Di386 $(filter-out -m64, $(compiler_flags))
linker_flags := -m32 $(filter-out -m64, $(linker_flags))
asm_compiler_flags := -m32 $(filter-out -m64, $(asm_compiler_flags))

.PHONY: default

ifeq ($(words $(cpp_files)),0)
default: compile pc
else
default: compile lib pc
endif

obj_dir=$(obj32_dir)

include $(mkf_path)/compile.mk

.PHONY: lib

ifeq ($(strip $(link_type)),dynamic)
lib: shared
else
ifeq ($(strip $(link_type)),static)
lib: archive
else
$(error 'link_type' must be either static or dynamic)
endif
endif

lib_dir = $(lib32_dir)

ifeq ($(strip $(link_type)),dynamic)
include $(mkf_path)/shared.mk
else
ifeq ($(strip $(link_type)),static)
include $(mkf_path)/archive.mk
else
$(error 'link_type' must be either static or dynamic)
endif
endif

pc_path = $(pc_path32)
include $(mkf_path)/pc.mk

