#!/bin/bash

# Test case flow:
# - Runs one instance of metax.
# - Sends register listener request for missing uuid.
# - Checks that register listener request fails because the uuid is missing.

source ../../test_utils/functions.sh

# wait command exit code when metax was successfully finished (i.e. without Seg. fault or Assertion)
EXIT_CODE_OK=0


main()
{
        $EXECUTE -f $tst_src_dir/config1.xml &
        p1=$!
        wait_for_init 7081
        echo -n $p1 > $test_dir/processes_ids

        # register for missing uuid.
        uuid='1efd1163-c4e0-4f27-896e-6c0889970ed4'
        res=$(curl -D $test_dir/h http://localhost:7081/db/register_listener?id=$uuid)
        expected='{"error": "Failed to register listener for: '$uuid.'"}'
        echo "Received $res"
        if [[ "$res" != "$expected" ]]; then
                echo "Mismatch of expected and received response. Res = $res, expected = $expected"
                kill_metax $p1
                exit
        fi
        http_code=$(cat $test_dir/h | grep HTTP/1.1 | grep OK | awk {'print $2'})
        echo "HTTP code of REGISTER_LISTENER request response is $http_code"
        if [[ 400 != $http_code ]]; then
                echo "HTTP code of REGISTER_LISTENER request response is $http_code"
                kill_metax $p1
                exit;
        fi

        echo $PASS > $final_test_result
        kill_metax $p1
}

main $@
