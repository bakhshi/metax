
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

ifeq ($(build_lib),)
p := $(shell pwd)
p := $(notdir $p)

.PHONY: mixed_libs
mixed_libs :
	@$(MAKE) $(make_flags) -C $(PWD) $(src_dir)/$(p) build_lib=32
	@$(MAKE) $(make_flags) -C $(PWD) $(src_dir)/$(p) build_lib=64
else

ifeq ($(build_lib),32) 
-include $(mkf_path)/lib32.mk
else 
ifeq ($(build_lib),64) 
-include $(mkf_path)/lib64.mk
else
$(error "Error in build scripts: build_lib=$(build_lib)")
endif
endif

endif



