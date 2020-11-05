#!/bin/bash

# Test case flow: 
# - runs two connected Metax peers 1 and 2
# - sends get_online_peers request
# - checks correctness of received public key

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
                kill_metax $p2 $p1
                exit;
        fi
        echo -n $p2 $p1 > $test_dir/processes_ids
        res=$(curl http://localhost:7081/config/get_online_peers)
        echo $res
        public_key=$(echo $res | jq -r '.[]' | jq -r '.user_public_key')
        expected_key=$(cat $test_dir/keys2/user/public.pem)
        kill_metax $p1 $p2;
        if [ "$public_key" != "$expected_key" ]; then
                echo "unexpected key is received"
                echo $FAIL > $final_test_result
                exit;
        fi
        echo $PASS > $final_test_result
}

main $@
