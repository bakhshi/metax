
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

.PHONY: check
check: check_executables check_libraries

.PHONY: check_executables
check_executables: check_libraries
	@$(echo) "Checking executable preconditions:"
	@for i in $(preconditions); \
	do \
		$(echo) -n "Looking for $${i}: ";  \
		if [ -n "`which $${i} 2>/dev/null`" ]; \
		then \
			$(echo) "found `which $${i}`"; \
		else \
			$(echo) "not found"; \
			exit 255; \
		fi; \
	done

.PHONY: check_libraries
check_libraries:
	@$(echo) "Checking library preconditions:"
	@for j in $(library_preconditions); \
	do \
		$(echo) -n "Looking for $${j} library: "; \
		pkg-config --exists $${j}; \
		if [ $$? -eq 0 ] || [ -n "`ls \
				/lib/lib$${j}.* \
				/usr/lib/lib$${j}.* \
				/usr/local/lib/lib$${j}.* \
				/usr/lib/i386-linux-gnu/lib$${j}.* \
				/usr/lib/x86_64-linux-gnu/lib$${j}.* \
				/lib64/lib$${j}.* \
				/usr/lib64/lib$${j}.* \
				/usr/local/lib64/lib$${j}.* \
				/opt/tpt/lib/lib$${j}.* \
				/opt/tpt/lib64/lib$${j}.* \
				$(SDK_ENV_PATH)/lib/lib$${j}.* \
				$(SDK_ENV_PATH)/lib64/lib$${j}.* \
				2> /dev/null`" ]; \
		then \
			$(echo) "found"; \
		else \
			$(echo) "not found"; \
			exit 255; \
		fi; \
	done

.PHONY: check_test_executables
check_test_executables:
	@$(echo) "Checking test preconditions:"
	@for i in $(test_preconditions); \
	do \
		$(echo) -n "Looking for $${i}: ";  \
		if [ -n "`which $${i} 2>/dev/null`" ]; \
		then \
			$(echo) "found `which $${i}`"; \
		else \
			$(echo) "not found"; \
			exit 255; \
		fi; \
	done
