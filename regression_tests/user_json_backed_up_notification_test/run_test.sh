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

start_metaxes()
{
        $EXECUTE -f $tst_src_dir/config2.xml &
        p2=$!
        is_peer_connected=$(peers_connect 7091 $tst_src_dir/config2.xml)
        if [ "0" != "$is_peer_connected" ];then
                echo "7090 does not connect to peers"
                kill_metax $p2
                exit;
        fi

        $EXECUTE -f $tst_src_dir/config1.xml &
        p1=$!
        is_peer_connected=$(peers_connect 7081 $tst_src_dir/config1.xml)
        if [ "0" != "$is_peer_connected" ];then
                echo "7080 does not connect to peers"
                kill_metax $p1 $p2
                exit;
        fi
        echo -n $p2 $p1 > $test_dir/processes_ids
}

main()
{
        cp $tst_src_dir/websocket_client.js .
        npm install ws
        rm -f $test_dir/ws_message

        start_metaxes

        nodejs websocket_client.js 7081 
        if [ "0" != "$?" ]; then
                echo " doesn't receive user json backed up message"
                kill_metax $p1 $p2
                exit;
        fi
        kill_metax $p1 $p2

        echo $PASS > $final_test_result
}

main $@
