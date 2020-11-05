#!/bin/bash

# Test case flow:
# - TODO

save_file=$test_dir/save_file
get_file=$test_dir/get_file

source ../../test_utils/functions.sh

# wait command exit code when metax was successfully finished (i.e. without Seg. fault or Assertion)
EXIT_CODE_OK=0


main()
{
        $EXECUTE -f $tst_src_dir/config.xml &
        p1=$!
        wait_for_init 7081
        echo -n $p1 > $test_dir/processes_ids

        copy_res=$(curl -D $test_dir/h http://localhost:7081/db/copy?id=5a1f3657-a06c-471b-8a1a-d00d9fff8967)
        echo "Copy request response - $copy_res"
        expected='{"error":"Failed to copy."}'
        if [ "$expected" != "$copy_res" ]; then
                echo "Received invalid response. copy_res = $copy_res, expected = $expected"
                kill_metax $p1
                exit;
        fi
        http_code=$(cat $test_dir/h | grep HTTP/1.1 | grep OK | awk {'print $2'})
        echo "HTTP code of COPY request response is $http_code"
        if [[ 404 != $http_code ]]; then
                echo "HTTP code of COPY request response is $http_code"
                kill_metax $p1
                exit;
        fi
        echo $PASS > $final_test_result
        kill_metax $p1
}

main $@

# vim:et:tabstop=8:shiftwidth=8:cindent:fo=croq:textwidth=80:
