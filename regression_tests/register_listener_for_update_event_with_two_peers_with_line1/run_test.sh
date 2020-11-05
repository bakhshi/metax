#!/bin/bash

# Test case flow:
# - Runs two instances of metax (metax2 -> metax3)
# - Send save request to metax2 (local=0).
# - Run one instance of metax (metax1 [storaeg_class = 4] -> metax2).
# - Sends websocket connect request to running metax1 by
#   running websocket_client.js nodejs script.
# - Sends register_listener request to metax1.
# - Sends update request to the metax2.
# - Check that update event successfully delivered.


source ../../test_utils/functions.sh

# wait command exit code when metax was successfully finished (i.e. without Seg. fault or Assertion)
EXIT_CODE_OK=0





main()
{
        npm install ws
        cp $tst_src_dir/websocket_client.js .
        $EXECUTE -f $tst_src_dir/config3.xml &
        p3=$!
        is_peer_connected=$(peers_connect 7101 $tst_src_dir/config3.xml)
        $EXECUTE -f $tst_src_dir/config2.xml &
        p2=$!
        is_peer_connected=$(peers_connect 7091 $tst_src_dir/config2.xml)
        if [ "0" != "$is_peer_connected" ];then
                echo "7090 does not connect to peer"
                kill_metax $p3 $p2
                exit;
        fi
        suuid=$(curl --data "save content" "http://localhost:7091/db/save/node?enc=0&local=0")
        suuid=$(echo $suuid | cut -d'"' -f 4)

        $EXECUTE -f $tst_src_dir/config1.xml &
        p1=$!
        is_peer_connected=$(peers_connect 7081 $tst_src_dir/config1.xml)
        if [ "0" != "$is_peer_connected" ];then
                echo "7080 does not connect to peers"
                kill_metax $p3 $p2 $p1
                exit;
        fi
        echo -n $p3 $p2 $p1 > $test_dir/processes_ids

        nodejs websocket_client.js $test_dir/data 7081 &
        sleep 2

        res=$(curl http://localhost:7081/db/register_listener?id=$suuid)
        expected='{"success": "registered listener for '$suuid'"}'
        if [ "$expected" != "$res" ];then
                echo "Failed to register Peer1: Mismatch of expected and received response. Res = $res"
                kill_metax $p2 $p1 $p3
                exit;
        fi

        uuuid=$(curl --data "update content" "http://localhost:7091/db/save/node?enc=0&id=$suuid")
        echo after update $uuuid
        uuuid=$(echo $uuuid | cut -d'"' -f 4)
        if [ "$uuuid" != "$suuid" ]; then
                echo "Failed to update content"
                kill_metax $p1 $p2 $p3
                exit;
        fi
        sleep 2

        data=$(cat $test_dir/data)
        expected="{\"event\":\"updated\",\"uuid\":\"$suuid\"}"
        if [ "$data" != "$expected" ]; then
                echo "Updated event isn't received"
                kill_metax $p1 $p2 $p3
                exit;
        fi 

        echo $PASS > $final_test_result
        kill_metax $p1 $p2 $p3
}

main $@
