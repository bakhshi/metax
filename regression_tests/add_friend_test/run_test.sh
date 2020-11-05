#!/bin/bash

# Test case flow:
# - runs two connected Metax peers 1 and 2
# - saves encrypted data in peer 1
# - shares with username of peer 2 (friend 2)
# - gets appropriate error message while public key of friend 2 is not available
#   for peer 1
# - add friend 2 public key using add_friend API
# - shares the saved uuid again
# - share successes
# - peer 2 gets and checks the saved data


source ../../test_utils/functions.sh

# wait command exit code when metax was successfully finished (i.e. without Seg. fault or Assertion)
EXIT_CODE_OK=0

main()
{
        rm -f $test_dir/friends1.json
        $EXECUTE -f $tst_src_dir/config2.xml &
	p2=$!
        is_peer_connected=$(peers_connect 7091 $tst_src_dir/config2.xml)
	$EXECUTE -f $tst_src_dir/config1.xml &
	p1=$!
        is_peer_connected=$(peers_connect 7081 $tst_src_dir/config1.xml)
        if [ "0" != "$is_peer_connected" ];then
                echo "7080 does not connect to peers"
                kill_metax $p2 $p1
                exit;
        fi
        echo -n $p2 $p1 > $test_dir/processes_ids
        save_content="Some content for share test"
        uuid=$(curl --data "$save_content" http://localhost:7081/db/save/node)
        uuid=$(echo $uuid | cut -d'"' -f 4)
        expected='Peer not found for the request with friend2'
        res=$(curl -m 10 -F "username=friend2" http://localhost:7081/db/share?id=$uuid)
        share=$(echo $res | jq -r '.error')
        echo $res
        if [ "$share" != "$expected" ]; then
                echo "unexpected result from share when friend isn't added"
                kill_metax $p1 $p2;
                exit;
        fi
        res=$(curl -F"key=
-----BEGIN RSA PUBLIC KEY-----
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
-----END RSA PUBLIC KEY-----
        " -F "username=friend2" "http://localhost:7081/config/add_friend")
        expected='Adding friend friend2 completed successfully.'
        add_friend_st=$(echo $res | jq -r '.success')
        echo $add_friend_st
        echo $expected
        if [ "$add_friend_st" != "$expected" ]; then
                echo "failed to add friend"
                kill_metax $p1 $p2;
                exit;
        fi
        res=$(curl -m 10 -F "username=friend2" http://localhost:7081/db/share?id=$uuid)
        share=$(echo $res | jq -r '.share_uuid')
        if [ "$share" != "$uuid" ]; then
                echo "unexpected result from share when the friend is added"
                kill_metax $p1 $p2;
                exit;
        fi

        key=$(echo $res | jq -r '.key')
        iv=$(echo $res | jq -r '.iv')
        res=$(curl -m 10 -F "key=$key" -F "iv=$iv" http://localhost:7091/db/accept_share?id=$uuid)
        expected='{"share":"accepted"}'
        if [ "$expected" != "$res" ]; then
                echo "Res=$res"
                echo "unexpected result from accept_share request"
                kill_metax $p1 $p2;
                exit;
        fi

        share_info=$(curl http://localhost:7091/db/get?id=$uuid)
        if [ "$share_info" != "$save_content" ]; then
                echo "received incorrect shared data"
                kill_metax $p1 $p2;
                exit;
        fi
        echo $PASS > $final_test_result
        kill_metax $p1 $p2;
}

main $@
