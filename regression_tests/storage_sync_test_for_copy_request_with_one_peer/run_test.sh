#!/bin/bash

# Test case flow:
# - Runs and connects two instances of metax (metax1 [storage_class = 4] -> metax2 [storage_class = 5]).
# - Sends "node" save request for some content with enc=0 parameter to metax2.
# - Waits about 6 second for backup then kills metax1.
# - Sends copy request for received uuid to running metax2.
# - Sends get request for received uuid to running metax2.
# - Run metax with config1.xml configuration.
# - Waits about 10 second to finish sync process.
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
        for i in ${!uj_uuids[@]}; do
                diff_options+=" --exclude=${uj_uuids[$i]}*"
        done

        # Save some content to metax2.
        save_content="Some content for test"
        uuid=$(curl --data "$save_content" http://localhost:7091/db/save/node?enc=0)
        uuid=$(echo $uuid | cut -d'"' -f 4)
        get_content=$(curl http://localhost:7091/db/get/?id=$uuid)
        if [ "$save_content" != "$get_content" ]; then
                echo "GET request failed. get_content=$get_content"
                kill_metax $p1 $p2
                exit;
        fi

        # wait for backup
        sleep $TIME

        diff=$(diff $test_dir/storage2 $test_dir/storage1 $diff_options)
        if [ -n "$diff" ]; then
                echo "Failed to save uuids in two metaxes. diff=$diff"
                kill_metax $p1 $p2
                exit;
        fi

        # Metax1 goes offline
        kill_metax $p1

        storage1_old=$(find $test_dir/storage1 -name "????????-*" ! -name ${uj_uuids[0]} ! -name ${uj_uuids[1]})
	cache1_old_size=$(find $test_dir/storage1/cache -name "????????-*" | grep -v "$str" | expr $(wc -l))
        if [ "0" != "$cache1_old_size" ];then
                echo "cache1 not empty!. cache1_old_size=$cache1_old_size"
                kill_metax $p2
                exit;
        fi

        copy_res=$(curl http://localhost:7091/db/copy?id=$uuid)
        copy_uuid=$(echo $copy_res | jq -r '.copy_uuid')
        orig_uuid=$(echo $copy_res | jq -r '.orig_uuid')
        if [ "$orig_uuid" != "$uuid" ]; then
                echo "Failed to copy uuid. Copy request response : $copy_res"
                kill_metax $p2
                exit;
        fi

        get_content=$(curl http://localhost:7091/db/get/?id=$copy_uuid)
        if [ "$save_content" != "$get_content" ]; then
                echo "Failed to copy uuid: Copied uuid content: $get_content"
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

        storage1_new=$(find $test_dir/storage1 -name "????????-*" ! -name ${uj_uuids[0]} ! -name ${uj_uuids[1]})
	cache1_new_size=$(find $test_dir/storage1/cache -name "????????-*" | grep -v "$str" | expr $(wc -l))
        storage2=$(find $test_dir/storage2 -name "????????-*" ! -name ${uj_uuids[0]} ! -name ${uj_uuids[1]})
	storage2_size=$(find $test_dir/storage2 -name "????????-*" | grep -v "$str" | expr $(wc -l))

        if [ "0" != "$cache1_new_size" ];then
                echo "cache1 not empty!. cache1_new_size=$cache1_new_size"
                kill_metax $p1 $p2
                exit;
        fi

        if [[ "$storage1_new" != "$storage1_old" ]]; then
                echo "Invalid storage1 after sync. storage1_old=$storage1_old, storage1_new=$storage1_new"
                kill_metax $p1 $p2
                exit;
        fi

        if [[ ! "$storage2[@]" =~ "$copy_uuid" ]] || [[ ! "$storage2[@]" =~ "$orig_uuid" ]]; then
                echo "Invalid storage2 after sync"
                kill_metax $p1 $p2
                exit;
        fi

        echo $PASS > $final_test_result
        kill_metax $p1 $p2
}

main $@
