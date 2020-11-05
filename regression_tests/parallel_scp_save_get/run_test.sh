#!/bin/bash

# Launches N scp commands to the HOST
# Computes min/max/average time of scp
# Launches N save request and wiats for all of them to finish
# Computes min/max/average time of save requests
# Launches N get request of the save files and wiats for all of them to finish
# Computes min/max/average time of get requests
# TODO: The test also can be run with restricted network bandwidth. Now the
#       restriction is commented out. To turn on the restrictions please
#       uncomment the commands which contain 'wondershaper' command

# Simultaneously runs N scp commands (on file.txt) with time utility.
# The output of the time utility of all scp instances is written into scp_time
# file
function launch_parallel_scp
{
        i=1
        while [ $i -le $n ]
        do
                (time sshpass -p $password scp -o StrictHostKeyChecking=no $input $user@$host:~/ 2> /dev/null) >> $scp_time 2>&1 &
                pids[$i]=$!
                i=$((i+1))
        done
        f=0
        for pid in ${pids[*]}; do
                wait $pid || f=1
                if [ $f != 0 ]; then
                        echo $FAIL > $final_test_result
                        echo "Failed scp from the host:" $host
                        exit;
                fi
        done
}

# Simultaneously runs N Metax save request (uploading file.txt) with time
# utility.
# The output of the time utility of all save requests as well as saved UUIDs
# are written into save_res file
function launch_parallel_save
{
        i=1
        while [ $i -le $n ]
        do
                (time curl --form "fileupload=@$input" "http://$host:$metax_port/db/save/data?enc=0" 2> /dev/null) >> $save_res 2>&1 &
                pids[$i]=$!
                i=$((i+1))
        done
        f=0
        for pid in ${pids[*]}; do
                wait $pid || f=1
                if [ $f != 0 ]; then
                        echo $FAIL > $final_test_result
                        echo "Failed save file in the host:" $host
                        exit;
                fi
        done
        if [[ $(grep -i "error" $save_res) ]]; then
                echo $FAIL > $final_test_result
                echo "Failed save file in the host:" $host
                exit;
        fi
}

# Simultaneously runs N Metax get request with time utility. The UUIDs are
# taken from the output of the save_res file.
# The output of the time utility of all get requests is written into get_res
# file
function launch_parallel_get
{
        i=1
        while read l
        do
                (time curl "http://$host:$metax_port/db/get?id=$l" 2> /dev/null 1> $get_file$i) >> $get_res 2>&1 &
                pids[$i]=$!
                i=$((i+1))
        done <$test_dir/uuids
        f=0
        i=1
        for pid in ${pids[*]}; do
                wait $pid || f=1
                if [ $f != 0 ]; then
                        echo $FAIL > $final_test_result
                        echo "Getting file failed from the host:" $host
                        exit;
                fi
                d=$(diff $input $get_file$i 2>&1)
                if [[ ! -z $d ]]; then
                        echo $FAIL > $final_test_result
                        echo "File mismatch on $i get request"
                        exit;
                fi
		rm $get_file$i
                i=$((i+1))
        done
        if [[ $(grep -i "error" $get_res) ]]; then
                echo $FAIL > $final_test_result
                echo "Getting file failed from the host:" $host
                exit;
        fi
}

# Assumes that the file "tmp" contains time inforation in each line, were the
# the lins are the "real" part of the time command output
# Reads the time information, converts to seconds and computes min, max and
# average of all the lines
function compute_timing()
{
        min=10000
        max=0
        sum=0
        while read l
        do
                a=$(echo $l | awk '{print $2}')
                t1=$(echo $a | cut -d'm' -f1)
                t2=$(echo $a | cut -d'm' -f2 | cut -d's' -f1)
                t1=$(bc <<< "scale=3; 60*$t1")
                t=$(bc <<< "scale=3; $t1 + $t2")
                if (( $(bc <<<"$t < $min") )) ; then
                        min=$t
                fi
                if (( $(bc <<<"$t > $max") )) ; then
                        max=$t
                fi
                sum=$(bc <<< "scale=3; ($sum) + ($t)")
        done <$tmp
        avg=$(bc <<< "scale=3; $sum/$n")
        echo average time: $avg
        echo minimum time: $min
        echo maximum time: $max
}

user=pi
password=bananapi
host=172.17.7.65
metax_port=7081
n=3
scp_time=$test_dir/scp_time
get_res=$test_dir/get_res
save_res=$test_dir/save_res
get_file=$test_dir/get_file
tmp=$test_dir/tmp
input=$test_dir/file.txt

# Touch processes_ids file to avoid errors from killall_running_metax.sh script.
touch $test_dir/processes_ids
dd if=/dev/zero of=$input bs=100000 count=1024

# limit network bandwidth
#sshpass -p$pass ssh -oStrictHostKeyChecking=no  -tt $user@$host "echo $pass | sudo -S -s /bin/bash -c 'wondershaper  eth0 8000 8000'"

# cleanup generated files
rm -f $scp_time
rm -f $get_res
rm -f $save_res
rm -f $tmp

# Launch n instances of scp and compute min, max and average times
echo "-------------------"
echo "launching bulk scp"
launch_parallel_scp
grep real $scp_time > $tmp
echo "timing for scp (seconds)"
compute_timing

# Launch n instances of Metax save requests and compute min, max and average
# times
echo "-------------------"
echo "launching bulk save"
launch_parallel_save
grep real $save_res > $tmp
echo "timing for save (seconds)"
compute_timing

# Launch n instances of Metax get requests and compute min, max and average
# times
echo "-------------------"
echo "launching bulk get"
cat $save_res | grep uuid | jq .uuid | cut -d'"' -f2 > $test_dir/uuids
launch_parallel_get
grep real $get_res > $tmp
echo "timing for get (seconds)"
compute_timing

# remove network bandwidth limitation
#sshpass -p$pass ssh -oStrictHostKeyChecking=no  -tt $user@$host "echo $pass | sudo -S -s /bin/bash -c 'wondershaper clear eth0'"

# delete previously saved all uuids.
while read l
do
        expected='{"deleted":"'$l'"}'
        res=$(curl http://$host:$metax_port/db/delete?id=$l)
        echo "Delete request response is $res"
        if [ "$expected" != "$res" ]; then
                echo $FAIL > $final_test_result
        fi

done <$test_dir/uuids
echo $PASS > $final_test_result
