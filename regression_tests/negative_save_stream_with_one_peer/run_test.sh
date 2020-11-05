#!/bin/bash

#Test failes because of bug #9391

# Test case flow:
# - Runs and connects two instances of metax with each other (metax1 -> metax2).
# - Send save stream request to metax1.
# - Send get request to metax2.
# - Check get request response.

source ../../test_utils/functions.sh

# wait command exit code when metax was successfully finished (i.e. without Seg. fault or Assertion)
EXIT_CODE_OK=0
save_file=$test_dir/save_file




main()
{
        if [[ "Darwin" == "$target" ]]; then
                echo $SKIP > $final_test_result
                exit
        fi
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

        #Generated ~500KB file
        dd if=/dev/urandom of=$save_file bs=100000 count=5
        echo "du -sh save_file `du -sh $save_file `"
        uuid="283abccd-8c1e-47c1-8795-839abcfede5c"

        g++ -std=c++11 $tst_src_dir/livestream_maker.cpp -lPocoFoundation  -lPocoNet -o exe
        ./exe 7095 $save_file &
        exe_pid=$!
        sleep 1

        res=$(curl -D $test_dir/h "http://localhost:7081/db/save/stream?url=127.0.0.1:7095&id=$uuid&enc=0")
        kill $exe_pid
        wait $exe_pid
        echo "Save stream res = $res"
        expected='{"error":"Failed to save stream"}'
        if [ "$expected" != "$res" ];then
                echo "Invalid response for save request. Expected = $expected, Res = $res"
                kill_metax $p1 $p2
                exit;
        fi
        http_code=$(cat $test_dir/h | grep HTTP/1.1 | awk {'print $2'})
        echo "HTTP code of SAVE stream request response is $http_code"
        if [[ 410 != $http_code ]]; then
                echo "HTTP code of SAVE stream request response is $http_code"
                kill_metax $p1 $p2
                exit;
        fi

        echo $PASS > $final_test_result
        kill_metax $p1 $p2
}

main $@

