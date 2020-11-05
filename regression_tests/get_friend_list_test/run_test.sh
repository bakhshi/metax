#!/bin/bash

# Test case flow: 
# - runs one Metax instance
# - gets friend list via get_friend_list API
# - it should contain a friend configured in friends.json, path is given in
#   config.xml
# - adds two new friends and checks whether get_friend_list returns added
#   friends


source ../../test_utils/functions.sh

# wait command exit code when metax was successfully finished (i.e. without Seg. fault or Assertion)
EXIT_CODE_OK=0


main()
{
        cp -f $tst_src_dir/friends.json $test_dir/
        $EXECUTE -f $tst_src_dir/config.xml &
        p=$!
        wait_for_init 7081
        echo -n $p > $test_dir/processes_ids
        res=$(curl http://localhost:7081/config/get_friend_list)
        key=$(echo $res | jq '[.[]|select(.name=="friend1")][0]' | jq -r '.public_key')
        echo "key1=$key"
        if [[ -z $key || null == $key ]]; then
                echo "friend list failed when there is one friend, friend_list=$res"
                kill_metax $p;
                exit;
        fi
        friend2_key="-----BEGIN RSA PUBLIC KEY-----
MIICCAKCAgEA4Ct4Bod4zorjs1XLAlfw3nnNNls3y8wgFxSJwrX8aMEP1yY4ES0n
5uVUK5vsWM5KRt3s4ORrs2JhLjvBjF5EpJBf1UK1MqGIEBULrVFkUo86Wpu8BNO6
6LDp2sQHOWvHmNoekPWTJVnZe7qlkg9oov28H8103Lp3oSXhv/7YVDHbyMOnCWwG
4blp53CY5WdmFj9MIZJoTx21JbC6+CHWQAJRXdXPVD4r9FgrEtKlwZnSEjJQoKIG
g6mMuCL+/reUVcirn5nCkwJdCByLvatxkrjf2bJSnnxm/HLYsUNPKgSlM02ayKi+
h80z7/3/rdOnqCKRE6ltH/CaPzWyLIl8pB5u3oexlTChO2a5mrzYjhqSIZezlbV4
dJm5SoczdBmNh4DSy/lMy2HRjWEJfrHsFMRT/vsHIzHEaAgo5wxVxiRFbwO2CJlz
zcX4HZI8LzHHsHo4iCaBesPt7cheZQxYZ2be90/YnAtYzs6Jkd45YU9uSYWqNZ31
cIZ/z+LKb64fLa7u+KarYFOQFrA8ma7oMGLbAArFzA5/KgsrGVdcafZzsM4NSJcb
NDk7ngoQjxeKN3mhzRZ7KIQQsO5yCiI+ytaTh7F1pxS15C52IP0V8eFon7Ay++ld
Z8tzfj/ynKKu0iCILTl5plVX+z6r/DlB8zJ73rnU2BH6xTjH3Evd+6UCAQM=
-----END RSA PUBLIC KEY-----"
        curl -F"key=$friend2_key" -F "username=friend2" "http://localhost:7081/config/add_friend"
        res=$(curl http://localhost:7081/config/get_friend_list)
        key=$(echo $res | jq '[.[]|select(.name=="friend2")][0]' | jq -r '.public_key')
        echo "key2=$key"

        if [ "$key" != "$friend2_key" ]; then
                echo "friend list failed when there are two friends, friends list: $res"
                kill_metax $p;
                exit;
        fi
        friend3_key="-----BEGIN RSA PUBLIC KEY-----
MIICCAKCAgEA4Ct4Bod4zorjs1XLAlfw3nnNNls3y8wgFxSJwrX8aMEP1yY4ES0n
5uVUK5vsWM5KRt3s4ORrs2JhLjvBjF5EpJBf1UK1MqGIEBULrVFkUo86Wpu8BNO6
6LDp2sQHOWvHmNoekPWTJVnZe7qlkg9oov28H8103Lp3oSXhv/7YVDHbyMOnCWwG
4blp53CY5WdmFj9MIZJoTx21JbC6+CHWQAJRXdXPVD4r9FgrEtKlwZnSEjJQoKIG
g6mMuCL+/reUVcirn5nCkwJdCByLvatxkrjf2bJSnnxm/HLYsUNPKgSlM02ayKi+
h80z7/3/rdOnqCKRE6ltH/CaPzWyLIl8pB5u3oexlTChO2a5mrzYjhqSIZezlbV4
dJm5SoczdBmNh4DSy/lMy2HRjWEJfrHsFMRT/vsHIzHEaAgo5wxVxiRFbwO2CJlz
zcX4HZI8LzHHsHo4iCaBesPt7cheZQxYZ2be90/YnAtYzs6Jkd45YU9uSYWqNZ31
cIZ/z+LKb64fLa7u+KarYFOQFrA8ma7oMGLbAArFzA5/KgsrGVdcafZzsM4NSJcb
NDk7ngoQjxeKN3mhzRZ7KIQQsO5yCiI+ytaTh7F1pxS15C52IP0V8eFon7Ay++ld
Z8tzfj/ynKKu0iCILTl5plVX+z6r/DlB8zJ73rnU2BH6xTjH3Evd+6UCAQM=
-----END RSA PUBLIC KEY-----"
        curl -F"key=$friend3_key" -F "username=friend3" "http://localhost:7081/config/add_friend"
        res=$(curl http://localhost:7081/config/get_friend_list)
        key=$(echo $res | jq '[.[]|select(.name=="friend3")][0]' | jq -r '.public_key')
        echo "key3=$key"
        if [ "$key" != "$friend3_key" ]; then
                echo "friend list failed when there are three friends"
                kill_metax $p;
                exit;
        fi
        echo $PASS > $final_test_result
        kill_metax $p;
}

main $@
