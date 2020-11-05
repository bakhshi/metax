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
$(call check_variable,tutor_dir)
$(call check_variable,dev_doc_dir)
$(call check_variable,mkf_path)
$(call check_variable,doc_dir)

utl_path:=$(project_root)/$(utl_dir)

doc_path:=$(project_root)/$(doc_dir)

.PHONY: all
all: default developer_docs user_docs 
	@cat doc/developer/doxygen.err > doc/doxygen.err

.PHONY: docs
docs: developer_docs user_docs 
	@cat doc/developer/doxygen.err > doc/doxygen.err

.PHONY: user_docs
ifneq ($(wildcard src/doc/tutorials),)
user_docs: clean_user_docs tutorials_docs
else
user_docs: clean_user_docs
endif
	@$(echo) -n "Generating user docs ... "
	@mkdir -p -m 775 $(manual_dir)
	@$(echo) "@INCLUDE=./Doxyfile" > Doxyfile_usr_docs
	@$(echo) "OUTPUT_DIRECTORY=doc/user/manual" >> Doxyfile_usr_docs
	@$(echo) "WARN_LOGFILE=doc/user/manual/doxygen.err" >> Doxyfile_usr_docs
	@$(echo) "@INCLUDE=$(wildcard ./src/doc/Doxyfile.user)" >> Doxyfile_usr_docs
	@doxygen Doxyfile_usr_docs >> doc/user/manual/doxygen.log 2>&1
	@rm -f Doxyfile_usr_docs
	@$(echo) "done"
	@$(echo) "For details please see the following files:"
	@$(echo) "	doc/user/manual/doxygen.log"
	@$(echo) "	doc/user/manual/doxygen.err"


.PHONY: tutorials_docs 
tutorials_docs: clean_user_docs
	@$(echo) -n "Generating user tutorials ... "
	@mkdir -p -m 775 $(tutor_dir)
	@$(echo) "@INCLUDE=./Doxyfile" > Doxyfile_tutor_docs
	@$(echo) "INPUT=src/doc/tutorials" >> Doxyfile_tutor_docs
	@$(echo) "OUTPUT_DIRECTORY=doc/user/tutorials" >> Doxyfile_tutor_docs
	@$(echo) "WARN_LOGFILE=doc/user/manual/doxygen.err" >> Doxyfile_tutor_docs
	@doxygen Doxyfile_tutor_docs >> doc/user/manual/doxygen.log 2>&1
	@rm -f Doxyfile_tutor_docs
	@$(echo) "done"
	@$(echo) "For details please see the following files:"
	@$(echo) "	doc/user/manual/doxygen.log"
	@$(echo) " 	doc/user/manual/doxygen.err"
 
.PHONY: developer_docs
developer_docs: clean_developer_docs
	@$(echo) -n "Generating developer docs ... "
	@mkdir -p -m 775 $(dev_doc_dir)
	@$(echo) "@INCLUDE=./Doxyfile" > Doxyfile_dev_docs
	@$(echo) "EXCLUDE=src/doc/user src/doc/tutorials" >> Doxyfile_dev_docs
	@$(echo) "OUTPUT_DIRECTORY=doc/developer" >> Doxyfile_dev_docs
	@$(echo) "WARN_LOGFILE=doc/developer/doxygen.err" >> Doxyfile_dev_docs
	@$(echo) "@INCLUDE=$(wildcard ./src/doc/Doxyfile.developer)" \
		>> Doxyfile_dev_docs
	@doxygen Doxyfile_dev_docs >> doc/developer/doxygen.log 2>&1
	@rm -f Doxyfile_dev_docs
	@$(echo) "done"
	@$(echo) "For details please see the following files:"
	@$(echo) "	doc/developer/doxygen.log"
	@$(echo) "	doc/developer/doxygen.err"

