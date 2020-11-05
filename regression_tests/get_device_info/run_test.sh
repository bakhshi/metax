#!/bin/bash

# Test case flow:
# - Runs one instance of Metax
# - Sends get_metax_info request and checks that device_info_uuid is empty
# - Copies metax_device_info.json with hard-coded device_info_uuid from test source directory to test directory
# - Sends get_metax_info request
# - Kills Metax
# - Checks that device_info_uuid is not empty

source ../../test_utils/functions.sh

# wait command exit code when metax was successfully finished (i.e. without Seg. fault or Assertion)
EXIT_CODE_OK=0


main()
{
	$EXECUTE -f $tst_src_dir/config.xml &
	p=$!
        wait_for_init 7081
        echo -n $p > $test_dir/processes_ids

        res=$(curl http://localhost:7081/config/get_metax_info)
        echo "get_metax_info request response is $res"
        get_device_info_uuid=$(echo $res | jq -r '.device_info_uuid')
        if [ -n "$get_device_info_uuid" ]; then
                echo "device_info_uuid should be empty"
                kill_metax $p
                exit
        fi

        cp $tst_src_dir/metax_device_info.json $test_dir
        res=$(curl http://localhost:7081/config/get_metax_info)
        kill_metax $p

        echo "get_metax_info request response is $res"

        get_device_info_uuid=$(echo $res | jq -r '.device_info_uuid')
        if [ -z "$get_device_info_uuid" ]; then
                echo "Failed to get device info uuid"
                exit
        fi

        echo $PASS > $final_test_result
}

main $@
