#!/bin/bash

# Test case flow:
# - Runs an instance of metax with config.xml configurations.
# - Sends "node" save request to metax (config.xml) for some content.
# - Sends share request with invalid public key.
# - Checks the compliance of received/expected messages.


source ../../test_utils/functions.sh

# wait command exit code when metax was successfully finished (i.e. without Seg. fault or Assertion)
EXIT_CODE_OK=0



main()
{
        $EXECUTE -f $tst_src_dir/config.xml &
        p=$!
        wait_for_init 7081
        echo -n $p > $test_dir/processes_ids

        save_content="Some content for share test"
        uuid=$(curl --data "$save_content" http://localhost:7081/db/save/node)
        uuid=$(echo $uuid | cut -d'"' -f 4)
        expected='Reencryption failed for the request with public key'
        res=$(curl -D $test_dir/h -m 10 -F "key=-----BEGIN RSA PUBLIC KEY-----
        MIICCAKCAgEA8ej4CBFiQmrR/POX5Z1PXl+KemhEijV6ntbei/E6af2sG7QroUWR
        TAT/pP5z/zYjsl+4rHDjdfHRC579tGj8RhH/Dw/ebCkccLoT9iZ41VqLQAobMQW8" http://localhost:7081/db/share?id=$uuid)
        echo $res
        msg=$(echo $res | jq -r '.error')
        if [ "$expected" != "$msg" ]; then
                echo "Mismatch of expected/received messages. Msg = $msg, Rxpected = $expected"
                kill_metax $p
                exit
        fi
        http_code=$(cat $test_dir/h | grep HTTP/1.1 | grep OK | awk {'print $2'})
        echo "HTTP code is $http_code"
        if [ "400" != "$http_code" ]; then
                echo "HTTP code of SHARE request response is $http_code"
                kill_metax $p
                exit;
        fi
        echo $PASS > $final_test_result
        kill_metax $p
}

main $@

# vim:et:tabstop=8:shiftwidth=8:cindent:fo=croq:textwidth=80:
