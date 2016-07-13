#!/bin/bash
#
# Regression testing for Duet: must run as root!
#

usage() {
	echo "Usage: $0 [OPTION]...

	-b	Test Duet startup process
	-e	Test event-based Duet
	-s	Test state-based Duet
	-d	Test Duet with a disk workload
	-r	Test Duet on a RAM disk
	-a	Run all tests
"
}

die() {
	echo "Aborting..." >&2
	exit 1
}

chk_return() {
	if [ $1 -eq 1 ]; then
		echo " vvvvvvvvvvvv ERROR vvvvvvvvvvvv "
		echo -e "Testing error: $2"
		echo " ^^^^^^^^^^^^ ERROR ^^^^^^^^^^^^ "
		die
	fi
}

test_boot() {
	# Description: Inserts Duet module, starts framework, stops it,
	# and finally unloads the Duet module.

	echo "## Test: Duet startup process"
	modprobe duet
	chk_return $? "module insertion failed"
	duet status start
	chk_return $? "Duet startup failed"
	duet status stop
	chk_return $? "Duet teardown failed"
	modprobe -r duet
	chk_return $? "module removal failed"
	echo -e "## Test 1: Success\n"
}

test_evt() {
	# Description: Uses dummy task to register for Duet page events.
	# Fetches events for 30 sec every 250 msec.

	echo "## Test 2: Event-based Duet"
	modprobe duet || die
	duet status start || die
	cd "${BASEDIR}/../duet-tools/dummy_task"
	./dummy -oeg -f 250 -d 30 -p /
	chk_return $? "event-based dummy task failed"
	duet status stop || die
	modprobe -r duet || die
	echo -e "## Test 2: Success\n"
}

test_state() {
	# Description: Uses dummy task to register for Duet page state changes.
	# Fetches events for 30 sec every 250 msec.

	echo "## Test 3: State-based Duet"
	modprobe duet || die
	duet status start || die
	cd "${BASEDIR}/../duet-tools/dummy_task"
	./dummy -og -f 250 -d 30 -p /
	chk_return $? "state-based dummy task failed"
	duet status stop || die
	modprobe -r duet || die
	echo -e "## Test 3: Success\n"
}

test_disk() {
	# Description: Runs filebench, cpugrabber, and dummy task simultaneously
	# to measure Duet overhead.

	echo "## Test 4: Disk workload"
	modprobe duet || die
	duet status start || die
	source ./eval_disk.sh /dev/vdb1
	chk_return $? "disk workload test failed"
	duet status stop || die
	modprobe -r duet || die
	echo -e "## Test 4: Success\n"
}

test_ram() {
	# TODO: Run the filebench workload on a RAMdisk
	die
}

echo "########################################"
echo "## Regression testing for Duet module ##"
echo -e "########################################\n"

# TODO: Check that we are running as root!

while getopts ":besdra" opt; do
	case $opt in
	b)
		test_boot
		exit 0
		;;
	e)
		test_evt
		exit 0
		;;
	s)
		test_state
		exit 0
		;;
	d)
		test_disk
		exit 0
		;;
	r)
		test_ram
		exit 0
		;;
	a)
		test_boot
		test_evt
		test_state
		test_disk
		test_ram
		exit 0
		;;
	\?)
		echo "Invalid option: -$OPTARG" >&2
        usage && die
		;;
	esac
done

usage && die

