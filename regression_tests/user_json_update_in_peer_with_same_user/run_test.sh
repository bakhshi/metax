#!/bin/bash

# Test case flow: 
# - Runs an instance of Metax - Metax 2
# - Copies user keys and user_json_info from Metax 2 to Metax 1 folder
# - Runs Metax 1 with the same user keys as Metax 2 and connects them each other
# - Saves an encrypted file in Metax 1
# - Gets the file from Metax 2
# - Checks the file is gotten successfully

source ../../test_utils/functions.sh

# wait command exit code when metax was successfully finished (i.e. without Seg. fault or Assertion)
EXIT_CODE_OK=0

main()
{
        $EXECUTE -f $tst_src_dir/config2.xml &
        p2=$!
        is_peer_connected=$(peers_connect 7091 $tst_src_dir/config2.xml)
        if [ "0" != "$is_peer_connected" ];then
                echo "7090 does not connect to peers"
                kill_metax $p2
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
                kill_metax $p1 $p2
                exit;
        fi
        echo -n $p2 $p1 > $test_dir/processes_ids

        save_data="save data"
        uuid=$(curl --data $save_data http://localhost:7081/db/save/node)
        uuid=$(echo $uuid | cut -d'"' -f 4)
        sleep $TIME

	res=$(curl http://localhost:7091/db/get/?id=$uuid)
	echo $res
        if [[ "$res" == "$save_data" ]]; then
                echo $PASS > $final_test_result
        fi
        kill_metax $p1 $p2
}

main $@
