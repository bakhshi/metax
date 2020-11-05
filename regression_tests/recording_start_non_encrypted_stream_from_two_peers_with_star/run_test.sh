#!/bin/bash

# Test case flow:
# - Runs and connects three instances of metax (peer1 -> peer2, peer3 -> peer2).
# - Send save non encrypted stream request to peer1.
# - Send recording start to peer2.
# - Send recording start to peer3.
# - Send get request to peer2 and peer3 to check get requests responses.

source ../../test_utils/functions.sh

# wait command exit code when metax was successfully finished (i.e. without Seg. fault or Assertion)
EXIT_CODE_OK=0


random_file=$test_dir/random_file
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
                echo "7090 does not connect to peer"
                kill_metax $p2
                exit;
        fi
        $EXECUTE -f $tst_src_dir/config3.xml &
        p3=$!
        is_peer_connected=$(peers_connect 7101 $tst_src_dir/config3.xml)
        if [ "0" != "$is_peer_connected" ];then
                echo "7100 does not connect to peer"
                kill_metax $p3 $p2
                exit;
        fi
        $EXECUTE -f $tst_src_dir/config1.xml &
        p1=$!
        is_peer_connected=$(peers_connect 7081 $tst_src_dir/config1.xml)
        if [ "0" != "$is_peer_connected" ];then
                echo "7080 does not connect to peers"
                kill_metax $p3 $p2 $p1
                exit;
        fi
        echo -n $p3 $p2 $p1 > $test_dir/processes_ids

        #Generated ~500KB file
        dd if=/dev/urandom of=$save_file bs=100000 count=5
        echo "du -sh save_file `du -sh $save_file `"
        uuid="283abccd-8c1e-47c1-8795-839abcfede5c"

        g++ -std=c++11 $tst_src_dir/livestream_maker.cpp -lPocoFoundation  -lPocoNet -o exe
        ./exe 7095 $save_file &
        exe_pid=$!
        sleep 1

        curl "http://localhost:7081/db/save/stream?url=127.0.0.1:7095&id=$uuid&enc=0"&
        cpid=$!
        sleep 1

        ruuid=$(curl -D $test_dir/h "http://localhost:7091/db/recording/start?livestream=$uuid")
        ruuid=$(echo $ruuid | cut -d'"' -f 4)
        echo ruuid = $ruuid
        http_code=$(cat $test_dir/h | grep HTTP/1.1 | awk {'print $2'})
        if [[ 200 != $http_code ]]; then
                echo "HTTP code of RECORDING_START_STREAM request response is $http_code"
                kill $exe_pid
                wait $exe_pid
                kill_metax $p1 $p2 $p3
                exit;
        fi
        ruuid1=$(curl "http://localhost:7101/db/recording/start?livestream=$uuid")

        ruuid1=$(echo $ruuid1 | cut -d'"' -f 4)
        echo ruuid1 = $ruuid1

        wait $cpid
        sleep 1
        response=$(curl "http://localhost:7091/db/recording/stop?livestream=$uuid&recording=$ruuid1")
        echo "recording stop response $response"
        sleep 1
        response=$(curl "http://localhost:7091/db/recording/stop?livestream=$uuid&recording=$ruuid")
        echo "recording stop response $response"
        sleep 2

        curl "http://localhost:7091/db/get?id=$ruuid" --output $test_dir/get_file
        cat $test_dir/get_file
        rec_url=$(grep "/db/get?id=" $test_dir/get_file)
        echo "getting recorded livestream from peer2 by $rec_url url"

        curl "http://localhost:7091$rec_url" --output $test_dir/get_rec_file

        curl "http://localhost:7101/db/get?id=$ruuid1" > $test_dir/get_file
        cat $test_dir/get_file
        rec_url=$(grep "/db/get?id=" $test_dir/get_file)
        echo "getting recorded livestream from peer3 by $rec_url uuid"

        curl "http://localhost:7101$rec_url" --output $test_dir/get_rec_file1

        tail --byte 35 $save_file > $test_dir/save_last_line
        tail --byte 35 $test_dir/get_rec_file > $test_dir/get1_last_line
        tail --byte 35 $test_dir/get_rec_file1 > $test_dir/get2_last_line
        diff1=$(diff $test_dir/save_last_line $test_dir/get1_last_line)
        diff2=$(diff $test_dir/save_last_line $test_dir/get2_last_line)
        if [ -n "$diff1" ] || [ -n "$diff2" ];then
                echo "The recording does not match"
                kill $exe_pid
                wait $exe_pid
                kill_metax $p1 $p2 $p3
                exit;
        fi

        echo $PASS > $final_test_result
        kill $exe_pid
        wait $exe_pid
        kill_metax $p1 $p2 $p3
}

main $@
