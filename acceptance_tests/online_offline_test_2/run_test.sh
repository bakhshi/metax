#!/bin/bash

# Test case flow:
# - Runs and connects local two metaxes
# - Check that peers are connected successfully (by using /config/get_online_peers request).
# - Sleep 91 seconds and check that peers are connected successfully (by using /config/get_online_peers request).


source ../../test_utils/functions.sh

# wait command exit code when metax was successfully finished (i.e. without Seg. fault or Assertion)
EXIT_CODE_OK=0




main()
{
        $EXECUTE -f $tst_src_dir/config2.xml &
        p2=$!
        is_peer_connected=$(peers_connect 7091 $tst_src_dir/config2.xml)

        $EXECUTE -f $tst_src_dir/config1.xml &
        p1=$!
        is_peer_connected=$(peers_connect 7081 $tst_src_dir/config1.xml)
        if [ "0" != "$is_peer_connected" ];then
                echo "7080 does not connect to peers"
                kill_metax $p1 $p2
                exit;
        fi
        echo -n $p1 $p2 > $test_dir/processes_ids

        res=$(curl http://localhost:7091/config/get_user_public_key)
        echo "res $res"
        exp_peer_public_key=$(echo $res | jq -r '.user_public_key')

        res=$(curl http://localhost:7081/config/get_online_peers)
        echo "res $res"
        public_key=$(echo $res | jq -r '.[]' | jq -r '.user_public_key')
        if [ "$public_key" != "$exp_peer_public_key" ]; then
                echo "Unexpected key is received"
                kill_metax $p1 $p2
                exit;
        fi

        sleep 91

        res=$(curl http://localhost:7081/config/get_online_peers)
        echo $res
        public_key=$(echo $res | jq -r '.[]' | jq -r '.user_public_key')
        if [ "$public_key" != "$exp_peer_public_key" ]; then
                echo "Network enabled, but peer is disconnected."
                kill_metax $p1 $p2
                exit;
        fi

        echo $PASS > $final_test_result
        kill_metax $p1 $p2
}

main $@
