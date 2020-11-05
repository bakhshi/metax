#!/bin/bash

# Test case flow:
# - Runs an instance of metax with config.xml configurations.
# - Sends get request with uuid, which not exist.
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

        expected='{"error":"Getting file failed: uuid=c27b9cbf-c3ba-4207-9539-e28fd625d542."}'
        res=$(curl -D $test_dir/h http://localhost:7081/db/get?id=c27b9cbf-c3ba-4207-9539-e28fd625d542)
        echo $res
        if [ "$expected" != "$res" ]; then
                echo "GET response and expected message mismatch. Res = $res"
                kill_metax $p
                exit;
        fi

        echo $PASS > $final_test_result
        http_code=$(cat $test_dir/h | grep HTTP/1.1 | awk {'print $2'})
        echo "HTTP code of GET request response is $http_code"
        if [[ 404 != $http_code ]]; then
                echo "HTTP code of GET request response is $http_code"
                kill_metax $p
                exit;
        fi
        kill_metax $p
}

main $@

# vim:et:tabstop=8:shiftwidth=8:cindent:fo=croq:textwidth=80:
