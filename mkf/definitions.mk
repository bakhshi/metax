
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

ifneq ($(words $(shell pwd)),1)
$(error 'pwd' contains whitespaces)
endif

ifneq ($(check), off)
check_variable=$(if $($1), ,$(error '$(strip $1)' is undefined))
endif

export setup_file := _setup_

export src_dir	 	:= src
export bin_dir	 	:= bin
export lib_dir	 	:= lib
export lib32_dir 	:= lib32
export lib64_dir 	:= lib64
export obj_dir	 	:= obj
export obj32_dir 	:= obj32
export obj64_dir 	:= obj64
export inc_dir	 	:= inc
export pc_dir	 	:= pkgconfig
export utl_dir	 	:= utl
export doc_dir	 	:= doc
export pkg_dir	 	:= pkg
export result_suffix := results


export dev_doc_dir	:= $(doc_dir)/developer
export usr_doc_dir	:= $(doc_dir)/user
export tutor_dir  	:= $(usr_doc_dir)/tutorials
export manual_dir 	:= $(usr_doc_dir)/manual
export test_result_dir ?= $(project_root)/$(test_type)_$(result_suffix)
export test_result ?= $(project_root)/test_$(result_suffix).txt

export acceptance_tests_dir := acceptance_tests
export black_box_tests_dir := black_box_tests
export regression_tests_dir := regression_tests
export continuous_tests_dir := continuous_tests
export acceptance_tests_results_dir := acceptance_tests_$(result_suffix)
export black_box_tests_results_dir := black_box_tests_$(result_suffix)
export regression_tests_results_dir := regression_tests_$(result_suffix)
export continuous_tests_results_dir := continuous_tests_$(result_suffix)
export coverage_result := $(project_root)/coverage_$(result_suffix).txt
export coverage_result_dir := $(project_root)/tests_coverage
export coverage_projects


# TODO move this check to mkf/main and split definitions to 2 parts : pre-setup
# definitions and post-setup.
ifeq ($(filter setup,$(MAKECMDGOALS)),setup)
	include $(mkf_path)/setup.mk
endif

ifneq ($(filter setup,$(MAKECMDGOALS)),setup)
	include $(setup_file)
endif

ifeq ($(target),sh4)
export SH4_TOOLCHAIN_PATH := /usr/local/kreatv/toolchain/st40/4.0.1
ifndef TOOLS_PREFIX
export TOOLS_PREFIX := $(SH4_TOOLCHAIN_PATH)/bin/sh4-kreatv-linux-uclibc-
d := $(dir $(TOOLS_PREFIX))
ifeq ("$(wildcard $d)","")
$(warning toolchain path not found - $(sh4_linux_headers))
endif
endif
endif

export compiler		:= $(TOOLS_PREFIX)gcc
export cuda_compiler	:= nvcc
export asm_compiler	:= $(TOOLS_PREFIX)gcc
export linker		:= $(TOOLS_PREFIX)gcc
export archiver		:= ar
export striper		:= strip
export ln		:= ln
export dynamic_lib_ext	:= so
export dynamic_lib_flag	:= -shared
export striper_flags	:= -s
export install_name

export moc		:= moc
export uic		:= uic
export rcc		:= rcc

export make_flags	:= -s --print-directory

export mkf_path		:= $(strip $(mkf_path))
export project_root	:= $(strip $(project_root))
export project_name	:= $(strip $(project_name))
export project_version	:= $(strip $(project_version))
export link_type	:= $(strip $(link_type))
export build_type	:= $(strip $(build_type))
export use_sdk		:= $(strip $(use_sdk))
export use_gcov		:= $(strip $(use_gcov))
export install_path	:= $(strip $(install_path))
export dbg_package
export opt_package

export echo := $(shell which echo)

export test_type:=$(strip $(test_type))

ifneq ($(link_type),dynamic)
ifneq ($(link_type),static)
ifeq ($(wildcard $(setup_file)),)
$(error $(setup_file) file is missing \
	please run 'make setup' to generate one)
else
$(error 'link_type' must be either 'dynamic' or 'static'. \
	Please re-run 'make setup' to generate correct setup file)
endif
endif
endif

ifneq ($(build_type),debug)
ifneq ($(build_type),release)
ifeq ($(wildcard $(setup_file)),)
$(error $(setup_file) file is missing \
	please run 'make setup' to generate one)
else
$(error 'build_type' must be either 'debug' or 'release'. \
	Please re-run 'make setup' to generate correct setup file)
endif
endif
endif

ifeq ($(use_sdk), yes)
ifndef SDK_ENV_PATH
$(error SDK path is not defined, please run make setup)
endif
ifndef OFFICE_ENV_PATH
$(error Office SDK path is not defined, please run make setup)
endif
export PATH := $(SDK_ENV_PATH)/bin:$(OFFICE_ENV_PATH)/bin:$(PATH)
export LD_LIBRARY_PATH := $(SDK_ENV_PATH)/lib64:$(SDK_ENV_PATH)/lib:$(SDK_ENV_OO_PATH)/lib:$(LD_LIBRARY_PATH)
export PKG_CONFIG_PATH := $(SDK_ENV_PATH)/lib/pkgconfig:$(PKG_CONFIG_PATH)
export RPATHS := $(SDK_ENV_PATH)/lib64:$(SDK_ENV_PATH)/$(lib_dir):$(OFFICE_ENV_PATH)/lib64:$(OFFICE_ENV_PATH)/$(lib_dir):$(RPATHS)
else
export SDK_ENV_PATH :=
export OFFICE_ENV_PATH :=
endif

export RPATHS := $(install_path)/$(lib_dir):/lib64:/$(lib_dir)/:$(RPATHS)

compiler_flags:=-pipe \
		       -x c++ \
		       -std=c++11 \
		       -fPIC \
		       -pedantic \
		       -W \
		       -Wall \
		       -Wwrite-strings \
		       -Wcast-align \
		       -Wcast-qual \
		       -Wpointer-arith \
		       -Wshadow \
		       -Wendif-labels \
		       -Wundef \
		       -Wfloat-equal \
		       -Werror \
		       -Wconversion \
		       -DPROJECT_VERSION=\"$(project_version)\" \
		       -DUSE_SDK=\"$(use_sdk)\" \
		       # this flag is valid only for C.  Please consider this
		       # flag when adding support for C language
		       #-Wimplicit \

ifeq ($(strip $(build_type)),debug)
compiler_flags += -g -g3 -ggdb3
else
ifeq ($(strip $(build_type)),release)
compiler_flags += -O3 -DNDEBUG
else
$(error 'build_type' must be either debug or release)
endif
endif

ifeq ($(shell uname),Darwin)
ifeq ($(shell arch),i386)
	compiler_flags += -D__$(shell arch)__=1 \
			  -D__ppc64__=0
else
	compiler_flags += -D__$(shell arch)__=1 \
			  -D__i386__=0
endif
endif

export compiler_flags

export cuda_compiler_flags

export asm_compiler_flags:=-x assembler-with-cpp

ifeq ($(target),Darwin)
linker_flags := -pipe \
	-lstdc++ \
	-Wl,-rpath,$(project_root)/$(lib_dir) \
	-Wl,-rpath,$(SDK_ENV_PATH)/lib64 \
	-Wl,-rpath,$(SDK_ENV_PATH)/lib32 \
	-Wl,-rpath,$(SDK_ENV_PATH)/lib \

else
linker_flags := -pipe \
	-lstdc++ \
	-static-libgcc \
	-Wl,-rpath=$(project_root)/$(lib_dir) \
	-Wl,-rpath=$(SDK_ENV_PATH)/lib64 \
	-Wl,-rpath=$(SDK_ENV_PATH)/lib32 \
	-Wl,-rpath=$(SDK_ENV_PATH)/lib \

endif

ifeq ($(target),sh4)
linker_flags += \
	-L$(SH4_TOOLCHAIN_PATH)/sh4-kreatv-linux-uclibc/lib \
	-lgcc_s
endif



ifeq ($(words $(coverage_projects)),0)
coverage_projects := $(strip $(notdir $(projects)))
else
coverage_projects := $(strip $(notdir $(coverage_projects)))
endif

ifeq ($(strip $(use_gcov)), yes)
compiler_flags += --coverage
linker_flags += --coverage
endif

#linker_flags += -Xlinker -R./lib:$(PWD)/lib:$(install_path)/lib:/usr/local/lib:$(additional_rpaths)

export linker_flags

export archiver_flags:=crs

export pc_path:=$(strip $(project_root))/$(lib_dir)/$(pc_dir)
export pc_path64:=$(strip $(project_root))/$(lib64_dir)/$(pc_dir)
export pc_path32:=$(strip $(project_root))/$(lib32_dir)/$(pc_dir)

export PKG_CONFIG_PATH:=$(pc_path):$(pc_path64):$(pc_path32):$(PKG_CONFIG_PATH)

# Commented the line below due to error
# tr: misaligned [:upper:] and/or [:lower:] construct
#project_env_root:=$(shell echo $(strip $(project_name))|tr [:lower:] [:upper:])
project_env_root:=$(shell echo $(strip $(project_name))|tr a-z A-Z)

project_env_root:=$(project_env_root)_ROOT
export project_env_root
export $(project_env_root):=$(project_root)

ifeq ($(shell uname),Darwin)
dynamic_lib_ext:=dylib
dynamic_lib_flag:=-dynamiclib
install_name:=-install_name @rpath
linker:=$(TOOLS_PREFIX)g++
striper_flags:=-Sx
else
ifneq ($(shell uname | grep -i cygwin),)
dynamic_lib_ext:=dll
dynamic_lib_flag:=-shared
linker:=$(TOOLS_PREFIX)gcc
striper_flags:=-s
else
ifneq ($(shell uname | grep -i mingw),)
dynamic_lib_ext:=dll
dynamic_lib_flag:=-shared
linker:=$(TOOLS_PREFIX)gcc
striper_flags:=-s
endif
endif
endif
