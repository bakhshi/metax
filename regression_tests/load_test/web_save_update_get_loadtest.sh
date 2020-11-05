#!/bin/bash

# Launches 2*N save requests in parallel and waits for all of them to finish
# Computes min/max/average time of save requests
# Launches N update request and N get requests in parallel
# Wiats for all of them to finish
# All the described flow is done for different sizes

host=109.75.33.180
port=7073

sout=sout
uuids=uuids
uuuids=uuuids
guuids=guuids
uout=uout
gout=gout
tmp=tmp
result_dir=result_dir
source_dir=source

# Assumes that the file "tmp" contains time inforation in each line, were the
# the lins are the "real" part of the time command output
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

function launch_parallel_save
{
        c=$1
        input=$source_dir/$2
        echo "Running $c parallel save requests of $2 files"
        i=1;
        while [ $i -le $c ]
        do
                (time curl --form "fileupload=@$input" "http://$host:$port/db/save/data?enc=0" 2> /dev/null) >> ${sout}_$2 2<&1 &
                pids[$w]=$!
                w=$((w+1))
                i=$((i+1))
        done
}

function launch_parallel_get
{
        test=$1
        n=$(wc -l $result_dir/$test/$guuids | awk '{print $1}')
        echo "Launching $n get requests of $test files in parallel"
        i=1
        while read l
        do
            (time curl "http://$host:$port/db/get?id=$l" 2> /dev/null 1>$result_dir/$test/$l) >> $gout 2<&1 &
            pids[${w}]=$!
            w=$((w+1))
            i=$((i+1))
        done <$result_dir/$test/$guuids
}

function launch_parallel_update
{
        test=$1
        n=$(wc -l $result_dir/$test/$uuuids | awk '{print $1}')
        echo "Launching $n update requests of $test files in parallel"
        input=$source_dir/$test
        i=1
        while read l
        do
            (time curl --form "fileupload=@$input" "http://$host:$port/db/save/data?enc=0?id=$l" 2> /dev/null)  >> $uout 2<&1 &
            pids[${w}]=$!
            w=$((w+1))
            i=$((i+1))
        done <$result_dir/$test/$uuuids
}

function wait_for_save_finish()
{
        FAIL=0
        echo "Waiting for all save requests to finish"
        for pid in ${pids[*]}
        do
                wait $pid || FAIL=1
                if [ $FAIL != 0 ]; then
                        echo "Failed save file in the host:" $host
                        #exit 1;
                fi
        done
}

function wait_for_get_finish()
{
        FAIL=0
        echo "Waiting for all get requests to finish"
        for pid in ${pids[*]}
        do
                wait $pid || FAIL=1
                if [ $FAIL != 0 ]; then
                        echo "Failed get file in the host:" $host
                        #exit 1;
                fi
        done
}

function check_file_correctness()
{
        test=$1
        echo "Checking correctness of received files of size $test"
        while read f
        do
                d=$(diff $result_dir/$test/$f $source_dir/$test 2>&1)
                if [[ ! -z $d ]]; then
                        echo "File mismatch on $f get request"
                        #exit 1;
                fi
        done <$result_dir/$test/$guuids
}

# prepare input file for save/update/get
mkdir -p $source_dir
dd if=/dev/urandom of=$source_dir/3K count=3 bs=1024
dd if=/dev/urandom of=$source_dir/3M count=3000 bs=1024
dd if=/dev/urandom of=$source_dir/7M count=7000 bs=1024
dd if=/dev/urandom of=$source_dir/40M count=40000 bs=1024
dd if=/dev/urandom of=$source_dir/120M count=120000 bs=1024
#dd if=/dev/urandom of=$source_dir/700M count=700000 bs=1024

list="3K 3M 7M 40M 120M" 
#list="3K 3M 7M 40M 120M 700M"

n_3K=50
n_3M=25
n_7M=15
n_40M=6
n_120M=3
#n_700M=1


# cleanup generated files
rm -f sout_*
rm -fr $result_dir
rm -f $uout
rm -f $gout

# Running 2*n save requests in parallel and waiting for all to finish
for p in $list
do
        w=1
        launch_parallel_save $((n_$p*2)) $p
done

# wait for all pids
wait_for_save_finish

for p in $list
do
        if [[ $(grep -i "error" ${sout}_$p) ]]; then
                echo "Failed save file in the host:" $host
                exit 1;
        fi
done

# compute min, max and average times of save requests
for p in $list
do
        grep real ${sout}_${p} > $tmp
        echo "timing for save requests (seconds) of $p files"
        compute_timing
        echo "-------------------"
done

# run n parallel get
for p in $list
do
        mkdir -p $result_dir/$p
        cat ${sout}_$p | grep uuid | jq .uuid | cut -d'"' -f2 > $result_dir/$p/$uuids
        cp $result_dir/$p/$uuids $result_dir/$p/$guuids
        w=1;
        launch_parallel_get $p
done

wait_for_get_finish $p

for p in $list
do
        check_file_correctness $p
done

# prepare part n saved file for updating and n saved files for getting
#cat ${sout}_3K | grep uuid | jq .uuid | cut -d'"' -f2 > $uuids
echo
echo "Preparing uuids for save and get requests"
for p in $list
do
        cat $result_dir/$p/$uuids | head -n $((n_$p)) > $result_dir/$p/$uuuids
        cat $result_dir/$p/$uuids | tail -n $((n_$p)) > $result_dir/$p/$guuids
done

rm -f sout_*

# run n parallel save, update and get requests
w=1
for p in $list
do
        echo "-------------------"
        launch_parallel_save $((n_$p)) $p
        launch_parallel_update $p
        launch_parallel_get $p
done

wait_for_save_finish

if [[ $(grep -i "error" $uout) ]]; then
    echo "Failed update file in the host:" $host
	exit 1;
fi

# compute min, max and average times of update requests
cat ${sout}_* > sout_all
grep real sout_all > $tmp
echo "timing for save requests (seconds)"
compute_timing
echo "-------------------"

# compute min, max and average times of update requests
grep real $uout > $tmp
echo "timing for update requests (seconds)"
compute_timing
echo "-------------------"

# compute min, max and average times of get requests
grep real $gout > $tmp
echo "timing for get requests (seconds)"
compute_timing
echo "-------------------"

