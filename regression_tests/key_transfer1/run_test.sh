#!/bin/bash

# Test case flow:
# - Runs and connects two instances of metax (peer2->peer1).
# - Sends start_pairing request to the peer1
# - Sends get_pairing_peers to peer2 and makes sure the result is not empty
# - Takes the first pairing ip and sends request_keys request to peer2
# - Checks that user keys are the same for the both peers


source ../../test_utils/functions.sh

# wait command exit code when metax was successfully finished (i.e. without Seg. fault or Assertion)
EXIT_CODE_OK=0


main()
{
        rm -fr $test_dir/{keys1,user1.json,device1.json}
        rm -fr $test_dir/{keys2,user2.json,device2.json}
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
        code=$(curl http://localhost:7081/config/start_pairing?timeout=300 | cut -d'"' -f4)
        echo $code;
        curl http://localhost:7091/config/get_pairing_peers > $test_dir/pairing_peers
        pairing_peers_count=$(jq '. | length' $test_dir/pairing_peers)
        echo "found $pairing_peers_count pairing peers"
        if [[ 0 -eq $pairing_peers_count ]]; then
                echo "No pairing peers found"
                kill_metax $p1 $p2
                exit
        fi
	OS=$(uname -s)
	if [[ "Darwin" == "$OS" ]]; then
		my_ip=$((ipconfig getifaddr en1; ipconfig getifaddr en0)  | head -n1)
	else
		my_ip=$(ip route get 1 | sed -n 's/^.*src \([0-9.]*\) .*$/\1/p')
	fi
        echo "my_ip: $my_ip"
        is_my_peer_ready=0
        i=1
        for row in $(cat $test_dir/pairing_peers | jq -c '.[]')
        do
                peer_ip=$(echo $row | jq -r ".\"$i\"")
                echo "peer_ip: $peer_ip"
                if [ "$peer_ip" == "$my_ip" ]
                then
                        is_my_peer_ready=1
                        break
                fi
                i=$((i+1))
        done
        if [[ 0 -eq $is_my_peer_ready ]]
        then
                echo "The expected peer is not found"
                kill_metax $p1 $p2
                exit
        fi
        curl "http://localhost:7091/config/request_keys?ip=$peer_ip&code=$code"
        d=$(diff -rq $test_dir/keys1 $test_dir/keys2/ | grep "keys[1,2]/user")
        if [[ -n $d ]] ; then
                echo "User keys are not the same"
                echo $d
                kill_metax $p2 $p1
                exit;
        fi
        echo $PASS > $final_test_result
        kill_metax $p1 $p2
}

main $@

# vim:et:tabstop=8:shiftwidth=8:cindent:fo=croq:textwidth=80:
