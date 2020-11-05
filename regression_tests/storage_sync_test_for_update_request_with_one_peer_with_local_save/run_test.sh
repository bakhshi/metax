#!/bin/bash

# Test case flow: 
# - Runs and connects two instances of metax (metax1 [storage_class = 4] -> metax2 [storage_class = 5]).
# - Sends "node" save request for some content with enc=0 parameter to metax1.
# - Waits about 6 second for backup then kills metax1.
# - Sends update request for received uuid to running metax2.
# - Run metax with config1.xml configuration.
# - Waits about 10 second to finish sync process.
# - Checks storage contents for metax1 after sync.

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

        $EXECUTE -f $tst_src_dir/config1.xml &
        p1=$!
        is_peer_connected=$(peers_connect 7081 $tst_src_dir/config1.xml)
        if [ "0" != "$is_peer_connected" ];then
                echo "7080 does not connect to peers"
                kill_metax $p1 $p2
                exit;
        fi
        echo -n $p2 $p1 > $test_dir/processes_ids
        get_user_json_uuids 2
        str=$(echo ${uj_uuids[@]} | sed 's/ /*\\|/g')

        diff_options="--exclude=cache --exclude=storage.json"
        for i in ${!uj_uuids[@]}; do
                diff_options+=" --exclude=${uj_uuids[$i]}*"
        done

        # Save some content to metax2.
        save_content="Some content for test"
        uuid=$(curl --data "$save_content" http://localhost:7081/db/save/node?enc=0)
	uuid=$(echo $uuid | cut -d'"' -f 4)
        echo "uuid=$uuid"

        get_content=$(curl http://localhost:7081/db/get/?id=$uuid)
        if [ "$save_content" != "$get_content" ]; then
                echo "GET request failed. get_content = $get_content"
                kill_metax $p1 $p2
                exit;
        fi

        #Wait to be saved in metax2.
        sleep $TIME
	piece=$(cat $test_dir/storage1/$uuid | jq .pieces | jq -r 'keys[]')
        echo "piece=$piece"

        diff=$(diff $test_dir/storage1 $test_dir/storage2 $diff_options)
        if [ -n "$diff" ]; then
                echo "Failed save data in two metaxes. Diff = $diff"
                kill_metax $p1 $p2
                exit;
        fi

        # Metax1 goes offline
        kill_metax $p1

	update_content="Some content for update"
        uuid1=$(curl --data "$update_content" http://localhost:7091/db/save/node?id=$uuid\&enc=0)
        uuid1=$(echo $uuid | cut -d'"' -f 4)

        get_content=$(curl http://localhost:7091/db/get/?id=$uuid1)
        if [ "$update_content" != "$get_content" ]; then
                echo "GET request after update failed. get_content=$get_content"
                kill_metax $p2
                exit;
        fi

        new_piece=$(cat $test_dir/storage2/$uuid | jq .pieces | jq -r 'keys[]')
        echo "new piece=$new_piece"

        $EXECUTE -f $tst_src_dir/config1.xml &
        p1=$!
        is_peer_connected=$(peers_connect 7081 $tst_src_dir/config1.xml)
        if [ "0" != "$is_peer_connected" ];then
                echo "7080 does not connect to peers"
                kill_metax $p1 $p2
                exit;
        fi
        echo -n $p1 >> $test_dir/processes_ids

        # Sleep for some period to finish sync process.
        sleep $TIME

        res=$(find $test_dir/storage1 -name "$piece")
        if [ -n "$res" ];then
                echo "Old piece in storage1 was not deleted after sync"
                echo "find = $res"
                kill_metax $p1 $p2
                exit;
        fi

        res=$(find $test_dir/storage1 -name "$new_piece")
        if [ -n "$res" ];then
                echo "New piece in storage1 should not appear after sync"
                echo "find = $res"
                kill_metax $p1 $p2
                exit;
        fi

        diff=$(find $test_dir/storage1/$uuid1 $test_dir/storage2/$uuid1)
        if [ -n "$res" ];then
                echo "Master objects in storage1 and storage2 should be the same after sync"
                echo "Diff = $diff"
                kill_metax $p1 $p2
                exit;
        fi

        cache_content=$(ls $test_dir/storage1/cache/????????-* | grep -v "$str" 2&>/dev/null)
        if [ -n "$res" ];then
                echo "Cache for metax1 should be empty after sync"
                echo "cache_content = $cache_content"
                kill_metax $p1 $p2
                exit;
        fi

        get_content=$(curl http://localhost:7081/db/get/?id=$uuid1)
        if [ "$update_content" != "$get_content" ]; then
                echo "Unable to get uuid after sync. Get res = $get_content"
                kill_metax $p1 $p2
                exit;
        fi

        echo $PASS > $final_test_result
        kill_metax $p1 $p2
}

main $@

