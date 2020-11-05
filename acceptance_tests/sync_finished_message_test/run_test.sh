#!/bin/bash

# Test case flow: 
# - Saves a file in Metax instance 2 and kills it
# - Launch Metax instance 1
# - Connect instance 1 with websocket
# - Launch instance 2 which will connect to instance 1
# - After sync is finished between them a corresponding message should be
#   received via websocket

source ../../test_utils/functions.sh

# wait command exit code when metax was successfully finished (i.e. without Seg. fault or Assertion)
EXIT_CODE_OK=0





main()
{
        cp $tst_src_dir/websocket_client.js .
        npm install ws
        rm -f $test_dir/ws_message
        $EXECUTE -f $tst_src_dir/config2.xml &
        p2=$!
        is_peer_connected=$(peers_connect 7091 $tst_src_dir/config2.xml)
        echo -n $p2 > $test_dir/processes_ids

        save_content="Some content for test"
        curl --data "$save_content" http://localhost:7091/db/save/node
        kill_metax $p2

        $EXECUTE -f $tst_src_dir/config1.xml &
        p1=$!
        is_peer_connected=$(peers_connect 7081 $tst_src_dir/config1.xml)

        nodejs websocket_client.js 7081 $test_dir/ws_message &
        sleep 2

        $EXECUTE -f $tst_src_dir/config2.xml &
        p2=$!
        is_peer_connected=$(peers_connect 7091 $tst_src_dir/config2.xml)
        if [ "0" != "$is_peer_connected" ];then
                echo "7090 does not connect to peers"
                kill_metax $p1 $p2
                exit;
        fi
        echo -n " $p1 $p2" >> $test_dir/processes_ids
        sleep $TIME

        data=$(cat $test_dir/ws_message)
        expected='{"event":"sync finished"}'
        if [ "$data" != "$expected" ]; then
                echo "sync finished isn't received"
                kill_metax $p1 $p2
                exit;
        fi
        kill_metax $p1 $p2

        echo $PASS > $final_test_result
}

main $@
