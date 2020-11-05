
SHELL := bash

export tst_src_dir := $(shell pwd)

project_name := $(notdir $(tst_src_dir))

export test_dir := $(test_result_dir)/$(project_name)

export final_test_result := $(test_dir)/result.txt

export shell_bin := $(project_root)/$(bin_dir)/metax_web_api

export use_valgrind

ifeq ($(target),Darwin)
export SEDIP := -i ''
export TIMEOUT := gtimeout
else
export SEDIP := -i
export TIMEOUT := timeout
endif

# 300s timeout is fixed while there are tests which wait for metax tender expiration - occuring after 4.5 minutes
ifeq ($(use_valgrind),yes)
export TIME := 20
export EXECUTE := $(TIMEOUT) -s SIGKILL 600 valgrind --leak-check=full --log-file=$(test_dir)/valgrind_result.txt $(shell_bin)
else
export TIME := 10
ifeq ($(test_type), acceptance_tests)
export EXECUTE := $(TIMEOUT) -s SIGKILL 600 $(shell_bin)
else
export EXECUTE := $(TIMEOUT) -s SIGKILL 300 $(shell_bin)
endif
endif

export PASS := Unexpected Pass
export FAIL := Expected Fail
export SKIP := SKIP
export CRASH := CRASH

$(final_test_result) : $(shell_bin) $(tst_src_dir)/run_test.sh
	@echo $(FAIL) > $(final_test_result)
	@echo "Running $(shell basename $(tst_src_dir)) test case ........."
	@-cd $(test_dir) && (npm install ws &> npm_install_output) && $(tst_src_dir)/run_test.sh &> $(test_dir)/test_output
	@$(tst_src_dir)/killall_running_metax.sh >> $(test_dir)/test_output
	@echo "Finished $(shell basename $(tst_src_dir)) test case"

