#!/bin/bash

rm -rf storage_data 
rm -rf storage_data_new
rm -rf old_storage
rm -rf get_old_uuids
rm -f master_uuid
rm -f master_uuids
rm -f leviathan1.log

main()
{
        # git checkout to the old Metax to get all uuids
        git checkout 096b1642948863f48206f7a95678c0da946d2ccc
        make
        ./bin/metax_web_api -f config1.xml &> leviathan1.log &
        p=$!
        sleep 1
        
        # Starting save encrypted file in the old Metax
        for (( i = 1; i <= 20; ++i))
        do
                curl --form "fileupload=@save_file" http://localhost:8003/db/save/data?enc=1
        done

        # Copying the old storage
        cp -r storage old_storage
        for u in storage/????????-*; do
                grep ".master_uuid" $u
                if [ $? -eq 0 ]; then
                        echo $u >> master_uuid
                fi
        done
        cat master_uuid | cut -d '/' -f2 > master_uuids

        # Creating storage_data directory
        mkdir storage_data
        
        # Sending get request for every master_uuid
        # and saving them as files
        GET_REQUEST=$(cat master_uuids)
        echo "starting get"
        for uuid in $GET_REQUEST
        do
                curl http://localhost:8003/db/get?id=$uuid > ./storage_data/$uuid
        done
        kill $p
        wait $p

        sleep 5
        # git pull for starting update old storage's data in the new Metax.
        echo "git pull"
        git pull origin master
        make
        sleep 1
        
        echo "Running new Metax"
        ./bin/metax_web_api -f config1.xml &> leviathan1.log &
        p=$!
        sleep 1

        echo "Starting getting with a new Metax"
        mkdir get_old_uuids 

        for uuid in $GET_REQUEST
        do
                curl http://localhost:8003/db/get?id=$uuid > ./get_old_uuids/$uuid
        done
        
        # Sending "file" update request for every file saved in storage_data
        # from the old storage
        echo "As getting failed, so starting updating the old storage"
        cd storage_data
        for u in ????????-*
        do
             curl --form "fileupload=@$u" "http://localhost:8003/db/save/data?enc=0&id=$u"
        done
        
        cd ..
        echo "starting get"
        mkdir storage_data_new
        GET_REQUEST=$(cat master_uuids)
        for uuid in $GET_REQUEST
        do
                curl http://localhost:8003/db/get?id=$uuid > ./storage_data_new/$uuid
        done
        kill $p
        wait $p
}

main $@

# vim:et:tabstop=8:shiftwidth=8:cindent:fo=croq:textwidth=80:
