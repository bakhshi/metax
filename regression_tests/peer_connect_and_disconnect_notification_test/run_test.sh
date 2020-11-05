#!/bin/bash

# Test case flow: 
# - Runs first instance of metax with config2.xml configuration.
# - Sends websocket connect request to the first running metax (config2.xml) by
#   running websocket_client.js nodejs script
# - Runs second instance of metax with config1.xml configuration (metax1 -> metax2)
# - Check that metax1 connected to metax2.

source ../../test_utils/functions.sh

# wait command exit code when metax was successfully finished (i.e. without Seg. fault or Assertion)
EXIT_CODE_OK=0

main()
{
        npm install ws
        cp $tst_src_dir/websocket_client.js .
        $EXECUTE -f $tst_src_dir/config2.xml &
        p2=$!
        is_peer_connected=$(peers_connect 7091 $tst_src_dir/config2.xml)
        nodejs websocket_client.js $test_dir/data 7091 &
        sleep 2

        echo "--------RUN METAX1---------"
        $EXECUTE -f $tst_src_dir/config1.xml &
        p1=$!
        is_peer_connected=$(peers_connect 7081 $tst_src_dir/config1.xml)
        if [ "0" != "$is_peer_connected" ];then
                echo "7080 does not connect to peers"
                kill_metax $p2 $p1
                exit;
        fi
        echo -n $p2 $p1 > $test_dir/processes_ids
        res=$(curl http://localhost:7081/config/get_user_public_key)
        peer1_public_key=$(echo $res | jq '.user_public_key')
        if [[ -z $res ]] || [[ null == $peer1_public_key ]]; then
                echo "Unable to get user public key from peer1. Res = $res"
                kill_metax $p1 $p2
                exit;
        fi
        echo "peer1_public_key=$peer1_public_key"

        #sed $SEDIP 's/{"event":"sync finished"}//g' $test_dir/data
        #sed $SEDIP 's/{"event":"keys are generated"}//g' $test_dir/data
        evnt=$(grep "peer_is_connected" $test_dir/data)
        connect=$(echo $evnt | jq '.event')
        public_key=$(echo $evnt | jq '.peer' | jq '.user_public_key')
        expected='"peer_is_connected"'

        if [ "$expected" != "$connect" ] || [ "$peer1_public_key" != "$public_key" ]; then
                echo "Received incorrect connect event. Event=$connect, public_key=$public_key"
                kill_metax $p1 $p2
                exit;
        fi
        rm $test_dir/data 

        echo "--------KILL METAX1---------"
        kill_metax $p1
        sleep 2

        evnt=$(grep "peer_is_disconnected" $test_dir/data)
        connect=$(echo $evnt | jq '.event')
        expected='"peer_is_disconnected"'
        public_key=$(echo $evnt | jq '.peer' | jq '.user_public_key')
        if [ "$expected" != "$connect" ] || [ "$peer1_public_key" != "$public_key" ]; then
                echo "Received incorrect disconnect event. Event=$connect, public_key=$public_key"
                kill_metax $p2
                exit;
        fi

        echo $PASS > $final_test_result
        kill_metax $p2
}

main $@
