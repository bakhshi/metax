#!/bin/bash

# Test case flow:
# - Runs an instance of metax with config.xml.
# - Sends get request with uuid, which is not in valid format.
# - Checks the compliance of received and expected messages.


source ../../test_utils/functions.sh

# wait command exit code when metax was successfully finished (i.e. without Seg. fault or Assertion)
EXIT_CODE_OK=0



main()
{
        $EXECUTE -f $tst_src_dir/config.xml &
        p=$!
        wait_for_init 7081
        echo -n $p > $test_dir/processes_ids

        uuid=16022337-411b-4c5d-9fd3-7
        expected="{\"error\": \"Syntax error: $uuid\"}"
        res=$(curl -D $test_dir/h http://localhost:7081/db/get?id=$uuid)
        echo $res
        if [ "$res" != "$expected" ]; then
                echo "Mismatch of expected/received messages. Res = $res, Rxpected = $expected"
                kill_metax $p
                exit;
        fi
        http_code=$(cat $test_dir/h | grep HTTP/1.1 | awk {'print $2'})
        echo "HTTP code of GET request response is $http_code"
        if [[ 400 != $http_code ]]; then
                echo "HTTP code of GET request response is $http_code"
                kill_metax $p
                exit;
        fi
        echo $PASS > $final_test_result
        kill_metax $p
}

main $@

# vim:et:tabstop=8:shiftwidth=8:cindent:fo=croq:textwidth=80:
