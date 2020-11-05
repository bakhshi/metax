#!/bin/bash

# Test case flow:
# - Runs and connects two instances of metax.
# - Sends "node" save request for some content with enc=0 parameter.
# - Waits about 10 second for backup then kills all metaxes.
# - Runs metax with config2.xml configuration.
# - Sends get request for received uuid to running metax.
# - Checks the compliance of received/saved contents.



source ../../test_utils/functions.sh

# wait command exit code when metax was successfully finished (i.e. without Seg. fault or Assertion)
EXIT_CODE_OK=0



start_metaxes()
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
}


main()
{
        start_metaxes
        get_user_json_uuids 2
        str=$(echo ${uj_uuids[@]} | sed 's/ /*\\|/g')

        diff_options="--exclude=cache --exclude=storage.json"

        # Save some content to metax2.
        save_content="Some content for test"
        for i in ${!uj_uuids[@]}; do
                diff_options+=" --exclude=${uj_uuids[$i]}*"
        done

        uuid=$(curl --data "$save_content" http://localhost:7081/db/save/node?enc=0\&local=0)
        uuid=$(echo $uuid | cut -d'"' -f 4)
        get_content=$(curl http://localhost:7081/db/get/?id=$uuid)
        if [ "$save_content" != "$get_content" ]; then
                echo "GET request failed. get_content = $get_content"
                kill_metax $p1 $p2
                exit;
        fi

        # Wait to be saved in metax1.
        sleep $TIME

        diff=$(diff $test_dir/storage1/cache $test_dir/storage2 $diff_options)
        if [ -n "$diff" ]; then
                echo "Invalid cache in storage2. diff = $diff"
                kill_metax $p1 $p2
                exit;
        fi

        # Metax1 goes offline
        kill_metax $p1
        update_content="Some content for update"
        uuid1=$(curl --data "$update_content" http://localhost:7091/db/save/node?id=$uuid\&local=0\&enc=0)
        uuid1=$(echo $uuid | cut -d'"' -f 4)
        get_content=$(curl http://localhost:7091/db/get/?id=$uuid1)
        if [ "$update_content" != "$get_content" ]; then
                echo "GET request failed after update request. get_content = $get_content"
                kill_metax $p2
                exit;
        fi

        diff=$(diff $test_dir/storage2 $test_dir/storage1 $diff_options)
        if [ -z "$diff" ]; then
                echo "Failed to update data. diff = $diff"
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

        # Sleep for some period to finish sync process.
        sleep $TIME

	storage_size=$(find $test_dir/storage1 -name "????????-*" | grep -v "$str" | expr $(wc -l))
	cache_size=$(find $test_dir/storage1/cache -name "????????-*" | grep -v "$str" | expr $(wc -l))
        if [ "0" != "$storage_size" ] || [ "0" != "$cache_size" ]; then
                echo "Failed to sync updated data : storage_size=$storage_size, cache_size=$cache_size"
                kill_metax $p1 $p2
                exit;
        fi

        uuid=$(curl --data "$save_content" http://localhost:7091/db/save/node)
        uuid=$(echo $uuid | cut -d'"' -f 4)
        get_content=$(curl http://localhost:7091/db/get/?id=$uuid)
        if [ "$save_content" != "$get_content" ]; then
                echo "Failed to save in metax2 after sync: $get_content"
                kill_metax $p1 $p2
                exit;
        fi

        uuid=$(curl --data "$save_content" http://localhost:7081/db/save/node)
        uuid=$(echo $uuid | cut -d'"' -f 4)
        get_content=$(curl http://localhost:7081/db/get?id=$uuid)
        if [ "$save_content" != "$get_content" ]; then
                echo "Failed to save in metax1 after sync: $get_content"
                kill_metax $p1 $p2
                exit;
        fi

        echo $PASS > $final_test_result
        kill_metax $p1 $p2
}

main $@
