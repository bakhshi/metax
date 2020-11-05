#!/bin/bash

# Test case flow:
# - Runs and connects two instances of metax (peer1 [size_limit = 0]-> peer2).
# - Send save non encrypted stream request to peer2.
# - Send recording start to peer1 (The recording should not match).

source ../../test_utils/functions.sh

# wait command exit code when metax was successfully finished (i.e. without Seg. fault or Assertion)
EXIT_CODE_OK=0
random_file=$test_dir/random_file
save_file=$test_dir/save_file

start_metaxes()
{
        $EXECUTE -f $tst_src_dir/config2.xml &
        p2=$!
        is_peer_connected=$(peers_connect 7091 $tst_src_dir/config2.xml)
        if [ "0" != "$is_peer_connected" ]; then
                echo "7090 does not connect to peer"
                kill_metax $p2
                exit;
        fi
        $EXECUTE -f $tst_src_dir/config1.xml &
        p1=$!
        is_peer_connected=$(peers_connect 7081 $tst_src_dir/config1.xml)
        if [ "0" != "$is_peer_connected" ]; then
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
        #Generated ~500KB file
        dd if=/dev/urandom of=$save_file bs=100000 count=15
        uuid="283abccd-8c1e-47c1-8795-839abcfede5c"

        g++ -std=c++11 $tst_src_dir/livestream_maker.cpp -lPocoFoundation  -lPocoNet -o exe
        ./exe 7095 $save_file &
        exe_pid=$!
        sleep 1

        curl "http://localhost:7091/db/save/stream?url=127.0.0.1:7095&id=$uuid&enc=0" &
        cpid=$!
        sleep 1

        ruuid=$(curl -D $test_dir/h "http://localhost:7081/db/recording/start?livestream=$uuid")
        ruuid=$(echo $ruuid | cut -d'"' -f 4)
        echo "ruuid = $ruuid"
        http_code=$(cat $test_dir/h | grep HTTP/1.1 | awk {'print $2'})
        echo "HTTP code of RECORDING_START_STREAM1 request response is $http_code"
        expected="Failed to save: No space in storage"
        wait $cpid
        if [ "$expected" != "$ruuid" ] || [[ 400 != $http_code ]]; then
                echo "The recording should not match. Storage size of peer1 is 0"
                kill $exe_pid
                wait $exe_pid
                kill_metax $p1 $p2
                exit;
        fi
        echo $PASS > $final_test_result
        kill $exe_pid
        wait $exe_pid
        kill_metax $p1 $p2
}

main $@
