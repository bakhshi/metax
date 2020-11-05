#!/bin/bash

# Test case flow: 
# - Runs one instance of metax.
# - Sends "node" save request for some data with enc=0.
# - Removes master json file from storage.
# - Sends "node" update request for received uuid
# - Sends get request for received uuid.
# - Checks the compliance of received/updated datas.



source ../../test_utils/functions.sh

# wait command exit code when metax was successfully finished (i.e. without Seg. fault or Assertion)
EXIT_CODE_OK=0



main()
{
        $EXECUTE -f $tst_src_dir/config.xml &
        p=$!
        wait_for_init 7081
        echo -n $p > $test_dir/processes_ids

        save_content="Some save content"
        uuid=$(curl --data "$save_content" http://localhost:7081/db/save/node?enc=0)
        uuid=$(echo $uuid | cut -d'"' -f 4)
        rm $test_dir/storage/$uuid
        update_content="Test message for content update test"
        res=$(curl --data "$update_content" http://localhost:7081/db/save/node?id=$uuid\&enc=0)
        expected="{\"error\":\"Failed updating uuid=$uuid. UUID is not found.\"}"
        if [ "$expected" == "$res" ]; then
                echo $PASS > $final_test_result
        fi
        kill_metax $p
}

main $@
