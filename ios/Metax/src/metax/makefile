#
# Copyright (c) 2015-2019 Leviathan CJSC
#  Email: info@instigatedesign.com
#  134/1 Tsarav Aghbyur St.,
#  Yerevan, 0052, Armenia
#  Tel:  +1-408-625-7509
#        +49-893-8157-1771
#        +374-60-464700
#        +374-10-248411 
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 3 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, see <http://www.gnu.org/licenses/>.
#

# The source files under the project.
cpp_files := \
	kernel.cpp \
	key_manager.cpp \
	storage.cpp \
	wallet.cpp \
	configuration_manager.cpp \
	link.cpp \
	socket_reactor.cpp \
	link_peer.cpp \
	router.cpp \
	hop.cpp \
	backup.cpp \
	user_manager.cpp \
	ssl_password_handler.cpp \

# The header files under the project, which are supposed to be used by other
# projects.
public_headers := \
	kernel.hpp \
	key_manager.hpp \
	storage.hpp \
	wallet.hpp \
	configuration_manager.hpp \
	link.hpp \
	socket_reactor.hpp \
	link_peer.hpp \
	router.hpp \
	hop.hpp \
	protocols.hpp \
	backup.hpp \
	user_manager.hpp \

# Building QT project.
# TODO: Add detailed comments.
qt_headers :=

moc_flags :=

qt_ui_files :=

uic_flags :=

# The compiler flags can be updated/overwritten for the project.
# 
# Example:
# compiler_flags += -s -Wno-long-long -fno-strict-aliasing
# compiler_flags := $(filter-out -Wshadow -Wconversion, $(compiler_flags))
# 
# Note: It is not recommended to overwrite the value (don't use := operator).
# 

compiler_flags += \
	-Wno-sign-conversion \
	-Wno-conversion \
	-Wno-float-equal \
	-Wno-unused-parameter \
	-Wno-cast-align \
	-Wno-sign-compare
ifeq ($(target),sh4)
compiler_flags += \
	  -Wno-conversion \
	  -Wno-sign-compare \
	  -Wno-shadow \
	  -Wno-float-equal \
	  -Wno-unused-parameter \
	  -Wno-cast-align \
	  -Wno-extra
endif

cflags += \
	-I$(POCO_PATH)/include \
	-I$(OPENSSL_PATH)/include \

# The linker flags can be updated/overwritten for the project
#
# Example:
# linker_flags += -i 
#
# Note: It is not recommended to overwrite the value (don't use := operator).
#

poco_libs := \
	PocoNetSSL \
	PocoNet \
	PocoCrypto \
	PocoJSON \
	PocoUtil \
	PocoXML \
	PocoFoundation

ssl_libs := \
	ssl \
	crypto

ifeq ($(strip $(build_type)),debug)
poco_libs := $(addsuffix d,$(poco_libs))
endif
ifeq ($(strip $(link_type)),static)
ifeq ($(shell uname),Linux)
poco_libs := $(addprefix -l:lib,$(addsuffix .a,$(poco_libs)))
ssl_libs := $(addprefix -l:lib,$(addsuffix .a,$(ssl_libs)))
else
poco_libs := $(addprefix $(POCO_PATH)/lib/lib,$(addsuffix .a,$(poco_libs)))
ssl_libs := $(addprefix $(OPENSSL_PATH)/lib/lib,$(addsuffix .a,$(ssl_libs)))
endif
else
poco_libs := $(addprefix -l,$(poco_libs))
ssl_libs := $(addprefix -l,$(ssl_libs))
endif

linker_flags += \
	-L$(POCO_PATH)/lib \
	-L$(OPENSSL_PATH)/lib \
	$(poco_libs) \
	$(ssl_libs) \

# The libraries which should be linked to the current project. The link type is
# specified as a global parameter, except the libraries, which are built as a
# either shared or static library. For those libraries the appropriate linking
# type is selected, e.g. if current project is being linked to lib_project,
# which is static library, then current project will be always linked to
# lib_project statically, despite global link type is shared or static.
# 
# Example:
# libs := project1 gtkmm-2.4 
# 
# Note: Specifying the library version is not supported.
#
libs := \
	platform \

# Library version
#
# Note: If the library version is not specified, then the program_version is
# taken for this library.
#
lib_version := \

# For each project either executable file or a library can be generated.
#
# To generate the executable file the exe.mk should be included, which is
# located in mkf directory.
#
# Two types of libraries can be generated for each project:
#       dynamic (shared objects) and static (archives).
#
# To generate the shared object the dynamic.mk should be included here.
# To generate the archive the static.mk file should be included here.
# To generate either (shared object or archive) depending on the global link
# type the lib.mk should be included.
#
# Note that the generated library will be located in local $lib_dir directory
# (by default $(project_root)/lib/).
#

include $(mkf_path)/lib.mk
