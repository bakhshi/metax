#!/bin/bash

#Test case flow: 
# - Runs and connects three instance of metax (pee1 -> peer2 -> peer3).
# - Sends save encrypted content request to peer1.
# - Sends share request to peer2 and peer3 with public keys.
# - Sends get requests to peer2 and peer3.
# - Checks the compliance of saved/shared contents.


source ../../test_utils/functions.sh

# wait command exit code when metax was successfully finished (i.e. without Seg. fault or Assertion)
EXIT_CODE_OK=0



start_metaxes()
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

        $EXECUTE -f $tst_src_dir/config1.xml &
        p1=$!
        is_peer_connected=$(peers_connect 7081 $tst_src_dir/config1.xml)
        if [ "0" != "$is_peer_connected" ];then
                echo "7080 does not connect to peers"
                kill_metax $p3 $p2 $p1
                exit;
        fi
        echo -n $p3 $p2 $p1 > $test_dir/processes_ids
}



save_file=$test_dir/save_file

main()
{
        start_metaxes
        save_content="Some content for share test"

        get_public_key=$(curl http://localhost:7091/config/get_user_public_key) 
        public_key=$(echo $get_public_key | jq -r '.user_public_key')
        echo "public_key=$public_key"

        get_public_key1=$(curl http://localhost:7101/config/get_user_public_key) 
        public_key1=$(echo $get_public_key1 | jq -r '.user_public_key')
        echo "public_key1=$public_key1"

        uuid=$(curl --data "$save_content" http://localhost:7081/db/save/node)
        uuid=$(echo $uuid | cut -d'"' -f 4)
        echo "uuid = $uuid"

        res=$(curl -m 10 -F "key=$public_key" http://localhost:7081/db/share?id=$uuid)
	share_uuid=$(echo $res | jq -r '.share_uuid')
        key=$(echo $res | jq -r '.key')
        iv=$(echo $res | jq -r '.iv')

        res1=$(curl -m 10 -F "key=$public_key1" http://localhost:7081/db/share?id=$uuid)
        share_uuid1=$(echo $res1 | jq -r '.share_uuid')
        key1=$(echo $res1 | jq -r '.key')
        iv1=$(echo $res1 | jq -r '.iv')

        if [ $share_uuid != $share_uuid1 ];then
                echo "Shared uuids are different."
                kill_metax $p1 $p2 $p3
                exit;
        fi

        expected='{"share":"accepted"}'
        received=$(curl -m 10 -F "key=$key" -F "iv=$iv" http://localhost:7091/db/accept_share?id=$share_uuid)
        if [ "$expected" != "$received" ]; then
                echo "Share accept failed from peer2. received=$received"
                kill_metax $p1 $p2 $p3;
                exit;
        fi

        received1=$(curl -m 10 -F "key=$key1" -F "iv=$iv1" http://localhost:7101/db/accept_share?id=$share_uuid)
        if [ "$expected" != "$received1" ]; then
                echo "Share accept failed from peer3. received=$received1"
                kill_metax $p1 $p2 $p3;
                exit;
        fi

        get=$(curl "http://localhost:7091/db/get?id=$share_uuid")
        get1=$(curl "http://localhost:7101/db/get?id=$share_uuid")
        echo "get = $get"
        echo "get1 = $get1"

        if [ "$save_content" != "$get" ] || [ "$save_content" != "$get1" ];then
                echo "Failed to get encrypted content from peers"
                kill_metax $p1 $p2 $p3
                exit;
        fi

        echo $PASS > $final_test_result
        kill_metax $p1 $p2 $p3
}

main $@
