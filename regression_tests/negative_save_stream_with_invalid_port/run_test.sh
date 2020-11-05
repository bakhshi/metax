#!/bin/bash

#Test failes because of bug #9391

# Test case flow:
# - Runs and connects two instances of metax with each other (metax1 -> metax2).
# - Send save stream request with invalid_port to metax1.
# - Check save stream response.

source ../../test_utils/functions.sh

# wait command exit code when metax was successfully finished (i.e. without Seg. fault or Assertion)
EXIT_CODE_OK=0
save_file=$test_dir/save_file


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
                kill_metax $p2 $p1
                exit;
        fi
        echo -n $p2 $p1 > $test_dir/processes_ids
}

main()
{
        if [[ "Darwin" == "$target" ]]; then
                echo $SKIP > $final_test_result
                exit
        fi
        start_metaxes
        uuid="283abccd-8c1e-47c1-8795-839abcfede5c"

        res=$(curl -D $test_dir/h "http://localhost:7081/db/save/stream?url=localhost:7085&id=$uuid&enc=0")
        echo "Save stream res = $res"
        expected="Connection refused"
        if [[ ! "$res" =~ "$expected" ]];then
                echo "Invalid response for save request. Expected = $expected, Res = $res"
                kill_metax $p1 $p2
                exit;
        fi

        http_code=$(cat $test_dir/h | grep HTTP/1.1 | awk {'print $2'})
        if [[ 400 != $http_code ]]; then
                echo "HTTP code of SAVE stream request response is $http_code"
                kill_metax $p1 $p2
                exit;
        fi

        echo $PASS > $final_test_result
        kill_metax $p1 $p2
}

main $@

