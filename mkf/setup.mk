
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

ifndef build_type
$(warning 'build_type' is not specified during setup process, \
	  build_type=debug default value will be used)
build_type:=debug
else
ifneq ($(build_type),release)
ifneq ($(build_type),debug)
$(error 'build_type' should either be set to 'release' or 'debug')
endif
endif
endif

ifndef link_type
$(warning 'link_type' is not specified during setup process, \
	  link_type=dynamic default value will be used)
link_type:=dynamic
else
ifneq ($(link_type),dynamic)
ifneq ($(link_type),static)
$(error 'link_type' should either be set to 'dynamic' or 'static'. \
	Please correct it in $(setup_file) file)
endif
endif
endif

ifndef use_sdk
$(warning 'use_sdk' is not specified during setup process, \
	  use_sdk=no default value will be used)
use_sdk:=no
else
ifneq ($(use_sdk),yes)
ifneq ($(use_sdk),no)
$(error 'use_sdk' should either be set to 'yes' or 'no')
endif
endif
endif

ifndef sdk_path
ifeq ($(use_sdk),yes)
$(warning 'sdk_path' is not defined. Using default path \
/opt/instigate/gnu_linux_sdk/2.2)
endif
export sdk_path=/opt/instigate/gnu_linux_sdk/2.2
endif
ifndef office_sdk_path
ifeq ($(use_sdk),yes)
$(warning 'office_sdk_path' is not defined. Using default path \
/opt/instigate/gnu_linux_office/1.3)
endif
export office_sdk_path=/opt/instigate/gnu_linux_office/1.3
endif

ifndef use_gcov
$(warning 'use_gcov' is not specified during setup process, \
	  use_gcov=no default value will be used)
use_gcov:=no
else
ifneq ($(use_gcov),yes)
ifneq ($(use_gcov),no)
$(error 'use_gcov' should either be set to 'yes' or 'no')
endif
endif
endif

ifndef target
$(warning 'target' is not specified during setup process, \
	  target=$(shell uname) value will be used)
target:=$(shell uname)
endif

export SDK_ENV_PATH := $(sdk_path)
export OFFICE_ENV_PATH := $(office_sdk_path)

.PHONY: setup
setup:: clean
ifneq ($(wildcard $(setup_file)),)
	@$(echo) "Deleting old setup file ..."
	@rm -f $(setup_file)
endif
	@$(echo) "Generating new setup file ... "
	@$(echo) "" > $(setup_file)
	@$(echo) "# THIS FILE IS READONLY !!!" >> $(setup_file)
	@$(echo) "#" >> $(setup_file)
	@$(echo) "# This file can be changed by entering" \
		>> $(setup_file)
	@$(echo) "# \"make setup use_sdk=<use_sdk> use_gcov=<use_gcov>" \
		>> $(setup_file)
	@$(echo) "#	build_type=<build_type> link_type=<link_type>" \
		>> $(setup_file)
	@$(echo) "#	sdk_path=<sdk_path> " \
		>> $(setup_file)
	@$(echo) "#	office_sdk_path=<office_sdk_path>\"" \
		>> $(setup_file)
	@$(echo) "# command from command line." >> $(setup_file)
	@$(echo) "# <build_type> must be either 'debug' or 'release'" \
		>> $(setup_file)
	@$(echo) "# <link_type> must be either 'dynamic' or 'static'" \
		>> $(setup_file)
	@$(echo) "# <use_sdk> must be either 'yes' or 'no'" >> \
		$(setup_file)
	@$(echo) "# <use_gcov> must be either 'yes' or 'no'" >> \
		$(setup_file)
	@$(echo) "" >> $(setup_file)
	@$(echo) "export build_type:=$(build_type)" >> $(setup_file)
	@$(echo) "export link_type:=$(link_type)" >> $(setup_file)
	@$(echo) "export use_sdk:=$(use_sdk)" >> $(setup_file)
	@$(echo) "export use_gcov:=$(use_gcov)" >> $(setup_file)
	@$(echo) "export target:=$(target)" >> $(setup_file)
ifeq ($(use_sdk),yes)
	@$(echo) "export SDK_ENV_PATH := $(sdk_path)" >> $(setup_file)
	@$(echo) "export OFFICE_ENV_PATH := $(office_sdk_path)" >> $(setup_file)
endif
	@chmod 444 $(setup_file)
	@$(echo) "-------------------------------------"
	@$(echo) "         BUILD CONFIGURATION         "
	@$(echo) "         -------------------         "
	@$(echo) "    BUILD TYPE = $(build_type)"
	@$(echo) "    LINK TYPE = $(link_type)"
	@$(echo) "    USE SDK = $(use_sdk)"
	@$(echo) "    USE GCOV = $(use_gcov)"
	@$(echo) "    TARGET = $(target)"
ifeq ($(use_sdk),yes)
	@$(echo) "    SDK_PATH = $(sdk_path)"
	@$(echo) "    OFFICE_SDK_PATH = $(office_sdk_path)"
endif
	@$(echo) "-------------------------------------"

include $(mkf_path)/clean.mk

