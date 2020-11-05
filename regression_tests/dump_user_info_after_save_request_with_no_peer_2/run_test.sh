#!/bin/bash

# Test case flow: 
# - Runs one instance of metax.
# - Sends "node" save request for some content (default it is saved as encrypted).
# - Sends "get" request for received uuid.
# - Checks the compliance of received/saved content.

save_content="Some content for test"


source ../../test_utils/functions.sh

# wait command exit code when metax was successfully finished (i.e. without Seg. fault or Assertion)
EXIT_CODE_OK=0



main()
{
        $EXECUTE -f $tst_src_dir/config.xml &
        p=$!
        wait_for_init 7081
        echo -n $p > $test_dir/processes_ids

        for i in {1..10}; do
                for i in {1..3}; do
                        uuid=$(curl --data "$save_content" http://localhost:7081/db/save/node)
                        uuid=$(echo $uuid | cut -d'"' -f 4)
                        get_content=$(curl http://localhost:7081/db/get/?id=$uuid)
                        if [ "$save_content" != "$get_content" ]; then
                                echo $FAIL > $final_test_result
                                kill $p
                                exit;
                        fi
                done
                res=$(curl http://localhost:7081/db/dump_user_info)
                expected='{"status":"succeeded"}'
                if [ "$res" != "$expected" ]; then
                        echo $FAIL > $final_test_result
                        kill $p
                        exit;
                fi
        done
        echo $PASS > $final_test_result
        kill_metax $p
}

main $@
