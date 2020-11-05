#!/bin/bash

# Test case flow:
# - Runs and connects two instances of metax (peer2->peer1).
# - Kills peer1, runs peer1, send reconnect_to_peers request COUNT times
# - Every time checks that peer is connected


source ../../test_utils/functions.sh

# wait command exit code when metax was successfully finished (i.e. without Seg. fault or Assertion)
EXIT_CODE_OK=0
COUNT=3


main()
{
        $EXECUTE -f $tst_src_dir/config1.xml &
        p1=$!
        is_peer_connected=$(peers_connect 7081 $tst_src_dir/config1.xml)
        if [ "0" != "$is_peer_connected" ];then
                echo "7080 does not connect to peer"
                kill_metax $p1
                exit
        fi
        $EXECUTE -f $tst_src_dir/config2.xml &
        p2=$!
        is_peer_connected=$(peers_connect 7091 $tst_src_dir/config2.xml)
        if [ "0" != "$is_peer_connected" ];then
                echo "7090 does not connect to peer"
                kill_metax $p2 $p1
                exit
        fi
        echo -n $p1 $p2 > $test_dir/processes_ids

        for ((index = 1; index <= $COUNT; ++index))
        do
                kill_metax $p1
                $EXECUTE -f $tst_src_dir/config1.xml &
                p1=$!
                is_http_server_started=$(wait_for_init 7081)
                if [ "0" != "$is_http_server_started" ];then
                        echo "htpp server isn't started - 7081"
                        kill_metax $p1 $p2
                        exit
                fi
                echo -n $p1 $p2 > $test_dir/processes_ids

                res=$(curl http://localhost:7091/config/get_online_peers)
                echo "i=$index and get_online_peers response before reconnect is $res"
                l=$(echo $res | jq '. | length')
                if [[ 0 -ne $l ]]; then
                        echo "before reconnect to peers get online peers list should be empty"
                        if [[ $COUNT -eq $index ]]; then
                                kill_metax $p2 $p1
                                exit
                        fi
                        continue
                fi

                curl -D $test_dir/http_status http://localhost:7091/config/reconnect_to_peers
                http_code=$(cat $test_dir/http_status | grep HTTP/1.1 | awk {'print $2'})
                echo "http_code is $http_code"
                if [[ 200 != $http_code ]]; then
                        echo "Failed to reconnect to peer. HTTP code of reconnect_to_peers request response is $http_code"
                        kill_metax $p1 $p2
                        exit
                fi
                sleep 2
                res=$(curl http://localhost:7091/config/get_online_peers)
                echo "i=$index and get_online_peers response after reconnect is $res"
                l=$(echo $res | jq '. | length')
                if [[ 0 -eq $l ]]; then
                        echo "get online peers list should not be empty, failed to reconnect to peer"
                        kill_metax $p2 $p1
                        exit
                fi
        done

        echo $PASS > $final_test_result
        kill_metax $p1 $p2
}

main $@

# vim:et:tabstop=8:shiftwidth=8:cindent:fo=croq:textwidth=80:
