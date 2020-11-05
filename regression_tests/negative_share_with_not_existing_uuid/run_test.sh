#!/bin/bash

# Test case flow:
# - Runs an instance of metax with config.xml configurations.
# - Sends share request with uuid, which not exist.
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

        expected="Fail to share uuid=c27b9cbf-c3ba-4207-9539-e28fd625d542 for the request with public key"
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
" http://localhost:7081/db/share?id=c27b9cbf-c3ba-4207-9539-e28fd625d542)
        echo $res
        msg=$(echo $res | jq -r '.error')
        if [ "$expected" != "$msg" ]; then
                echo "Mismatch of expected/received messages. Res = $res, Rxpected = $expected"
                kill_metax $p
                exit;
        fi
        http_code=$(cat $test_dir/h | grep HTTP/1.1 | grep OK | awk {'print $2'})
        echo "HTTP code of SHARE request response is $http_code"
        if [[ 400 != $http_code ]]; then
                echo "HTTP code of SHARE request response is $http_code"
                kill_metax $p
                exit;
        fi
        echo $PASS > $final_test_result
        kill_metax $p
}

main $@

# vim:et:tabstop=8:shiftwidth=8:cindent:fo=croq:textwidth=80:
