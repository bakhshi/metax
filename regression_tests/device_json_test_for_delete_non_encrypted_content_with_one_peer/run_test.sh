#!/bin/bash

# Test case flow:
# - Runs two instances of metax and connect with linear connection (metax1 <- metax2).
# - Sends "node" save request for some content for non encrypted content.
# - Checks the validation of device.json after "save" request.
# - Sends "delete" request for saved data.
# - Checks the validation of device.json after "delete" request.

source ../../test_utils/functions.sh

# wait command exit code when metax was successfully finished (i.e. without Seg. fault or Assertion)
EXIT_CODE_OK=0
save_content="Save some content for test"
re='^[0-9]+$'




main()
{
        $EXECUTE -f $tst_src_dir/config1.xml &
        p1=$!
        is_peer_connected=$(peers_connect 7081 $tst_src_dir/config1.xml)
        if [ "0" != "$is_peer_connected" ];then
                echo "7080 does not connect to peer"
                kill_metax $p1
                exit;
        fi

        $EXECUTE -f $tst_src_dir/config2.xml &
        p2=$!
        is_peer_connected=$(peers_connect 7091 $tst_src_dir/config2.xml)
        if [ "0" != "$is_peer_connected" ];then
                echo "7090 does not connect to peer"
                kill_metax $p2 $p1
                exit;
        fi
        echo -n $p1 $p2 > $test_dir/processes_ids

        uj_uuid1=$(decrypt_user_json_info $test_dir/keys1 | jq '.uuid' | sed -e 's/^"//' -e 's/"$//')
        uj_uuid2=$(decrypt_user_json_info $test_dir/keys2 | jq '.uuid' | sed -e 's/^"//' -e 's/"$//')

        uuid=$(curl --data "$save_content" http://localhost:7081/db/save/node?enc=0)
        uuid=$(echo $uuid | cut -d'"' -f 4)
        sleep $TIME
        kill_metax $p1 $p2

        uuids_peer1=$(ls $test_dir/storage1/????????-* | grep -v "$uj_uuid1\|$uj_uuid1.tmp" | grep -v "$uj_uuid2\|$uj_uuid2.tmp" | rev | cut -d'/' -f 1 | rev)
        uuids_peer2=$(ls $test_dir/storage2/????????-* | grep -v "$uj_uuid1\|$uj_uuid1.tmp" | grep -v "$uj_uuid2\|$uj_uuid2.tmp" | rev | cut -d'/' -f 1 | rev)

        ../../utils/encryption_decryption/decrypt_user_json.sh $test_dir/keys1/user/private.pem $test_dir/keys1/user_json_info.json $test_dir/device1.json $test_dir/decrypted_device1.json
        storage1=$(cat $test_dir/decrypted_device1.json | jq -r '.storage')

        ../../utils/encryption_decryption/decrypt_user_json.sh $test_dir/keys2/user/private.pem $test_dir/keys2/user_json_info.json $test_dir/device2.json $test_dir/decrypted_device2.json
        storage2=$(cat $test_dir/decrypted_device2.json | jq -r '.storage')

        for i in $uuids_peer1
        do
                if [[ "${uuids_peer2[@]}" =~ "$i" ]]; then
                        piece1=$(echo $storage1 | jq -r ".\"$i\"");
                        if [[ $piece1 == null ]]; then
                                echo "$i uuid missing in device1.json"
                                exit;
                        fi
                        piece2=$(echo $storage2 | jq -r ".\"$i\"");
                        if [[ $piece2 == null ]]; then
                                echo "$i uuid missing in device2.json"
                                exit;
                        fi

                        is_deleted1=$(echo $piece1 | jq -r '.is_deleted')
                        last_updated1=$(echo $piece1 | jq -r '.last_updated')
                        version1=$(echo $piece1 | jq -r '.version')

                        is_deleted2=$(echo $piece2 | jq -r '.is_deleted')
                        last_updated2=$(echo $piece2 | jq -r '.last_updated')
                        version2=$(echo $piece2 | jq -r '.version')

                        if [[ ! $last_updated1 =~ $re ]] || [ "$last_updated1" -ne "$last_updated2" ]; then
                                echo "Invalid last_updated field for piece (last_updated1=$last_updated1, last_updated2=$last_updated2)"
                                exit;
                        fi
                        if [ "$is_deleted1" != "false" ] || [ "$is_deleted1" != "$is_deleted2" ]; then
                                echo "Invalid is_deleted field for piece (is_deleted1=$is_deleted1, is_deleted2=$is_deleted2)"
                                exit;
                        fi
                        if (($version1 < 1)) || [ "$version1" -ne "$version2" ]; then
                                echo "Invalid version field for piece (version1=$version1, version2=$version2)"
                                exit;
                        fi
                else
                        echo "$i uuid from storage1 don't saved at storage2"
                fi
        done

        $EXECUTE -f $tst_src_dir/config1.xml &
        p1=$!
        is_peer_connected=$(peers_connect 7081 $tst_src_dir/config1.xml)

        $EXECUTE -f $tst_src_dir/config2.xml &
        p2=$!
        is_peer_connected=$(peers_connect 7091 $tst_src_dir/config2.xml)
        if [ "0" != "$is_peer_connected" ];then
                echo "7090 does not connect to peer"
                kill_metax $p2 $p1
                exit;
        fi
        echo -n $p1 $p2 > $test_dir/processes_ids

        expected='{"deleted":"'$uuid'"}'
        res=$(curl http://localhost:7081/db/delete?id=$uuid)
        sleep $TIME
        kill_metax $p1 $p2

        if [ "$expected" != "$res" ]; then
                echo "Failed to delete. Response is $res"
                exit;
        fi

        # keep properties for master object in device1.json and device2.json after deleted request.
        last_updated1=$(echo $storage1 | jq -r ".\"$uuid\"" | jq -r '.last_updated')

        ../../utils/encryption_decryption/decrypt_user_json.sh $test_dir/keys1/user/private.pem $test_dir/keys1/user_json_info.json $test_dir/device1.json $test_dir/decrypted_device1.json
        storage1=$(cat $test_dir/decrypted_device1.json | jq -r '.storage')

        ../../utils/encryption_decryption/decrypt_user_json.sh $test_dir/keys2/user/private.pem $test_dir/keys2/user_json_info.json $test_dir/device2.json $test_dir/decrypted_device2.json
        storage2=$(cat $test_dir/decrypted_device2.json | jq -r '.storage')

        for i in $uuids_peer1
        do
                is_deleted1_2=$(echo $storage1 | jq -r ".\"$i\"" | jq -r '.is_deleted')
                last_updated1_2=$(echo $storage1 | jq -r ".\"$i\"" | jq -r '.last_updated')
                version1_2=$(echo $storage1 | jq -r ".\"$i\"" | jq -r '.version')

                is_deleted2_2=$(echo $storage2 | jq -r ".\"$i\"" | jq -r '.is_deleted')
                last_updated2_2=$(echo $storage2 | jq -r ".\"$i\"" | jq -r '.last_updated')
                version2_2=$(echo $storage2 | jq -r ".\"$i\"" | jq -r '.version')

                if [[ ! $last_updated1_2 =~ $re ]] || [ "$last_updated1_2" -lt "$last_updated1" ] || [ "$last_updated1_2" != "$last_updated2_2" ] ; then
                        echo "Invalid last_updated1_2=$last_updated1_2 last_updated2_2=$last_updated2_2 field for old pieces"
                        exit;
                fi
                if [ "$is_deleted1_2" != "true" ] || [ "$is_deleted1_2" != "$is_deleted2_2" ] ; then
                        echo "Invalid is_deleted1_2=$is_deleted1_2 is_deleted2_2=$is_deleted2_2 field for old pieces"
                        exit;
                fi
                if (($version1_2 != 2)) || [ $version1_2 != $version2_2 ]; then
                        echo "Invalid version1_2=$version1_2 version2_2=$version2_2 field for old pieces"
                        exit;
                fi

        done
        echo $PASS > $final_test_result
}

main $@


