#!/bin/bash

# Test case flow:
# - Runs and connects two instances of metax.
# - Sends "node" save request for some content with encrypted.
# - Waits about 5 second for backup save.
# - Sends delete request to not owner user.
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


main()
{
        start_metaxes
        save_content="Some content for test"
        uuid=$(curl --data "$save_content" http://localhost:7081/db/save/node)
        uuid=$(echo $uuid | cut -d'"' -f 4)
        sleep $TIME
        get_content=$(curl http://localhost:7081/db/get/?id=$uuid)
        if [ "$save_content" != "$get_content" ]; then
                echo "GET request failed. get_content = $get_content"
                kill_metax $p1 $p2
                exit;
        fi

        expected='{"error":"Failed to delete '$uuid' uuid."}'
        res=$(curl -D $test_dir/h http://localhost:7091/db/delete/?id=$uuid)
        echo $res
        if [ "$expected" != "$res" ]; then
                echo "Failed to delete. Res = $res, Expected = $expected"
                kill_metax $p1 $p2
                exit;
        fi
        http_code=$(cat $test_dir/h | grep HTTP/1.1 | grep OK | awk {'print $2'})
        echo "HTTP code of DELETE request response is $http_code"
        if [[ 404 != $http_code ]]; then
                echo "HTTP code of DELETE request response is $http_code"
                kill_metax $p1 $p2
                exit;
        fi

        echo $PASS > $final_test_result
        kill_metax $p1 $p2
}

main $@
