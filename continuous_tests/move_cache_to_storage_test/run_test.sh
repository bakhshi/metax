#!/bin/bash

# Test case flow:
# - Runs one instance of metax.
# - Sends content save request for some data with enc=0 parameter..
# - Moves saved data from storage to cache
# - Calls move_cache_to_storage metax API to move data from cache to storage
# - Checks that all the original data is return back to storage



source ../../test_utils/functions.sh

# wait command exit code when metax was successfully finished (i.e. without Seg. fault or Assertion)
EXIT_CODE_OK=0



main()
{
        $EXECUTE -f $tst_src_dir/config.xml &
        p=$!
        wait_for_init 7081
        echo -n $p > $test_dir/processes_ids

        save_content="Some save content"
        uuid=$(curl --data "$save_content" http://localhost:7081/db/save/node?enc=0)
        uuid=$(echo $uuid | cut -d'"' -f 4)
        sleep 3
        echo "ls storage `ls $test_dir/storage`"
	storage_count1=$(ls $test_dir/storage/????????-*-???????????? | expr $(wc -l))
        echo "storage_count = $storage_count1"
        mv $test_dir/storage/????????-*-???????????? $test_dir/storage/cache/
        cache_count=$(ls $test_dir/storage/cache/????????-*-???????????? | expr $(wc -l))
        echo "cache_count = $cache_count"
        if [ $storage_count1 -ne $cache_count ]; then
                echo "unexpected mismatch of storage and cache file count"
                kill_metax $p
                exit
        fi
        expected='{"success":"Moving cache to storage completed"}'
        res=$(curl http://localhost:7081/db/move_cache_to_storage)
        echo "$res"
        if [ "$expected" != "$res" ]; then
                echo "moving cache to storage succeeded. Expected = $expected, res = $res"
                kill_metax $p
                exit;
        fi
        res=$(echo $res | cut -d'"' -f 2)
        if [ "success" != "$res" ]; then
                echo "move to cache failed"
                kill_metax $p
                exit
        fi
	storage_count2=$(ls $test_dir/storage/????????-*-???????????? | expr $(wc -l))
        echo "storage_count after moving is $storage_count2"
        if [ $storage_count1 -eq $storage_count2 ]; then
                echo $PASS > $final_test_result
        else
                echo "unexpected mismatch of storage and cache file count after move_to_cache"
        fi
        kill_metax $p
}

main $@
