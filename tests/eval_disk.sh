#!/bin/bash -x
#
# Runs filebench alongside Duet microbenchmarks, and dumps raw numbers in an R
# file. No results are kept for filebench.
# Supported workloads: "varmail", "webserver", "webproxy", "fileserver"
#
# @1: partition to run filebench on
#

# Set up environment variables
cd "$(dirname "$0")"
basedir="$(pwd)"
outdir="/media/duet"
fspath="/media/fbench"

# Program paths
dummyp="$basedir/../dummy_task/dummy"
cgrabp="$basedir/cpugrabber/cpugrabber"
pgrabp="$basedir/cpugrabber/process-grabber"

workload="webserver"
fetchfreq=(0 10 20 40)

# Set up argument variables
fspart="$1"
fssize="`sudo fdisk -l $fspart | head -1 | awk '{print $5}'`"
resdir="$outdir/results/"
cgout="$outdir/cgrabber.out"
fbfile="$outdir/fbperson.f"

# build_wkld.sh variables, no need to touch
explen=8000
dummylen=4 #XXX:300
cpugran=1000 #XXX: 5000
cpulen=4000 #XXX: 300000
profgran=2 #XXX: 20
warmup_periods=6 # periods of $profgran seconds each
source build_wkld.sh

# Runs one configuration of the microbenchmark
# @1: configuration -- duet off (0), duet on (1), duet register (2)
# @2: fetch frequency in ms (if @1 == 2)
# @3: duet item type -- events (0), states (1)
# @4: experiment suffix for output
run_one () {
	config=$1
	ffreq=$2
	evtbased=$3
	expsfx=$4

	duet status stop
	if [ $config -ge 1 ]; then
		echo -ne "- Starting the duet framework... " | tee -a $logpath
		duet status start
		echo -ne "Done.\n" | tee -a $logpath
	fi

	dargs="-d $dummylen"
	if [ $evtbased -eq 1 ]; then
		dargs="$dargs -e"
	fi
	if [ $config -eq 2 ]; then
		dargs="$dargs -o"
		if [ $ffreq -gt 0 ]; then
			dargs="$dargs -f $ffreq"
		fi
	fi

	for expiter in $(seq 1 $numreps); do
		# Start the cpugrabber before we start the microbenchmark
		sudo $cpugrabberp -r $cpugran -t $cpulen 2> $cgout &
		cgid=$!

		# Start dummy task, and wait until it's done
		sudo $dummyp $dargs
		wait $cgid

		# Log cpugrabber results
		if [ $expiter -eq 1 ]; then
			cgresults="`sudo $psgrabberp dummy $cgout`"
		else
			cgresults="$cgresults, `sudo $psgrabberp dummy $cgout`"
		fi
	done

	# Append results in R file
	echo -ne "Exporting collected stats... " | tee -a $logpath
	echo -e "${expsfx} <- c($cgresults);\n" >> $rfpath
	echo -ne "Done.\n" | tee -a $logpath
}

# Starts filebench and runs each iteration of the microbenchmark
run_experiments () {
	echo -ne "- Removing all files from '$fspath'..." | tee -a $logpath
	if [[ $fspath == /media/* ]]; then
		rm -Rf $fspath/*
	else
		echo "\nError: $fspath not under /media." | tee -a $logpath
		exit 1
	fi
	echo " Done." | tee -a $logpath

	echo -ne "- Compiling filebench profile..." | tee -a $logpath
	compileprof $wkld
	echo " Done." | tee -a $logpath

	echo -ne "- Clearing buffer cache... " | tee -a $logpath
	sudo sh -c "sync && echo 3 > /proc/sys/vm/drop_caches"
	echo -ne "Done.\n" | tee -a $logpath

	# Start filebench and count times we've seen "Running..." sequence
	running=0
	echo -e "- Starting filebench... " | tee -a $logpath
	sudo filebench -f $fbfile 2>&1 | tee -a /dev/tty | \
	grep --line-buffered "IO Summary" | \
	while read; do
		if [ $running -lt $warmup_periods ]; then
			running=$((running+1))
		elif [ $running -eq $warmup_periods ]; then
			running=$((running+1))

			echo -e "- Running microbenchmarks..." | tee -a $logpath
			echo -e "  >> Duet off" | tee -a $logpath
			echo -e "# Results when Duet is off" | tee -a $rfpath
			run_one 0 0 0 "x0"

			echo -e "  >> Duet state-based" | tee -a $logpath
			echo -e "# Results when state-based Duet is on" | tee -a $rfpath
			run_one 1 0 0 "s0"

			for ffreq in ${fetchfreq[@]}; do
				echo -e "  >> Duet state-based, fetch every ${ffreq}ms" | tee -a $logpath
				echo -e "# Results when state-based Duet is fetching every ${ffreq}ms" | tee -a $rfpath
				run_one 2 $ffreq 0 "s$ffreq"
			done

			echo -e "  >> Duet event-based" | tee -a $logpath
			echo -e "# Results when event-based Duet is on" | tee -a $rfpath
			run_one 1 0 1 "e0"

			for ffreq in ${fetchfreq[@]}; do
				echo -e "  >> Duet event-based, fetch every ${ffreq}ms" | tee -a $logpath
				echo -e "# Results when event-based Duet is fetching every ${ffreq}ms" | tee -a $rfpath
				run_one 2 $ffreq 1 "e$ffreq"
			done
		elif [ $running -gt $warmup_periods ]; then
			break
		fi
	done

	# If we got here, we're done. Filebench must die.
	fbpid="`ps aux | grep "sudo filebench" | grep -v grep \
			| awk '{print $2}'`"
	if [[ ! -z $fbpid  ]]; then
		kill -INT $fbpid
	fi

	# Keep syslog output in $output.log
	echo "- Appending syslog to $outpfx.syslog" | tee -a $logpath
	cp /var/log/syslog $outpfx.syslog
}

# Initialize some environment variables
case $workload in
"webserver")	wkld="wsv"	;;
"varmail")		wkld="var"	;;
"webproxy")		wkld="wpx"	;;
"fileserver")	wkld="fsv"	;;
*)
	echo "I don't recognize workload '$workload'. Goodbye."
	exit 1
	;;
esac

datstr="$(date +%y%m%d-%H%M)"		# Date string
outpfx="$resdir${wkld}_${datstr}"	# Output file prefix

logpath="${outpfx}.log"				# Log file path
rfpath="${outpfx}.R"				# R file path
mkdir -p $resdir
echo "" > $logpath
echo "" > $rfpath

# Create filesystem and mount it
sudo umount $fspart
sudo mkdir -p $fspath
sudo logrotate -f /etc/logrotate.conf
echo "- Creating ext4 filesystem on $fspart..." | tee -a $logpath
sudo mkfs.ext4 -F $fspart
echo "- Mounting ext4 filesystem on $fspath..." | tee -a $logpath
sudo mount $fspart $fspath

# Do what filebench wants
sudo sh -c "echo 0 > /proc/sys/kernel/randomize_va_space"

echo -e "\n=== Starting experiments ===" | tee -a $logpath
run_experiments
echo -e "\n=== Evaluation complete (results in ${resdir}) ===" | tee -a $logpath

# Cleanup temporary files and unmount fs
rm $fbfile
echo "- Unmounting ext4 filesystem on $fspath..." | tee -a $logpath
sudo umount $fspath
