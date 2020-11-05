#!/bin/bash

# Test case flow:
# - Run and connects local metax to 172.17.7.65 (Lvt_Box_2)
# - Check that peers are connected successfully (by using /config/get_online_peers request).
# - Turn off network then need to sleep 91 seconds and check that peers are disconnected (by using /config/get_online_peers request)
# - Turns on networking.


source ../../test_utils/functions.sh

# wait command exit code when metax was successfully finished (i.e. without Seg. fault or Assertion)
EXIT_CODE_OK=0





main()
{
        if [[ "Darwin" == "$target" ]]; then
                echo $SKIP > $final_test_result
                exit
        fi
        net_name=$(ls /sys/class/net | grep "^e.*" | head -n 1)
        echo $net_name
        sudo ifconfig $net_name:0 10.10.10.10
        echo "$net_name:0 10.10.10.10 Connection Established."

        $EXECUTE -f $tst_src_dir/config2.xml &
        p2=$!
        is_peer_connected=$(peers_connect 7091 $tst_src_dir/config2.xml)

        $EXECUTE -f $tst_src_dir/config1.xml &
        p1=$!
        is_peer_connected=$(peers_connect 7081 $tst_src_dir/config1.xml)
        if [ "0" != "$is_peer_connected" ];then
                echo "7080 does not connect to peers"
                kill_metax $p1
                exit;
        fi
        echo -n $p1 $p2 > $test_dir/processes_ids

        res=$(curl http://10.10.10.10:7091/config/get_user_public_key)
        echo "res $res"
        exp_peer_public_key=$(echo $res | jq -r '.user_public_key')

        res=$(curl http://10.10.10.10:7081/config/get_online_peers)
        echo "res $res"
        public_key=$(echo $res | jq -r '.[]' | jq -r '.user_public_key')
        if [ "$public_key" != "$exp_peer_public_key" ]; then
                echo "Unexpected key is received"
                kill_metax $p1 $p2
                exit;
        fi

        sudo ifconfig $net_name:0 down
        echo "$net_name:0 10.10.10.10 Disconnected"
        sleep 91

        res=$(curl http://localhost:7081/config/get_online_peers)
        echo $res
        expected="[]"
        if [ "$res" != "$expected" ]; then
                echo "Network is gone offline, but peer is not disconnected."
                kill_metax $p1 $p2
                exit;
        fi

        echo $PASS > $final_test_result
        kill_metax $p1 $p2
}

main $@
