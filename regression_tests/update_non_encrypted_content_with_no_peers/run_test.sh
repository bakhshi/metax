#!/bin/bash

# Test case flow: 
# - Runs one instance of metax.
# - Sends "node" save request for some content
# - Sends "node" update request for received uuid with enc=0
# - Sends get request for received uuid.
# - Checks the compliance of get/updated contents.



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
        update_content="Test message for content update test"
        res=$(curl --data "$update_content" http://localhost:7081/db/save/node?id=$uuid\&enc=0)
        uuid1=$(echo $res | cut -d'"' -f 4)
        if [ "$uuid" == "$uuid1" ]; then
                get_content=$(curl http://localhost:7081/db/get/?id=$uuid1)
                echo "GET content $get_content"
                if [ "$update_content" == "$get_content" ]; then
                        echo $PASS > $final_test_result
                fi
        fi
        kill_metax $p
}

main $@
