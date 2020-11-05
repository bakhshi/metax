
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

.DEFAULT_GOAL := default

ifeq ($(filter help,$(MAKECMDGOALS)),help)
	include $(mkf_path)/help.mk
else

ifeq ($(filter clean,$(MAKECMDGOALS)),clean)
	include $(mkf_path)/clean.mk
else

ifeq ($(filter clean_coverage,$(MAKECMDGOALS)),clean_coverage)
	include $(mkf_path)/clean.mk
else

ifeq ($(filter clean_docs,$(MAKECMDGOALS)),clean_docs)
	include $(mkf_path)/clean.mk
else

ifeq ($(filter clean_test,$(MAKECMDGOALS)),clean_test)
	include $(mkf_path)/clean.mk
else

ifeq ($(filter clean_conf,$(MAKECMDGOALS)),clean_conf)
	include $(mkf_path)/clean.mk
else

ifeq ($(filter test,$(MAKECMDGOALS)),test)
	include $(mkf_path)/test.mk
else

ifeq ($(filter coverage,$(MAKECMDGOALS)),coverage)
	include $(mkf_path)/test.mk
else

ifeq ($(filter install,$(MAKECMDGOALS)),install)
	include $(mkf_path)/install.mk
	include $(mkf_path)/default.mk
	include $(mkf_path)/clean.mk
else

ifeq ($(filter all,$(MAKECMDGOALS)),all)
	include $(mkf_path)/clean.mk
	include $(mkf_path)/default.mk
else

ifeq ($(filter docs,$(MAKECMDGOALS)),docs)
	include $(mkf_path)/clean.mk
else

ifeq ($(filter user_docs,$(MAKECMDGOALS)),user_docs)
	include $(mkf_path)/clean.mk
	include $(mkf_path)/default.mk
else

ifeq ($(filter tutorials_docs,$(MAKECMDGOALS)),tutorials_docs)
	include $(mkf_path)/clean.mk
else

ifeq ($(filter developer_docs,$(MAKECMDGOALS)),developer_docs)
	include $(mkf_path)/clean.mk
else
	include $(mkf_path)/default.mk
endif
endif
endif
endif
endif
endif
endif
endif
endif
endif
endif
endif
endif
endif
