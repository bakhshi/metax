#!/bin/bash

# Test case flow:
# - Runs one instance of Metax.
# - Sends share request with missing uuid parameter.
# - Checks the compliance of expected/received error messages.


source ../../test_utils/functions.sh

# wait command exit code when metax was successfully finished (i.e. without Seg. fault or Assertion)
EXIT_CODE_OK=0



main()
{
        $EXECUTE -f $tst_src_dir/config1.xml &
        p1=$!
        wait_for_init 7081
        echo -n $p1 > $test_dir/processes_ids

        expected='{"error": "Exception: File for sharing is not specified for the request with peer2"}'
        res=$(curl -D $test_dir/h -m 10 -F "username=peer2" http://localhost:7081/db/share)
        echo $res
        if [ "$expected" != "$res" ]; then
                echo "Mismatch of expected/received messages. Res = $res, Rxpected = $expected"
                kill_metax $p1
                exit;
        fi
        http_code=$(cat $test_dir/h | grep HTTP/1.1 | grep OK | awk {'print $2'})
        echo "HTTP code of SHARE request response is $http_code"
        if [[ 400 != $http_code ]]; then
                echo "HTTP code of SHARE request response is $http_code"
                kill_metax $p1
                exit;
        fi
        echo $PASS > $final_test_result;
        kill_metax $p1
}

main $@
