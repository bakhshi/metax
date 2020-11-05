#!/bin/bash

# Test case flow: 
# - Runs and connects three instance of metax (peer1 -> peer2).
# - Sends save encrypted content request to peer1.
# - Sends share request to peer1 with peer2 public key.
# - Sends update request to peer1 10 times.
# - Sends get requests to peer2.
# - Checks update request response.


source ../../test_utils/functions.sh

# wait command exit code when metax was successfully finished (i.e. without Seg. fault or Assertion)
EXIT_CODE_OK=0




start_metaxes()
{
        $EXECUTE -f $tst_src_dir/config2.xml &
        p2=$!
        is_peer_connected=$(peers_connect 7091 $tst_src_dir/config2.xml)
        if [ "0" != "$is_peer_connected" ];then
                echo "7090 does not connect to peer"
                kill_metax $p2
                exit;
        fi

        $EXECUTE -f $tst_src_dir/config1.xml &
        p1=$!
        is_peer_connected=$(peers_connect 7081 $tst_src_dir/config1.xml)
        if [ "0" != "$is_peer_connected" ];then
                echo "7080 does not connect to peers"
                kill_metax $p2 $p1
                exit;
        fi
        echo -n $p2 $p1 > $test_dir/processes_ids
}




save_content="Some content for share test"

main()
{
        npm install ws
        cp $tst_src_dir/websocket_client.js .

        start_metaxes
        nodejs websocket_client.js $test_dir/data 7091 &
        sleep 2

        get_public_key=$(curl http://localhost:7091/config/get_user_public_key)
        public_key=$(echo $get_public_key | jq -r '.user_public_key')
        echo "public_key=$public_key"

        uuid=$(curl --data "$save_content" http://localhost:7081/db/save/node)
        uuid=$(echo $uuid | cut -d'"' -f 4)
        echo "uuid = $uuid"

        res=$(curl -m 10 -F "key=$public_key" http://localhost:7081/db/share?id=$uuid)

	share_uuid=$(echo $res | jq -r '.share_uuid')
        key=$(echo $res | jq -r '.key')
        iv=$(echo $res | jq -r '.iv')

        received=$(curl -m 10 -F "key=$key" -F "iv=$iv" http://localhost:7091/db/accept_share?id=$uuid)
        expected='{"share":"accepted"}'
        if [ "$expected" != "$received" ]; then
                echo "Share accept failed from peer2. received=$received"
                kill_metax $p1 $p2
                exit;
        fi

        res=$(curl http://localhost:7091/db/register_listener?id=$share_uuid)
        echo $res
        res=$(echo $res | jq -r 'keys[]')
        if [[ $res != success ]]
        then
                echo "Failed to register on update of $share_uuid"
                kill_metax $p2 $p1
                exit
        fi

        # Checking that content was shared
        get=$(curl "http://localhost:7091/db/get?id=$share_uuid")
        echo "getting before update: $get"
        if [ "$save_content" != "$get" ];then
                echo "Failed to get shared content from peer"
                kill_metax $p1 $p2
                exit;
        fi

        n=10
        i=0
        while [ $i -le $n ]
        do
                update_content="Update content share test $i"

                curl --data "$update_content" http://localhost:7091/db/save/node?id=$share_uuid
                sleep 2

                data=$(cat $test_dir/data)
                expected="{\"event\":\"updated\",\"uuid\":\"$share_uuid\"}"
                if [ "$data" != "$expected" ]; then
                        echo "Updated event isn't received"
                        kill_metax $p1 $p2
                        exit;
                fi

                get=$(curl "http://localhost:7091/db/get?id=$share_uuid")
                echo "get_$i = $get"
                if [ "$update_content" != "$get" ];then
                        echo "Failed to get updated content from peer2"
                        kill_metax $p1 $p2
                        exit;
                fi
                rm $test_dir/data
                i=$((i+1))
        done

        echo $PASS > $final_test_result
        kill_metax $p1 $p2
}

main $@
