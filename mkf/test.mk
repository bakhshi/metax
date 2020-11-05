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

ifndef test_type
$(error test_type is not defined)
endif

ifndef acceptance_tests_dir
$(error acceptance_tests_dir is not defined)
endif

ifndef black_box_tests_dir
$(error black_box_tests_dir is not defined)
endif

ifndef regression_tests_dir
$(error regression_tests_dir is not defined)
endif

ifndef continuous_tests_dir
$(error continuous_tests_dir is not defined)
endif

ifndef acceptance_tests_results_dir
$(error acceptance_tests_results_dir is not defined)
endif

ifndef black_box_tests_results_dir
$(error black_box_tests_results_dir is not defined)
endif

ifndef regression_tests_results_dir
$(error regression_tests_results_dir is not defined)
endif

ifndef test_result
$(error test_result is not defined)
endif

ifeq ($(use_gcov),yes)
$(call check_variable,coverage_result)
$(call check_variable,coverage_result_dir)

ifndef coverage_projects
$(error coverage_projects is not defined)
endif

coverage_log := $(strip $(coverage_result_dir)/log 2>&1)

else

ifneq ($(MAKECMDGOALS),test)
$(info The flag use_gcov is set to 'no'.)
$(info Run 'make setup use_gcov=yes' to be able to collect coverage data.)
endif

endif

acceptance_sub_dirs := $(wildcard $(acceptance_tests_dir)/*)
acceptance_sub_results_dirs := $(subst $(acceptance_tests_dir),\
		$(acceptance_tests_results_dir),$(acceptance_sub_dirs))
black_box_sub_dirs := $(wildcard $(black_box_tests_dir)/*)
black_box_sub_results_dirs := $(subst $(black_box_tests_dir),\
		$(black_box_tests_results_dir),$(black_box_sub_dirs))
regression_sub_dirs := $(wildcard $(regression_tests_dir)/*)
regression_sub_results_dirs := $(subst $(regression_tests_dir),\
		$(regression_tests_results_dir),$(regression_sub_dirs))
continuous_sub_dirs := $(wildcard $(continuous_tests_dir)/*)
continuous_sub_results_dirs := $(subst $(continuous_tests_dir),\
		$(continuous_tests_results_dir),$(continuous_sub_dirs))

ifeq ($(test_type),acceptance_tests)
test_sub_dirs := $(acceptance_sub_dirs)
test_sub_results_dirs := $(acceptance_sub_results_dirs)
else
ifeq ($(test_type),black_box_tests)
test_sub_dirs := $(black_box_sub_dirs)
test_sub_results_dirs := $(black_box_sub_results_dirs)
else
ifeq ($(test_type),regression_tests)
test_sub_dirs := $(regression_sub_dirs)
test_sub_results_dirs := $(regression_sub_results_dirs)
else
ifeq ($(test_type),continuous_tests)
test_sub_dirs := $(continuous_sub_dirs)
test_sub_results_dirs := $(continuous_sub_results_dirs)
else
ifeq ($(test_type),all)
test_sub_dirs := $(acceptance_sub_dirs) \
		 $(black_box_sub_dirs) \
		 $(regression_sub_dirs) \
		 $(continuous_sub_dirs)
test_sub_results_dirs := $(acceptance_sub_results_dirs) \
			 $(black_box_sub_results_dirs) \
			 $(regression_sub_results_dirs)\
			 $(continuous_sub_results_dirs)
endif
endif
endif
endif
endif


ifeq ($(use_gcov),yes)

sorted_coverage_projects = $(sort $(coverage_projects))
all_ic_info = $(foreach t,$(sorted_coverage_projects),$(coverage_result_dir)/$t/ic.info)
all_gcno = $(foreach t,$(sorted_coverage_projects),$(wildcard $(project_root)/$(obj_dir)/$t/*.gcno))
all_gcda = $(foreach i,$(all_gcno),$(i:.gcno=.gcda))
gcda_dirs = $(strip $(notdir $(foreach t,$(sort $(dir $(all_gcda))),$(shell echo $(t)|sed s/.$$//;))))

get_gcda=$(filter $(project_root)/$(obj_dir)/$(strip $1)/%,$(all_gcda))
coverage: $(coverage_result)

$(coverage_result): $(all_ic_info)
	@$(echo) "" > $@
	@$(echo) "" >> $@
	@$(echo) " PROJECT:" >> $@
	@$(echo) "         Path - $(PWD)" >> $@
ifdef project_name
	@$(echo) "         Name - $(project_name)" >> $@
else
	@$(echo) "         Name - NONE" >> $@
endif

ifdef project_version
	@$(echo) "         Version - $(project_version)" >> $@
else
	@$(echo) "         Version - NONE" >> $@
endif
	@$(echo) "         Repository version - "`if [ -d .svn ]; then svn info 2> /dev/null | grep Revision | sed 's/Revision:\ //g'; else git rev-parse HEAD; fi` >> $@
	@$(echo) "" >> $@
	@$(echo) " PLATFORM:" >> $@
	@$(echo) "         Hostname - "`uname -n` >> $@
	@$(echo) "         Architecture - "`uname -m` >> $@
ifeq ($(shell uname),Darwin)
	@$(echo) "         OS - "`sw_vers | head -n 2 | cut -f2 | tr '\n' ' '` >> $@
else
	@$(echo) "         OS - "`cat /etc/issue` >> $@
endif
	@$(echo) "         Compiler - "`gcc --version | grep "gcc" -i` >> $@
	@$(echo) "         make version - "`make --version | grep "Make" -i` >> $@
	@$(echo) "" >> $@
	@$(echo) " COVERAGE SUMMARY:" >> $@
	@j=0;\
	echo -n Collecting overall coverage ..;\
	for i in $(gcda_dirs); do\
		if [ -f $(coverage_result_dir)/$$i/ic.info ]; then\
			if [ -s $(coverage_result_dir)/$$i/ic.info ]; then\
				str="-a $(coverage_result_dir)/$$i/ic.info";\
				srclist="$${srclist}$$str ";\
	                	j=`expr $$j + 1`;\
	            	fi;\
	      	fi;\
	done;\
	lcov $$srclist -o $(coverage_result_dir)/overall/overall.info | tail -3 >> $@;\
	genhtml -o $(coverage_result_dir)/overall\
		$(coverage_result_dir)/overall/overall.info >> $(coverage_log);\
	echo . done;


.SECONDEXPANSION:
$(coverage_result_dir)/%/ic.info: $$(call get_gcda, %) 
	@i=$(*F);\
	dir=$(project_root)/$(obj_dir)/$$i; \
	echo -n Collecting coverage of project $$i ..; \
	y=$(coverage_result_dir)/$$i;\
	cd src/$$i >> $(coverage_log);\
	lcov --directory ../../$(obj_dir)/$$i/ -b . --capture \
		--output-file $$y/ic.info >> $(coverage_log);\
	lcov --extract $$y/ic.info "*/src/$$i/*" \
		--output-file $$y/ic.info >> $(coverage_log);\
	genhtml -o $$y $$y/ic.info >> $(coverage_log);\
	cd - > /dev/null;\
	echo . done;

$(all_gcda): $(all_gcno) $(test_result) $(gcda_dirs)
	@if [ ! -f $@ ]; then \
		echo "Generated empty file $@" >> $(coverage_log);\
		touch $@;\
	fi;

$(gcda_dirs): $(coverage_result_dir)
	@mkdir -p $</$@

$(coverage_result_dir):
	@mkdir -p $@
	@mkdir -p $@/overall;
	@$(echo) Collecting coverage information ...;
	@>$(coverage_log);

else
coverage:
endif

sub_tests_results := $(addsuffix /result.txt, $(test_sub_results_dirs))
.PHONY: test
test: check_test_executables $(test_result)

get_number_of = $(shell grep -r $(1) $(shell ls $(sub_tests_results) 2> \
		/dev/null) | wc -l)

# TODO: The way number of total run tests is calculated in general is not
# correct because we may define only subset of tests to be run and thus the
# total number will be decreased and will not be equal to the number of
# files in $(test_sub_dirs)
$(test_result): $(sub_tests_results)
ifneq ($(words $(test_sub_results_dirs)),0)
	@$(echo) "" > $@
	@$(echo) " TOP LEVEL TEST RESULTS" >> $@
	@$(echo) "" >> $@
	@$(echo) " PROJECT:" >> $@
	@$(echo) "         Path - $(PWD)" >> $@
ifdef project_name
	@$(echo) "         Name - $(project_name)" >> $@
else
	@$(echo) "         Name - NONE" >> $@
endif
ifdef project_version
	@$(echo) "         Version - $(project_version)" >> $@
else
	@$(echo) "         Version - NONE" >> $@
endif
	@$(echo) "         Repository version - "`if [ -d .svn ]; then svn info 2> /dev/null | grep Revision | sed 's/Revision:\ //g'; else git rev-parse HEAD; fi` >> $@
	@$(echo) "" >> $@
	@$(echo) " PLATFORM:" >> $@
	@$(echo) "         Hostname - "`uname -n` >> $@
	@$(echo) "         Architecture - "`uname -m` >> $@
ifeq ($(shell uname),Darwin)
	@$(echo) "         OS - "`sw_vers | head -n 2 | cut -f2 | tr '\n' ' '` >> $@
else
	@$(echo) "         OS - "`cat /etc/issue` >> $@
endif
	@$(echo) "         Compiler - "`gcc --version | head -n 1` >> $@
	@$(echo) "         make version - "`make --version | grep "Make" -i` >> $@
	@$(echo) "" >> $@
	@$(echo) " SUMMARY:" >> $@
	@$(echo) "         Total -" `expr $(call get_number_of, PASS) + \
	$(call get_number_of, FAIL) + $(call get_number_of, "Expected Fail") + \
	$(call get_number_of, "Unexpected Pass") + \
	$(call get_number_of, SKIP) + $(call get_number_of, CRASH)` >> $@
	@$(echo) "         Passed -" $(call get_number_of, PASS) >> $@
	@$(echo) "         Failed -" $(call get_number_of, FAIL) >> $@
	@$(echo) "         Expected fails -" $(call get_number_of, \
		"Expected Fail") >> $@
	@$(echo) "         Unexpected passes -" $(call get_number_of,  \
		"Unexpected Pass") >> $@
	@$(echo) "         Skipped -" $(call get_number_of, SKIP) >> $@
	@$(echo) "         Crash -" $(call get_number_of, CRASH) >> $@
	@$(echo) "" >> $@
	@$(echo) " LEGEND:" >> $@
	@$(echo) "       Total - total number of all test-cases examined." >> $@
	@$(echo) "" >> $@
	@$(echo) "       Passed - number of test-cases which were executed \
	and succeeded." >> $@
	@$(echo) "" >> $@
	@$(echo) "       Failed - number of test-cases which were executed \
	but failed." >> $@
	@$(echo) "" >> $@
	@$(echo) "       Expected fails - number of test-cases which were \
	executed and failed," >> $@
	@$(echo) "               but are expected to fail due to known bugs." >> $@
	@$(echo) "" >> $@
	@$(echo) "       Unexpected pass - number of test-cases \
	which were executed and succeeded," >> $@
	@$(echo) "               although was expected to fail \
	due to known bug." >> $@
	@$(echo) "" >> $@
	@$(echo) "       Skipped - number of test-cases which are marked \
	by developer to be" >> $@
	@$(echo) "               skipped for some reason \
	(e.g. to distinguish from bugs/fails)."  >> $@
	@$(echo) "               These test-cases are not executed." >> $@
	@$(echo) "" >> $@
	@$(echo) "       Crashed - number of test-cases which were executed," >> $@
	@$(echo) "               but execution was interrupted \
	due to a tool failure" >> $@
	@$(echo) "               (e.g. coredump) or some other external reason " \
		>> $@
	@$(echo) "               (e.g.  environment problem, disk full, etc.)." \
		>> $@
	@$(echo) "" >> $@
	@$(echo) "RESULTS BY TEST SERIES:" >> $@
	@$(echo) "" >> $@
	@for i in `ls $(sub_tests_results) 2> /dev/null`; do \
		$(echo) -n $$i | sed 's/\/result.txt//' | sed 's/.*\///' >> $@; \
		$(echo) " - " `cat $$i`>> $@; \
		$(echo) "" >> $@; \
		$(echo) "---------------------------------------" >> $@; \
		$(echo) "" >> $@; \
	done
endif

remove_suffix=$(subst _$(result_suffix),,$1)

.PHONY: $(sub_tests_results)
$(sub_tests_results):
	@mkdir -p -m 755 $(dir $@)
	@$(MAKE) -C $(call remove_suffix,$(@D))

include $(mkf_path)/preconditions.mk
