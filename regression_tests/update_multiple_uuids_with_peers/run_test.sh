#!/bin/bash

# TODO Now this test fails with assertion because of the multiple peers.
# This test hangs during sanding multiple update requests. Bug #3377.

# Test case flow:
# - Runs an instance of metax with config.xml configuration with several peers.
# - Sends multiple "save" requests for getting valid uuids and saved that uuids
#   in array. 
# - Sends multiple "update" requests with valid existing uuids.
# - Checks the compliance of "save"/"update" request's messages.


source ../../test_utils/functions.sh

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
        
        for ((i=1; i<=100; ++i))
        do
                save=$(curl --data "save content" http://localhost:7081/db/save/node)
                res=$(echo $save | cut -d'"' -f 2)
                if [[ "uuid" != "$res" ]]; then
                        echo "Failed save in $i request"
                        echo $FAIL > $final_test_result
                        kill_metax $p
                        exit
                fi

                uuid=$(echo $save | cut -d'"' -f 4)
                uuids[i]="$uuid"
                echo $i "${uuids[i]}" 
        done
        
        for ((i=1; i<=100; ++i))
        do
                update=$(curl -m $TIME --data "updated content" "http://localhost:7081/db/save/node?id=${uuids[i]}")
                echo $i $update
                uuid=$(echo $update | cut -d'"' -f 4)
                if [ "${uuids[i]}" != "$uuid" ]; then
                        echo "Failed update in $i request"
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
