
SHELL := bash

export tst_src_dir := $(shell pwd)

project_name := $(notdir $(tst_src_dir))

export test_dir := $(test_result_dir)/$(project_name)

export final_test_result := $(test_dir)/result.txt

export shell_bin := $(project_root)/$(bin_dir)/metax_web_api

export SKIP := SKIP

$(final_test_result) : $(shell_bin)
	@echo $(SKIP) > $(final_test_result)
	@echo "Running $(shell basename $(tst_src_dir)) test case ........."
	@echo "Finished $(shell basename $(tst_src_dir)) test case"

