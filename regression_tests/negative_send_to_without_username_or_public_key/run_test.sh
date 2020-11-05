#!/bin/bash

# Test case flow:
# starts one instance of Metax
# sends "sendto" request
#       - username or key field is empty
# checks for the corresponding error message


source ../../test_utils/functions.sh

# wait command exit code when metax was successfully finished (i.e. without Seg. fault or Assertion)
EXIT_CODE_OK=0



main()
{
        $EXECUTE -f $tst_src_dir/config1.xml &
        p1=$!
        wait_for_init 7081
        echo -n $p1 > $test_dir/processes_ids

        res=$(curl -D $test_dir/h -F "data=content" http://localhost:7081/db/sendto)
        expected='{"error": "Exception: Username or key is not specified"}'
        echo $res
        if [ "$res" != "$expected" ]; then
                echo "Received invalid response. Res = $res, Expected = $expected"
                kill_metax $p1;
                exit;
        fi
        http_code=$(cat $test_dir/h | grep HTTP/1.1 | grep OK | awk {'print $2'})
        echo "HTTP code of SENDTO request response is $http_code"
        if [[ 400 != $http_code ]]; then
                echo "HTTP code of SENDTO request response is $http_code"
                kill_metax $p1
                exit;
        fi
        echo $PASS > $final_test_result
        kill_metax $p1
}

main $@

# vim:et:tabstop=8:shiftwidth=8:cindent:fo=croq:textwidth=80:
