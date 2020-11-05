#!/bin/bash

# Test case flow:
# - Runs an instance of metax: peer1
# - Sends "save" request to peer1 and gets the uuid
# - Runs the second instance of metax and connects to the first (peer2->peer1)
# - Sends "get" request to peer2, the requested data is retreived from peer1,
#   and cached by peer2
# - Sends "update" requests to peer2
# - Sends "get" request to peer2 again to make sure that the update request
#   droped cache in peer2 and successfully updated data in peer1


source ../../test_utils/functions.sh

# wait command exit code when metax was successfully finished (i.e. without Seg. fault or Assertion)
EXIT_CODE_OK=0







main()
{
        save_content="save content"
        $EXECUTE -f $tst_src_dir/config1.xml &
        p1=$!
        is_peer_connected=$(peers_connect 7081 $tst_src_dir/config1.xml)
        if [ "0" != "$is_peer_connected" ];then
                echo "7080 does not connect to peers"
                kill_metax $p1
                exit;
        fi
        echo -n $p1 > $test_dir/processes_ids
        
        save=$(curl --data "$save_content" "http://localhost:7081/db/save/node?enc=0")
        uuid=$(echo $save | cut -d'"' -f 4)

        $EXECUTE -f $tst_src_dir/config2.xml &
        p2=$!
        is_peer_connected=$(peers_connect 7091 $tst_src_dir/config2.xml)
        if [ "0" != "$is_peer_connected" ];then
                echo "7090 does not connect to peers"
                kill_metax $p2 $p1
                exit;
        fi
        echo -n $p1 $p2 > $test_dir/processes_ids

        get_content=$(curl -m 10 "http://localhost:7081/db/get?id=$uuid")
        if [ "$save_content" != "$get_content" ]; then
                echo "Failed getting from storage"
                echo $FAIL > $final_test_result
                kill_metax $p1 $p2
                exit
        fi
        update_content="updated content"
        update=$(curl -m 10 --data "$update_content" "http://localhost:7091/db/save/node?enc=0&id=$uuid")
        sleep $TIME
        get_content=$(curl -m 10 "http://localhost:7091/db/get?id=$uuid")
        if [ "$update_content" != "$get_content" ]; then
                echo "update=$update"
                echo $FAIL > $final_test_result
                kill_metax $p1 $p2
                exit
        fi
        echo $PASS > $final_test_result
        kill_metax $p1 $p2
}

main $@

# vim:et:tabstop=8:shiftwidth=8:cindent:fo=croq:textwidth=80:
