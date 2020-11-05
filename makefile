#
# Copyright (c) 2005-2015 Instigate CJSC
#  Email: info@instigatedesign.com
#  58/1 Karapet Ulnetsi St.,
#  Yerevan, 0069, Armenia
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

# The name of the project
# 
project_name := Metax

# The version of the project
#
project_version := 1.2.7

# The absolute path to the directory where the current project is located 
#
export project_root := $(shell pwd)

# Install path prefix
#
prefix := $(HOME)/leviathan

export LEVIATHAN_ENV_VOSTANP2P_ROOT := $(PWD)

# Tools path
#
export moc :=

# Android SDK Path
#
export ANDROID_SDK_PATH :=

# The projects should be listed here
#
# Example:
# projects := src/project1 src/project2
#
projects :=  \
	src/platform \
	src/metax \
	src/metax_web_api \
	#src/platform_test \

#List of the projects to collect coverage data
coverage_projects := $(projects)

# Includes the tests to check the basic principles of the
# application.  Should be called after each modification to
# check the regression of application.
#regression_tests := 
#continuous_tests :=
#acceptance_tests := 

test_type := continuous_tests
test_result := test_results.txt

# Test prerequisite tools
ifeq ($(test_type), regression_tests)
test_preconditions := \
        sshpass \
        nodejs \
        npm \
        jq \
        curl \
        bc
else
test_preconditions := \
        nodejs \
        npm \
        jq \
        curl \
        bc
endif

# Prerequisite tools
preconditions := \
	gcc \
	g++ \
	ar \
	ln \
	pkg-config \
	#patchelf \
	lcov \
	doxygen \

# Prerequisite libraries
library_preconditions := \

# Defined the path to where the package will be installed
#
install_path := $(prefix)/$(project_name)/$(project_version)

# Directories which should be installed/packaged in debug mode
# 
export dbg_package := inc lib bin doc res src utils 

# Directories which should be installed/packaged in release mode
#
export opt_package := inc lib bin doc res mkf

export deb_package := lib doc

export deb_dev_package := inc lib doc Doxyfile 

# Name of package.  If name of the package is not defined 
# default value of this variable will be 
# "<program_name>_<project_version>_<link_type>_<build_type>"
# <link_type> and <build_type> will be taken from ./_setup_ file.
export package_name :=

# build_type must be either 'debug' or 'release'.
#export build_type ?= $(config_type)

# This part checks gcc version and then depending on result
# performs filter-out for some flags in low-level makefiles.
#ifneq (`gcc --version 2>&1 | grep "4.3"`,)
#export platform := $(shell cat /etc/issue)
#endif

export post_deb_actions := post_deb_actions

# The path to the mkf should be assigned to this variable
#
export mkf_path := $(PWD)/mkf

# Run time search path, used by patchELF tool while installing the project.
export RPATHS := 

include $(mkf_path)/main.mk
include $(mkf_path)/doc.mk

ifeq ($(target),Darwin)
export OPENSSL_PATH := /usr/local/opt/openssl
export POCO_PATH := /usr/local
else
ifeq ($(target),sh4)
export OPENSSL_PATH := /opt/openssl/openssl1.1.0g_sh4
export POCO_PATH := /opt/poco/poco1.8.1_sh4
endif
endif


# Additional values to be added to linker run time paths
additional_rpaths :=

########################### Debian package creation ############################

export section :=
export priority :=

ifeq ("$(shell uname -m)", "x86_64")
export architecture := "amd64"
else
export architecture := "i386"
endif

export essential :=
export depends := 
export pre_depends :=
export recommends :=
export suggests :=
export installed_size :=
export maintainer :=
export conflicts :=
export replaces :=
export provides :=
export description :=

ifneq ($(filter $(MAKECMDGOALS), deb_dev),)
export deb_package_content := $(deb_dev_package)
export application_name := $(application_name)-dev
else
export deb_package_content := $(deb_package)
endif

.PHONY : $(post_deb_actions)
$(post_deb_actions) :
	@chmod 755 debian/ -R
	@dpkg-deb -b debian debian
	@mv -f debian/*.deb ./
	@rm -rf debian


#export LD_LIBRARY_PATH := $(LEVIATHAN_ENV_VOSTANP2P_ROOT)/lib:$(LD_LIBRARY_PATH)

export PKG_CONFIG_PATH := $(LEVIATHAN_ENV_VOSTANP2P_ROOT)/lib/pkgconfig:$(LEVIATHAN_ENV_VOSTANP2P_ROOT)/utils:/usr/lib/pkgconfig:$(PKG_CONFIG_PATH)

############################# dependencies #####################################
src/metax : src/platform
src/metax_web_api : src/metax
#src/platform_test : src/platform

.PHONY: apk
apk:
	make -C android/Leviathan/

.PHONY: apk_install
apk_install:
	make -C android/Leviathan/ install

clean_all::
	make clean
	make -C android/Leviathan/ clean

