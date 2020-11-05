#!/bin/bash

# Test case flow:
# - Runs and connects two instances of metax with each other (metax1 -> metax2).
# - Send recording start to metax2 when no livestream found with the specified uuid.
# - Check start recording request response.


source ../../test_utils/functions.sh

# wait command exit code when metax was successfully finished (i.e without Seg. fault or Assertion)
EXIT_CODE_OK=0


start_metaxes()
{
        $EXECUTE -f $tst_src_dir/config.xml &
        p=$!
        is_peer_connected=$(peers_connect 7081 $tst_src_dir/config.xml)
        if [ "0" != "$is_peer_connected" ];then
                echo "7080 does not connect to peers"
                kill_metax $p
                exit;
        fi
        echo -n $p > $test_dir/processes_ids
}

main()
{
        if [[ "Darwin" == "$target" ]]; then
                echo $SKIP > $final_test_result
                exit
        fi
        start_metaxes
        invalid_uuid="222aaaaa-8c1e-47c1-8795-839abcfede5c"

        res=$(curl -D $test_dir/h "http://localhost:7081/db/recording/start?livestream=$invalid_uuid")
        echo "res = $res"
        expected="{\"error\":\"livestream not found: $invalid_uuid\"}"
        if [ "$expected" != "$res" ];then
                echo "Invalid response for recording_start request. Expected = $expected, Res = $res"
                kill_metax $p
                exit;
        fi
        http_code=$(cat $test_dir/h | grep HTTP/1.1 | awk {'print $2'})
        if [[ 400 != $http_code ]]; then
                echo "HTTP code of SAVE stream request response is $http_code"
                kill_metax $p
                exit;
        fi

        echo $PASS > $final_test_result
        kill_metax $p
}

main $@
