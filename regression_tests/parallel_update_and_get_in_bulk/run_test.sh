#!/bin/bash

# Launches 2*N save requests in parallel and waits for all of them to finish
# Computes min/max/average time of save requests
# Launches N update request and N get requests in parallel
# Waits for all of them to finish
# Computes min/max/average time of update and get requests
# TODO: The test also can be run with restricted network bandwidth. Now the
#       restriction is commented out. To turn on the restrictions please
#       uncomment the commands which contain 'wondershaper' command


host=172.17.7.67
port=8003
n=3
user=metax
pass=metax_leviathan

sout=$test_dir/sout
uuids=$test_dir/uuids
uuuids=$test_dir/uuuids
guuids=$test_dir/guuids
uout=$test_dir/uout
gout=$test_dir/gout
get_file=$test_dir/get_file
input=$test_dir/file.txt
tmp=$test_dir/tmp

# Assumes that the file "tmp" contains time information in each line, where
# the lines are the "real" part of the time command output
# Reads the time information, converts to seconds and computes min, max and
# average of all the lines
function compute_timing()
{
        min=10000
        max=0
        sum=0
        i=0
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
                i=$((i+1))
        done <$tmp
        avg=$(bc <<< "scale=3; $sum/$i")
        echo average time: $avg
        echo minimum time: $min
        echo maximum time: $max
}

# prepare input file for save/update/get
dd if=/dev/zero of=$input bs=100000 count=1024

touch $test_dir/processes_ids

# cleanup generated files
rm -f $sout
rm -f $uout
rm -f $gout

# limit network bandwidth
#sshpass -p$pass ssh -oStrictHostKeyChecking=no  -tt $user@$host "echo $pass | sudo -S -s /bin/bash -c 'wondershaper  eth0 8000 8000'"

# Running 2*n save requests in parallel and waiting for all to finish
i=1;
c=$(($n*2))
echo "Running $c parallel save requests"
while [ $i -le $c ]
do
	(time curl --form "fileupload=@$input" "http://$host:$port/db/save/data?enc=0" 2> /dev/null) >> $sout 2<&1 &
	pids[$i]=$!
        i=$((i+1))
done

# wait for all pids
f=0
for pid in ${pids[*]}
do
    wait $pid || f=1
    if [ $f != 0 ]; then
	    echo $FAIL > $final_test_result
	    echo "Failed save file in the host:" $host
	    exit;
    fi
done
if [[ $(grep -i "error" $sout) ]]; then
        echo $FAIL > $final_test_result
        echo "Failed save file in the host:" $host
        exit;
fi

# compute min, max and average times of save requests
echo "-------------------"
grep real $sout > $tmp
echo "timing for save requests (seconds)"
compute_timing

# prepare part n saved file for updating and n saved files for getting
cat $sout | grep uuid | jq .uuid | cut -d'"' -f2 > $uuids 
cat $uuids | head -n $n > $uuuids
cat $uuids | tail -n $n > $guuids

echo "-------------------"
echo "Running $n update and $n get requests in parallel"
# run n parallel update
i=1
while read l
do
    (time curl --form "fileupload=@$input" "http://$host:$port/db/save/data?enc=0&id=$l" 2> /dev/null)  >> $uout 2<&1 &
    upids[$i]=$!
    i=$((i+1))
done <$uuuids

# run n parallel get
i=1
while read l
do
    (time curl "http://$host:$port/db/get?id=$l" 2> /dev/null 1> $get_file$i)  >> $gout 2<&1 &
    gpids[$i]=$!
    i=$((i+1))
done <$guuids

# wait for update requests to finish
for pid in ${upids[*]}; do
    wait $pid || f=1
    if [ $f != 0 ]; then
	    echo $FAIL > $final_test_result
	    echo "Failed update file in the host:" $host
	    exit;
    fi
done
if [[ $(grep -i "error" $uout) ]]; then
	echo $FAIL > $final_test_result
	echo "Failed update file in the host:" $host
	exit;
fi

# compute min, max and average times of update requests
echo "-------------------"
grep real $uout > $tmp
echo "timing for update requests (seconds)"
compute_timing
echo "-------------------"

# wait for get requests to finish
i=1
for pid in ${gpids[*]}; do
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
    rm  $get_file$i
    i=$((i+1))
done
if [[ $(grep -i "error" $gout) ]]; then
	echo $FAIL > $final_test_result
	echo "Getting file failed from the host:" $host
	exit;
fi

# compute min, max and average times of get requests
grep real $gout > $tmp
echo "timing for get requests (seconds)"
compute_timing

# delete previously saved all uuids.
while read l
do
        expected='{"deleted":"'$l'"}'
        res=$(curl http://$host:$port/db/delete?id=$l)
        echo "Delete request response is $res"
        if [ "$expected" != "$res" ]; then
                echo $FAIL > $final_test_result
        fi

done <$test_dir/uuids
# remove network bandwidth limitation
#sshpass -p$pass ssh -oStrictHostKeyChecking=no  -tt $user@$host "echo $pass | sudo -S -s /bin/bash -c 'wondershaper clear eth0'"

echo $PASS > $final_test_result
