#!/bin/bash

# Test case flow:
# - Runs and connects two instances of metax with config1.xml and config2.xml
#   configurations.
# - Sends websocket connect request to second running metax (config2.xml) by
#   running websocket_client.js nodejs script.
# - sends a message to the second peer via public key
# - if websocket connection is up and the message successfully delivered then
#   the delivered message is checked with the original one


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
	cd $test_dir
        cp $tst_src_dir/websocket_client.js .
        npm install ws
	start_metaxes
        nodejs websocket_client.js $test_dir/data &
	sleep 2
	content="Some content for send to test"
	res=$(curl -D $test_dir/h -F "key=-----BEGIN RSA PUBLIC KEY-----
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
	" -F "data=$content" http://localhost:7081/db/sendto)
	echo "Response is $res"
        http_code=$(cat $test_dir/h | grep HTTP/1.1 | grep OK | awk {'print $2'})
        if [[ 200 != $http_code ]]; then
                echo "HTTP code of SENDTO request response is $http_code"
                kill_metax $p1 $p2
                exit;
        fi
	expected='{"msg":"Data is delivered successfully"}'
	if [ "$res" != "$expected" ]; then
		echo "Failed to deliver data"
		kill_metax $p1 $p2;
		exit;
	fi
	sleep 2
	data=$(cat $test_dir/data)
	if [ "$data" != "$content" ]; then
		echo "Received incorrect data"
		kill_metax $p1 $p2
		exit;
	fi

	echo $PASS > $final_test_result
	kill_metax $p1 $p2
}

main $@
