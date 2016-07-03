#!/bin/bash

# Find location of this setup.sh
STARTDIR="$(pwd)"
cd "$(dirname "$0")"
BASEDIR="$(pwd)"
cd "${STARTDIR}"

usage() {
	echo "Usage: $0 [OPTION]...

	-d	Install Ubuntu dependencies
	-c	Configure Duet kernel
	-k	Compile Duet kernel
	-g	Compile Duet kernel with debug symbols
	-m	Compile Duet module
	-t	Compile Duet library and tools
	-K	Install Duet kernel
	-M	Install Duet module
	-T	Install Duet tools
	-u	Uninstall all but latest Duet kernel, and clean up deb packages
"
}

die() {
	echo "Aborting..."
	exit 1
}

cd "${BASEDIR}/../duet-kernel"
KERNEL_VERSION_APPEND="+duet-$(git rev-parse --short HEAD)"
cd "${STARTDIR}"

while getopts ":dckgmtKMTu" opt; do
	case $opt in
	d)
		echo "Installing Ubuntu dependencies..."

		# Check that we're running on Ubuntu
		if [[ ! `grep "DISTRIB_ID=Ubuntu" /etc/lsb-release` ]]; then
			echo "I only know the dependencies for Ubuntu. Sorry." >&2
			exit 1
		fi

		# Install kernel dependencies
		sudo apt-get install build-essential kernel-package libssl-dev \
			libncurses5-dev

		echo "Done processing Ubuntu dependencies."
		exit 0
		;;
	c)
		cd "${BASEDIR}/../duet-kernel"
		rm .config
		touch .scmversion
		make localmodconfig || die

		cat .config | sed 's/# CONFIG_DUET is not set/CONFIG_DUET=y/g' > .config-new
		mv .config-new .config
		exit 0
		;;
	[kg])
		cd "${BASEDIR}/.."
		KDBG=`test $opt == 'g' && echo kernel_debug`

		# Prep the environment
		export CLEAN_SOURCE=no
		export CONCURRENCY_LEVEL="$(expr `nproc` + 1)"
		echo CLEAN_SOURCE=$CLEAN_SOURCE
		echo CONCURRENCY_LEVEL=$CONCURRENCY_LEVEL

		# (re)compile the kernel
		cd "${BASEDIR}/../duet-kernel"
		time fakeroot make-kpkg --initrd --append-to-version="${KERNEL_VERSION_APPEND}" \
			kernel_image kernel_headers $KDBG || die

		echo "Done compiling Duet kernel."
		exit 0
		;;
	m)
		cd "${BASEDIR}/../duet-module"

		# (re)compile the duet module
		moddir="`ls -l /lib/modules | grep ${KERNEL_VERSION_APPEND} | \
			cut -d' ' -f 9 | head -1`"
		make -C /lib/modules/${moddir}/build M=$(pwd) modules

		echo "Done compiling Duet module."
		exit 0
		;;
	t)
		cd "${BASEDIR}/../duet-tools"

		# (re)compile the duet tools
		cd duet-progs
		make || die
		locate libduet | grep /usr/local/lib > /dev/null
		if [[ $? == 1 ]]; then
			sudo make install
		fi

		# (re)compile the dummy task
		cd ../dummy_task
		make || die

		exit 0
		;;
	K)
		# Install the kernel
		sudo dpkg -i "${BASEDIR}"/../linux-headers-*"${KERNEL_VERSION_APPEND}"*.deb || die
		sudo dpkg -i "${BASEDIR}"/../linux-image-*"${KERNEL_VERSION_APPEND}"*.deb || die

		exit 0
		;;
	M)
		cd "${BASEDIR}/../duet-module"

		# Install the module
		moddir="`ls -l /lib/modules | grep ${KERNEL_VERSION_APPEND} | \
			cut -d' ' -f 9 | head -1`"
		sudo make -C /lib/modules/${moddir}/build M=$(pwd) modules_install
		sudo depmod
		sudo modprobe duet

		echo "Done installing Duet module."
		exit 0
		;;
	T)
		cd "${BASEDIR}/../duet-tools"

		# Install the duet tools (in /usr/local/bin)
		cd duet-progs
		sudo make install || die

		exit 0
		;;
	u)
		# Get installed packages for all but latest Duet kernel
		DPKGS="`dpkg --get-selections | grep -E 'linux-.*+duet' | \
			cut -f1 | grep -v $KERNEL_VERSION_APPEND | tr '\n' ' '`"
		sudo dpkg -P $DPKGS || die

		# Get .deb packages in BASEDIR of all but latest Duet kernel
		cd "${BASEDIR}/.."
		ls | grep -E 'linux-.*.deb' | grep -v $KERNEL_VERSION_APPEND | \
			tr '\n' ' ' | xargs rm -v || die

		exit 0
		;;
	\?)
		echo "Invalid option: -$OPTARG" >&2
        usage
        exit 1
		;;
	esac
done

usage
exit 1
