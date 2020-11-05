#!/bin/bash

# Test case flow:
# - Runs an instance of metax with config.xml configurations.
# - Sends "send_to" request with invalid public key.
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

        expected='{"error":"Failed to deliver data to peer for the request with public key"}'
        res=$(curl -D $test_dir/h -F "key=-----BEGIN RSA PUBLIC KEY-----
        MIICCAKCAgEA8ej4CBFiQmrR/POX5Z1PXl+KemhEijV6ntbei/E6af2sG7QroUWR
        TAT/pP5z/zYjsl+4rHDjdfHRC579tGj8RhH/Dw/ebCkccLoT9iZ41VqLQAobMQW8
        " -F "data=content" http://localhost:7081/db/sendto)
        echo $res
        if [ "$expected" != "$res" ]; then
                echo "Received invalid response. Res = $res, Expected = $expected"
                kill_metax $p
                exit;
        fi
        http_code=$(cat $test_dir/h | grep HTTP/1.1 | grep OK | awk {'print $2'})
        echo "HTTP code of SENDTO request response is $http_code"
        if [[ 400 != $http_code ]]; then
                echo "HTTP code of SENDTO request response is $http_code"
                kill_metax $p
                exit;
        fi
        echo $PASS > $final_test_result
        kill_metax $p
}

main $@

# vim:et:tabstop=8:shiftwidth=8:cindent:fo=croq:textwidth=80:
