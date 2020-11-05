#!/bin/bash

# Test case flow: 
# - Runs an instance of Metax - Metax 3
# - Runs an instance of Metax - Metax 2 and connects to Metax 3
# - Copies user keys and user_json_info from Metax 2 to Metax 1 folder
# - Runs Metax 1 with the same user keys as Metax 2 and connects to Metax 3
# - Metax 1 and Metax 2 storage class is 0, Metax 3 storage class is 5
# - Saves an encrypted file in Metax 1
# - Gets the file from Metax 2
# - Checks the file is gotten successfully
# - While saving file in Metax 1, the user json should be updated in Metax 3
#   also, Metax 2 should be informed about the update via register listener and
#   get the modified version by Metax 1, thus getting the encrypted data from
#   Metax 2 should succeed


source ../../test_utils/functions.sh

# wait command exit code when metax was successfully finished (i.e. without Seg. fault or Assertion)
EXIT_CODE_OK=0

main()
{
        $EXECUTE -f $tst_src_dir/config3.xml &
        p3=$!
        is_peer_connected=$(peers_connect 7101 $tst_src_dir/config3.xml)
        if [ "0" != "$is_peer_connected" ];then
                echo "7101 does not connect to peer"
                kill_metax $p3
                exit;
        fi
        $EXECUTE -f $tst_src_dir/config2.xml &
        p2=$!
        is_peer_connected=$(peers_connect 7091 $tst_src_dir/config2.xml)
        if [ "0" != "$is_peer_connected" ];then
                echo "7090 does not connect to peer"
                kill_metax $p3 $p2
                exit;
        fi


	mkdir -p $test_dir/keys1
	cp -rf $test_dir/keys2/user $test_dir/keys1/
	cp  -f $test_dir/keys2/user_json_info.json $test_dir/keys1/user_json_info.json

        $EXECUTE -f $tst_src_dir/config1.xml &
        p1=$!
        is_peer_connected=$(peers_connect 7081 $tst_src_dir/config1.xml)
        if [ "0" != "$is_peer_connected" ];then
                echo "7080 does not connect to peers"
                kill_metax $p1 $p2 $p3
                exit;
        fi
        echo -n $p3 $p2 $p1 > $test_dir/processes_ids

	save_data="save_data"
        uuid=$(curl --data "$save_data" http://localhost:7081/db/save/node)
        uuid=$(echo $uuid | cut -d'"' -f 4)
        sleep $TIME

	res=$(curl http://localhost:7091/db/get/?id=$uuid)
	echo $res
        if [[ "$res" == "$save_data" ]]; then
                echo $PASS > $final_test_result
        fi
	kill_metax $p1 $p2 $p3
}

main $@
