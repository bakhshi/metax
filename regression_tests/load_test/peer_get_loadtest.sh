#!/bin/bash


result_dir=results

if [[ $# -ne 2 ]]
then
	echo "illegal number of parameters"
	echo "usage: $0 <source data folder> <number of metax instances>"
	exit 1
fi

if [ -d "$1" ]
then
	source_dir=$1
else
	echo "error: no such directory: $1"
	exit 1
fi

if [[ $2 =~ ^[0-9]+$ ]]
then
	N=$2
else
	echo "error: $1 is not and integer positive value"
	exit 1
fi

host=localhost
result_dir=result
bin_path=$PWD/../../bin/metax_web_api
mport=9000
wport=9100
mpids=mpids
sout=sout

#host=109.75.33.37
#metax_port=7071

function prepare_metaxes()
{
        echo preparing $N metax directories
        for ((i=1;  i <= $N; ++i))
        do
                mp=$((mport+i))
                wp=$((wport+i))
                metax_dir=$result_dir/metax$i
                mkdir -p $metax_dir
                sed -e "s/%MPORT%/$mp/" \
                        -e "s/%WPORT%/$wp/" \
                        -e "s/%PEER%/109.75.33.37:7080/" \
                        templates/config.xml > $metax_dir/config.xml
                #cp templates/peers_ratings.json $metax_dir/
                mkdir -p $metax_dir/bin
                ln -sf $bin_path $metax_dir/bin/test_metax
                # in case directories exist anyway remove log
                rm -f $metax_dir/leviathan.log*
        done
}

function launch_metaxes()
{
        for ((i=1;  i <= $N; ++i))
        do
                metax_dir=$result_dir/metax$i
                cd $metax_dir && exec ./bin/test_metax -f config.xml & echo -n " $!" >> $mpids
        done
}

function launch_parallel_get()
{
        test=$1
        n=$(wc -l $source_dir/uuids_$test | awk '{print $1}')
        echo "Launching $n get requests of $test files in parallel"
	for ((i=1;  i <= $N; ++i))
	do
		metax_dir=$result_dir/metax$i
		rm -fr $metax_dir/$test
		mkdir -p $metax_dir/$test
		wp=$((wport+i))
		while read l
		do
			#curl "http://$host:$metax_port/db/get?id=$l" 2> /dev/null 1>$result_dir/$test/$l &
			(time curl "http://$host:$wp/db/get?id=$l&cache=0" 2> /dev/null 1>$metax_dir/$test/$l) >> ${sout}_$test 2<&1 &
			pids[$w]=$!
			w=$((w+1))
		done <$source_dir/uuids_$test
	done
}

function wait_for_finish()
{
        FAIL=0
        echo "Waiting for all get requests to finish"
        for pid in ${pids[*]}; do
                wait $pid || FAIL=1
                if [ $FAIL != 0 ]; then
                        echo "Failed get from host:" $host
                        echo "Get request exited with non 0 status"
                        #exit 1;
                fi
        done
}

function check_file_correctness()
{
        test=$1
        echo "Checking correctness of received files of size $test"

	for ((i=1;  i <= $N; ++i))
	do
		metax_dir=$result_dir/metax$i
		for f in $metax_dir/$test/*
		do
			d=$(diff $f $source_dir/$test 2>&1)
			if [[ ! -z $d ]]; then
				echo "File mismatch on $f get request"  
				kill `cat $mpids`
				exit 1;
			fi
		done
	done



}

mkdir -p $result_dir
rm -fr $result_dir/{3KB,3MB,40MB,700MB}
prepare_metaxes $N
rm -f $mpids
launch_metaxes $N
sleep 7

rm -f sout_*

w=1
# launching parellel get requests of specified size of files
launch_parallel_get 3KB
#launch_parallel_get 3MB
launch_parallel_get 40MB
launch_parallel_get 700MB

# waiting for all get requests to finish
wait_for_finish

# checking correctness of received files
check_file_correctness 3KB
#check_file_correctness 3MB
check_file_correctness 40MB
check_file_correctness 700MB
echo success

kill `cat $mpids`
