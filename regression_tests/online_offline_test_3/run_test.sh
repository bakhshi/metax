#!/bin/bash

# Test case flow:
# - Start metax with no peers
# - Connect to metax port by using telnet command.
# - check that there is no connected peer by using /config/get_online_peers request.
# - sleep 91 second and check that telnet is closed.
# - Check that there is no connected peers by using /config/get_online_peers request


source ../../test_utils/functions.sh

# wait command exit code when metax was successfully finished (i.e. without Seg. fault or Assertion)
EXIT_CODE_OK=0



main()
{
        $EXECUTE -f $tst_src_dir/config.xml &
        p1=$!
        wait_for_init 7081
        echo -n $p1 > $test_dir/processes_ids

        telnet localhost 7080

        res=$(curl http://localhost:7081/config/get_online_peers)
        echo "res $res"
        expected="[]"
        if [ "$res" != "$expected" ]; then
                echo "Reconnected peer"
                kill_metax $p1
                exit;
        fi

        sleep 91
        pgrep telnet
        if [ "1" != "$?" ];then
                echo "telnet has not been turned off"
                kill_metax $p1
                exit;
        fi

        res=$(curl http://localhost:7081/config/get_online_peers)
        echo "res $res"
        expected="[]"
        if [ "$res" != "$expected" ]; then
                echo "Reconnected peer.There are a peers connected to metax"
                kill_metax $p1
                exit;
        fi

        echo $PASS > $final_test_result
        kill_metax $p1
}

main $@
