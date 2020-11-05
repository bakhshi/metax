#!/bin/bash

# Test case flow: 
# - Runs one instance of metax.
# - Sends save request (node) with empty content
# - Sends get request for received uuid.
# - The reply will be empty, no crash should happen

get_file=$test_dir/get_file


source ../../test_utils/functions.sh

# wait command exit code when metax was successfully finished (i.e. without Seg. fault or Assertion)
EXIT_CODE_OK=0



main()
{
        $EXECUTE -f $tst_src_dir/config.xml &
        pid=$!
        wait_for_init 7081
        echo -n $pid > $test_dir/processes_ids

        uuid=$(curl --data "" "http://localhost:7081/db/save/node?enc=0")
        uuid=$(echo $uuid | cut -d'"' -f 4)
        curl http://localhost:7081/db/get/?id=$uuid > $get_file
        if [[ -f $get_file && ! -s $get_file ]]
        then
                echo $PASS > $final_test_result
        fi
        kill_metax $pid
}

main $@
