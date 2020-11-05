#!/bin/bash

# This test case added for #9572 ticket with Expected Fail status.

# Test case flow:
# - generate save_file file
# - use nc command to write for stream - nc -l localhost 7095 < save_file &;
# - sleep 2 seccond.
# - send save stream request to metax - curl "http://<host>:<port>/db/save/stream?url=127.0.0.1:7095&id=<uuid>"
# - wait 5 second
# - kill metax.
# - kill nc.

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
        $EXECUTE -f $tst_src_dir/config1.xml &
        p1=$!
        is_peer_connected=$(peers_connect 7081 $tst_src_dir/config1.xml)
        if [ "0" != "$is_peer_connected" ];then
                echo "7080 does not connect to peers"
                kill_metax $p2 $p1
                exit;
        fi
        echo -n $p2 $p1 > $test_dir/processes_ids

        #Generated ~300KB file
        dd if=/dev/urandom of=$save_file bs=100000 count=3
        echo "du -sh save_file `du -sh $save_file `"

        g++ -std=c++11 $tst_src_dir/livestream_maker.cpp -lPocoFoundation  -lPocoNet -o exe
        ./exe 7095 $save_file &
        exe_p=$!
        sleep 1
        curl "http://localhost:7081/db/save/stream?url=127.0.0.1:7095&id=283abccd-8c1e-47c1-8795-839abcfede5c"

        # kill metax-es and check exit code
        kill $p1
        wait $p1
        c1="$?"
        kill $p2
        wait $p2
        c2="$?"
        if [[ "$EXIT_CODE_OK" != "$c1" || "$EXIT_CODE_OK" != "$c2" ]]; then
                echo "Metax crashed: c1=$c1, c2=$c2"
                echo $CRASH > $final_test_result
        else
                echo $PASS > $final_test_result
        fi
        kill $exe_p
        wait $exe_p
        rm $save_file
}

main $@
