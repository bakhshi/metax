#!/bin/bash

# Test case flow:
# starts one instance of Metax
# sends "sendto" request
#	- public key is valid
#	- data parameter is missing
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
        " http://localhost:7081/db/sendto)
        echo $res
        expected='{"error": "Exception: Data for sending is not specified for the request with public key"}'
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
