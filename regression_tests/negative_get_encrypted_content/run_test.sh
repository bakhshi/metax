#!/bin/bash

# Test case flow:
# - Runs and connects two instances of metax with config1.xml and config2.xml
#   configurations (the peers have different device/user keys).
# - Sends "node" save request to the first metax (config1.xml) for some content
#   (default it is saved as encrypted).
# - Sends get request to the second running metax (config2.xml) for uuid, which
#   has been saved in the first metax.
# - Checks the compliance of received/expected messages.


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
        echo -n $p2 $p1 > $test_dir/processes_ids
        save_content="Some content to save in peer1."
        uuid=$(curl --data "$save_content" http://localhost:7081/db/save/node)
        uuid=$(echo $uuid | cut -d'"' -f 4)
        sleep $TIME
        expected="{\"error\":\"Getting file failed: uuid=$uuid. Master object is not valid or encrypted but key not found.\"}"
        res=$(curl -D $test_dir/h http://localhost:7091/db/get?id=$uuid)
        echo $res
        if [ "$res" != "$expected" ]; then
                echo "Mismatch of expected/received messages. Res = $res, Rxpected = $expected"
                kill_metax $p1 $p2
                exit;
        fi
        http_code=$(cat $test_dir/h | grep HTTP/1.1 | awk {'print $2'})
        echo "HTTP code of GET request response is $http_code"
        if [[ 404 != $http_code ]]; then
                echo "HTTP code of GET request response is $http_code"
                kill_metax $p1 $p2
                exit;
        fi
        echo $PASS > $final_test_result
        kill_metax $p1 $p2
}

main $@

# vim:et:tabstop=8:shiftwidth=8:cindent:fo=croq:textwidth=80:
