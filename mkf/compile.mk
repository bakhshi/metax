
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
$(call check_variable,obj_dir)
$(call check_variable,inc_dir)
$(call check_variable,compiler)
$(call check_variable,compiler_flags)
$(call check_variable,cpp_files)
$(call check_variable,asm_files)
$(call check_variable,moc)
$(call check_variable,cuda_compiler)
$(call check_variable,cuda_compiler_flags)
$(call check_variable,src_dir)
$(call check_variable,pkg_dir)

lib_name:=$(notdir $(shell pwd))

pkg_cflags_file := $(project_root)/$(pkg_dir)/$(lib_name)/pkg_cflags.mk
project_makefile := $(project_root)/$(src_dir)/$(lib_name)/makefile

-include $(pkg_cflags_file)

ifeq ($(target),Darwin)
	export sw_version :=$(shell sw_vers | grep ProductVersion | awk -F' ' '{print $$2}')
	export mac_sdk_inc_path_pre := /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX
	export major :=$(shell echo $(sw_version) | awk -F'.' '{print $$1}')
	export minor :=$(shell echo $(sw_version) | awk -F'.' '{print $$2}')
	export gcc_version := $(shell gcc --version | head -n 1 | awk '{print $$3}')
	export mac_sdk_inc_path := $(mac_sdk_inc_path_pre)$(major).$(minor).sdk
	cflags += -I$(mac_sdk_inc_path)/usr/include -I$(mac_sdk_inc_path)/usr/include/c++/$(gcc_version)
else
ifeq ($(target),sh4)
	toolchain_headers := $(SH4_TOOLCHAIN_PATH)/sh4-kreatv-linux-uclibc/sys-root/usr/include/
ifeq ("$(wildcard $(toolchain_headers))","")
$(warning toolchain headers by the path following path not found - $(toolchain_headers) )
endif
	cflags += -I$(toolchain_headers)
endif
endif

objs_path:=$(project_root)/$(obj_dir)/$(lib_name)

obj_files:=$(patsubst %.cpp,$(objs_path)/%.o,$(cpp_files))
moc_files:=$(patsubst %.hpp,$(objs_path)/moc_%.cpp,$(qt_headers))
rcc_files:=$(patsubst %.qrc,$(objs_path)/qrc_%.cpp,$(qrc_files))
obj_files+=$(patsubst %.S,$(objs_path)/%.o,$(asm_files))
obj_files+=$(subst .cpp,.o,$(moc_files))
obj_files+=$(subst .cpp,.o,$(rcc_files))
obj_files+=$(subst .cu,.o,$(cuda_files))

inc_path:=$(project_root)/$(inc_dir)
headers_path:=$(inc_path)/$(lib_name)

ifneq ($(words $(public_headers)),0)
public_headers:=$(addprefix $(headers_path)/,$(public_headers))
endif

ifneq ($(words $(qt_ui_files)),0)
	ui_headers:=$(patsubst %.ui,$(headers_path)/ui_%.hpp,$(qt_ui_files))
	cflags += -I$(headers_path)
endif


dep_files:=$(addprefix $(objs_path)/,$(addsuffix .d,$(basename $(cpp_files))))

ifneq ($(words $(dep_files)),0)
-include $(dep_files)
endif

.PHONY: depends
depends:
	for i in $(dep_files); \
	do \
		test -f $$i; \
		if [ $$? -eq 0 ]; \
		then \
			r=`sed -e 's/\\\//g' -e 's/://g' $$i`; \
			for j in $$r; \
			do \
				test -f $$j; \
				if [ $$? != 0 ]; \
				then \
					echo "RM      `basename $$i`"; \
					rm -f $$i; \
					break; \
				fi; \
			done; \
		fi; \
	done

.PHONY: compile
compile: $(rcc_files) $(ui_headers) $(public_headers) $(obj_files)

$(objs_path)/%.o: %.cpp
	$(echo) "CC	$<"
	$(compiler) -c $(compiler_flags) $(cflags)  $< -o $@

$(objs_path)/%.o: $(objs_path)/%.cpp
	$(echo) "CC	$(subst $(objs_path)/,,$<)"
	$(compiler) -c $(compiler_flags) $(cflags) $< -o $@

$(objs_path)/%.o: %.cu
	$(echo) "NC	$<"
	$(cuda_compiler) -c $(cuda_compiler_flags) $(cflags) $< -o $@

$(objs_path)/%.o: %.S
	$(echo) "AS	$<"
	$(asm_compiler) -c $(asm_compiler_flags) $< -o $@

$(headers_path)/%.txx: %.txx
	$(echo) "LN	$<"
	$(ln) -sf ../../$(src_dir)/$(lib_name)/$< $@

$(headers_path)/%.tpp: %.tpp
	$(echo) "LN	$<"
	$(ln) -sf ../../$(src_dir)/$(lib_name)/$< $@

$(headers_path)/%.hpp: %.hpp
	$(echo) "LN	$<"
	$(ln) -sf ../../$(src_dir)/$(lib_name)/$< $@

$(headers_path)/%.h: %.h
	$(echo) "LN	$<"
	$(ln) -sf ../../$(src_dir)/$(lib_name)/$< $@

$(headers_path)/%.hh: %.hh
	$(echo) "LN	$<"
	$(ln) -sf ../../$(src_dir)/$(lib_name)/$< $@

$(headers_path)/%.hxx: %.hxx
	$(echo) "LN	$<"
	$(ln) -sf ../../$(src_dir)/$(lib_name)/$< $@

$(objs_path)/%.d: %.cpp $(pkg_cflags_file)
	$(echo) "GD	$<"
	$(compiler) $(compiler_flags) -I$(inc_path) $(cflags) -MM $< -MF $@ -MT '$@ $(subst .d,.o,$@)'

$(objs_path)/moc_%.cpp: %.hpp
	$(echo) "MOC	$<"
	$(moc) $(moc_flags) $< -o $@

$(headers_path)/ui_%.hpp: %.ui
	$(echo) "UIC	$<"
	$(uic) $< -o $@

$(objs_path)/qrc_%.cpp: %.qrc
	$(echo) "RCC	$<"
	$(rcc) $< -o $@

$(pkg_cflags_file) : $(project_makefile)
	$(echo) "GCF	$(lib_name)"
	$(echo) -n "cflags += " > $@
ifneq ($(words $(libs)),0)
	$(eval cflags += $(shell pkg-config --cflags $(libs)))
	$(echo) -n "$(cflags)" >> $@
endif

