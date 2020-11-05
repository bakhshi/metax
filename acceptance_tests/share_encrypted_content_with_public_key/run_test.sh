#!/bin/bash

# Test case flow:
# - Runs and connects two instance of metax with config1.xml and config2.xml
#   configurations.
# - Sends websocket connect request to second running metax (config2.xml) by
#   running websocket_client.js nodejs script.
# - Sends "node" save request to first metax (config1.xml) for some content
#   (should be saved as encrypted).
# - Sends share request to the second running metax with public key.
# - Checks the compliance of received/expected messages.
# - Checks the compliance of received/shared uuids.
# - Checks the correctness of origin uuid.
# - Sends get request to the second running metax to receive the shared content.
# - Checks the compliance of saved/shared contents.


source ../../test_utils/functions.sh

# wait command exit code when metax was successfully finished (i.e. without Seg. fault or Assertion)
EXIT_CODE_OK=0


start_metaxes()
{
        $EXECUTE -f $tst_src_dir/config2.xml &
        p2=$!
        is_peer_connected=$(peers_connect 7091 $tst_src_dir/config2.xml)
        if [ "0" != "$is_peer_connected" ];then
                echo "7090 does not connect to peers"
                kill_metax $p2
                exit;
        fi

        $EXECUTE -f $tst_src_dir/config1.xml &
        p1=$!
        is_peer_connected=$(peers_connect 7081 $tst_src_dir/config1.xml)
        if [ "0" != "$is_peer_connected" ];then
                echo "7080 does not connect to peers"
                kill_metax $p1 $p2
                exit;
        fi
        echo -n $p2 $p1 > $test_dir/processes_ids
}


main()
{
        start_metaxes
        ave_content="Some content for share test"
        uuid=$(curl --data "$save_content" http://localhost:7081/db/save/node)
        uuid=$(echo $uuid | cut -d'"' -f 4)
        res=$(curl -D $test_dir/h -m 10 -F "key=-----BEGIN RSA PUBLIC KEY-----
MIICCAKCAgEAyfa17c+FHn2JSGf/ZUAgk9KzxnrtvCjWa7FwQp0GuTxTrNwEAB8W
JDUCYJRAtmaANzI+/jaX9I2bqKSB0498ELkZI0kuQcsdtt/1jqfPVPmQvADK0b/x
+CexSXlf48CVJQjgjdkrxBD83l0P9FLhFZAJPtYSLDym6RvF2802dkeT2En6Xvnh
XUYoOnuH5VEO42Hmdsu2uW/VN/jFuFVcdJQS8r4fyq1dyxvbz0Yf4h1weyhrAhUL
5Ei/32qnJfhbwRb1Ff2y4Y8UXlHnnmVYv7LrfUBYzfWjdUizuPb61D0jLgFBN5zK
siQ+yTg3mOHG0Naphuof2EY5uWz9lHRVabIkklmmQj+/gcAU6nG8qNF2K0kHpxgP
YciZmNCyq0oGMiAAWDX8FZ8C76NkAJWjA6JvYiytIsnnAtkoGfPVBReEJAERw9bn
2/7vfCzBNcNhkA/FcLlhgGW9bVBDuD/FIxDSRja7Uvqx11k/RLqN3yXVMUw3uF1r
WpIgndXdmgA045a0hqJNkik5Si1OiX1RMVI+7PnP+JoOo6ErZ84gcyPWahiuNo01
FC8j8ROFflM1sGdxIvOwYbVR95WyzU2dionaXy1cNCGr9O4u0BctO2UCOGmxa2Ap
h07ffPrH7P/UixnMgkQVpbN5dXzay2WZS9HfW4xCotViDePFG7PEEE0CAQM=
-----END RSA PUBLIC KEY-----
" http://localhost:7081/db/share?id=$uuid)
        echo "Res=$res"
        http_code=$(cat $test_dir/h | grep HTTP/1.1 | grep OK | awk {'print $2'})
        if [[ 200 != $http_code ]]; then
                echo "HTTP code of SHARE request response is $http_code"
                kill_metax $p1 $p2
                exit;
        fi
	share_uuid=$(echo $res | jq -r '.share_uuid')
        key=$(echo $res | jq -r '.key')
        iv=$(echo $res | jq -r '.iv')
        if [ "$share_uuid" != "$uuid" ]; then
                echo "Share uuid is not same"
                kill_metax $p1 $p2;
                exit;
        fi
        if [[ $key == null ]] || [[ $iv == null ]]; then
                echo "Key and iv are not returned"
                kill_metax $p1 $p2;
                exit;
        fi
        if [[ "$key" == "" ]] || [[ "$iv" == "" ]]; then
                echo "Key and iv are not empty"
                kill_metax $p1 $p2;
                exit;
        fi
        received=$(curl -D $test_dir/h -m 10 -F "key=$key" -F "iv=$iv" http://localhost:7091/db/accept_share?id=$uuid)
        echo "Res=$received"
        http_code=$(cat $test_dir/h | grep HTTP/1.1 | grep OK | awk {'print $2'})
        if [[ 200 != $http_code ]]; then
                echo "HTTP code of ACCEPT_SHARE request response is $http_code"
                kill_metax $p1 $p2
                exit;
        fi
        expected='{"share":"accepted"}'
        if [ "$expected" != "$received" ]; then
                echo "Share accept failed"
                kill_metax $p1 $p2;
                exit;
        fi
        share_info=$(curl http://localhost:7091/db/get?id=$share_uuid)
        if [ "$share_info" != "$save_content" ]; then
                echo "received incorrect shared data"
                kill_metax $p1 $p2;
                exit;
        fi
        echo $PASS > $final_test_result
        kill_metax $p1 $p2
}

main $@
