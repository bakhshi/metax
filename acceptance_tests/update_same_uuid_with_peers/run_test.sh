#!/bin/bash

# - TODO
# Very often this test fails with the assertion failure, because of the multiple
# peers (bug #3389). After fixing that bug metax will hang (bug #3377).

# Test case flow:
# - Runs an instance of metax with config.xml configuration.
# - Sends "save" request for getting a valid uuid.
# - Sends multiple "update" requests with valid existing uuid.
# - Checks the compliance of "save"/"update" request's messages.


source ../../test_utils/functions.sh

if [ "yes" == "$use_valgrind" ]; then
        COUNT=10
        TIME=30
else
        COUNT=500
fi


# wait command exit code when metax was successfully finished (i.e. without Seg. fault or Assertion)
EXIT_CODE_OK=0




main()
{
        $EXECUTE -f $tst_src_dir/config.xml &
        p=$!
        is_peer_connected=$(peers_connect 7081 $tst_src_dir/config.xml)
        if [ "0" != "$is_peer_connected" ];then
                echo "7080 does not connect to peers"
                kill_metax $p
                exit;
        fi
        echo -n $p > $test_dir/processes_ids
        save=$(curl --data "save content" http://localhost:7081/db/save/node)
        uuid=$(echo $save | cut -d'"' -f 4)
        for ((i=1; i<=$COUNT; ++i))
        do
            update=$({ time curl -m $TIME --data "updated content" http://localhost:7081/db/save/node?id=$uuid; } 2> $test_dir/time_log)
            update_time=$(cat $test_dir/time_log | grep real | awk '{print $2}')
            echo $i $update $update_time
            if [ "$save" != "$update" ]; then
                    echo "Failed updating $i time"
                    echo $FAIL > $final_test_result
                    kill_metax $p
                    exit
            fi
        done

        echo $PASS > $final_test_result
        kill_metax $p
}

main $@

# vim:et:tabstop=8:shiftwidth=8:cindent:fo=croq:textwidth=80:
