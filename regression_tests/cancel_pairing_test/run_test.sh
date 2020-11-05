#!/bin/bash

# Test case flow:
# - Runs and connects two instances of metax (peer2->peer1).
# - Sends start_pairing request to the peer1 and checks response
# - Sends get_pairing_peers to peer2 and makes sure the result is not empty
# - Sends cancel_pairing request to the peer1 and checks response
# - Sends get_pairing_peers to peer2 and makes sure the result is empty


source ../../test_utils/functions.sh

# wait command exit code when metax was successfully finished (i.e. without Seg. fault or Assertion)
EXIT_CODE_OK=0


main()
{
        $EXECUTE -f $tst_src_dir/config1.xml &
        p1=$!
        is_peer_connected=$(peers_connect 7081 $tst_src_dir/config1.xml)
        if [ "0" != "$is_peer_connected" ];then
                echo "7080 does not connect to peer"
                kill_metax $p1
                exit;
        fi
        $EXECUTE -f $tst_src_dir/config2.xml &
        p2=$!
        is_peer_connected=$(peers_connect 7091 $tst_src_dir/config2.xml)
        if [ "0" != "$is_peer_connected" ];then
                echo "7090 does not connect to peer"
                kill_metax $p2 $p1
                exit;
        fi
        echo -n $p1 $p2 > $test_dir/processes_ids

        res=$(curl http://localhost:7081/config/start_pairing?timeout=300)
        pairing_res=$(echo $res | jq '.generated_code')
        if [ null == $pairing_res ]; then
                echo "start_pairing request failed. Response is $res"
                kill_metax $p1 $p2
                exit
        fi

        curl http://localhost:7091/config/get_pairing_peers > $test_dir/pairing_peers
        pairing_peers_count=$(jq '. | length' $test_dir/pairing_peers)
        echo "after start pairing found $pairing_peers_count pairing peers"
        if [[ 0 -eq $pairing_peers_count ]]; then
                echo "No pairing peers found after start pairing"
                kill_metax $p1 $p2
                exit
        fi

        curl -D $test_dir/http_status http://localhost:7081/config/cancel_pairing
        http_code=$(cat $test_dir/http_status | grep HTTP/1.1 | awk {'print $2'})
        echo "http_code is $http_code"
        if [[ 200 != $http_code ]]; then
                echo "cancel_pairing request failed. HTTP code of request response is $http_code"
                kill_metax $p1 $p2
                exit
        fi

        curl http://localhost:7091/config/get_pairing_peers > $test_dir/pairing_peers_after_cancel
        pairing_peers_count=$(jq '. | length' $test_dir/pairing_peers_after_cancel)
        echo "after cancel pairing found $pairing_peers_count pairing peers"
        if [[ 0 -ne $pairing_peers_count ]]; then
                echo "Pairing peers found after cancel pairing"
                kill_metax $p1 $p2
                exit
        fi

        echo $PASS > $final_test_result
        kill_metax $p1 $p2
}

main $@

# vim:et:tabstop=8:shiftwidth=8:cindent:fo=croq:textwidth=80:
