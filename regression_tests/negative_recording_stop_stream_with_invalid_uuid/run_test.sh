#!/bin/bash

# Test case flow:
# - Runs and connects two instances of metax with each other (peer1 -> peer2).
# - Send save stream request to metax1 (enc = 0).
# - Send record an existing livestream in metax2 storage.
# - Sends stop recording when no livestream found with the specified luuid.
# - Sends stop recording when no recording found with the specified luuid.
# - Check stop recording request responses.

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

        #Generated ~10MB file
        dd if=/dev/urandom of=$save_file bs=1000000 count=10

        luuid="283abccd-8c1e-47c1-8795-839abcfede5c"
        invalid_uuid="222aaaaa-8c1e-47c1-8795-839abcfede5c"

        g++ -std=c++11 $tst_src_dir/livestream_maker.cpp -lPocoFoundation -lPocoNet -o exe
        ./exe 7095 $save_file &
        exe_pid=$!
        sleep 1

        curl "http://localhost:7081/db/save/stream?url=127.0.0.1:7095&id=$luuid&enc=0"&
        cpid=$!
        sleep 1

        ruuid=$(curl "http://localhost:7091/db/recording/start?livestream=$luuid")
        ruuid=$(echo $ruuid | cut -d'"' -f 4)
        echo "ruuid=$ruuid"
        sleep 3

        echo rec_stop1_start
        response=$(curl -D $test_dir/h "http://localhost:7091/db/recording/stop?livestream=$invalid_uuid&recording=$ruuid")
        echo "recording_stop response $response"
        expected="{\"error\":\"livestream not found: $invalid_uuid\"}"

        if [ "$response" != "$expected" ];then
                echo "Invalid response for recording_stop request. Expected = $expected, Res = $response"
                kill $exe_pid
                wait $exe_pid
                kill_metax $p1 $p2
                exit;
        fi

        http_code=$(cat $test_dir/h | grep HTTP/1.1 | awk {'print $2'})
        if [[ 400 != $http_code ]]; then
                echo "HTTP code of RECORDING_STOP_STREAM request response with invalid luuid is $http_code"
                kill $exe_pid
                wait $exe_pid
                kill_metax $p1 $p2
                exit;
        fi

        echo rec_stop2_start
        response=$(curl -D $test_dir/h "http://localhost:7091/db/recording/stop?livestream=$luuid&recording=$invalid_uuid")
        echo "recording_stop response $response"
        expected="{\"error\":\"no recording $invalid_uuid found for livestream $luuid\"}"
        if [ "$response" != "$expected" ];then
                echo "Invalid response for recording_stop request. Expected = $expected, Res = $response"
                kill $exe_pid
                wait $exe_pid
                kill_metax $p1 $p2
                exit;
        fi
        http_code=$(cat $test_dir/h | grep HTTP/1.1 | awk {'print $2'})
        if [[ 400 != $http_code ]]; then
                echo "HTTP code of RECORDING_STOP_STREAM request response with invalid ruuid is $http_code"
                kill $exe_pid
                wait $exe_pid
                kill_metax $p1 $p2
                exit;
        fi

        kill $exe_pid
        wait $exe_pid
        echo $PASS > $final_test_result
        kill_metax $p1 $p2
}

main $@
