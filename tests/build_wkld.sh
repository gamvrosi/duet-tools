#!/bin/bash

# Generates the designated filebench personality and profile files
# @1: filebench personality name

compileprof () {
	case $1 in
	"var")	# Varmail personality
		fbwarmup=120
		nfiles="`echo "$fssize * 0.7 / 1048576" | bc`"
		fbperson="#
# Varmail personality, as found in Filebench 1.4.9.1, varmail.f
#

set \$dir=$fspath
set \$nfiles=$nfiles
set \$meandirwidth=10000
set \$meanfilesize=1m
set \$appendsize=16k
set \$nthreads=16
set \$iosize=1m

define fileset name=bigfileset, path=\$dir, size=\$meanfilesize,
               entries=\$nfiles, dirwidth=\$meandirwidth, prealloc=80

define process name=filereader, instances=1
{
  thread name=filereaderthread, memsize=10m, instances=\$nthreads
  {
    flowop deletefile name=deletefile1,filesetname=bigfileset
    flowop createfile name=createfile2,filesetname=bigfileset,fd=1
    flowop appendfile name=appendfile2,iosize=\$appendsize,fd=1
    #flowop appendfilerand name=appendfilerand2,iosize=\$appendsize,fd=1
    flowop bwlimit name=serverlimit2, target=appendfile2
    flowop fsync name=fsyncfile2,fd=1
    flowop closefile name=closefile2,fd=1
    flowop openfile name=openfile3,filesetname=bigfileset,fd=1
    flowop readwholefile name=readfile3,fd=1,iosize=\$iosize
    flowop bwlimit name=serverlimit3a, target=readfile3
    flowop appendfile name=appendfile3,iosize=\$appendsize,fd=1
    #flowop appendfilerand name=appendfilerand3,iosize=\$appendsize,fd=1
    flowop bwlimit name=serverlimit3b, target=appendfile3
    flowop fsync name=fsyncfile3,fd=1
    flowop closefile name=closefile3,fd=1
    flowop openfile name=openfile4,filesetname=bigfileset,fd=1
    flowop readwholefile name=readfile4,fd=1,iosize=\$iosize
    flowop bwlimit name=serverlimit4, target=readfile4
    flowop closefile name=closefile4,fd=1
    #flowop opslimit name=serverlimit
  }
}

echo  \"Varmail Version 3.0 personality successfully loaded\""
		;;

	"wsv")	# Webserver personality
		fbwarmup=120
		nfiles="`echo "$fssize * 0.7 / 1048576" | bc`"
		fbperson="#
# Webserver personality, as found in Filebench 1.4.9.1, webserver.f
#

set \$dir=$fspath
set \$nfiles=$nfiles
set \$meandirwidth=20
set \$meanfilesize=1m
set \$nthreads=1
set \$iosize=1m
set \$appendsize=128k

define fileset name=bigfileset, path=\$dir, size=\$meanfilesize,
               entries=\$nfiles, dirwidth=\$meandirwidth, prealloc=100
define fileset name=logfiles, path=\$dir, size=\$meanfilesize,
               entries=1, dirwidth=\$meandirwidth, prealloc

define process name=filereader,instances=1
{
  thread name=filereaderthread,memsize=10m,instances=\$nthreads
  {
    flowop openfile name=openfile1,filesetname=bigfileset,fd=1
    flowop readwholefile name=readfile1,fd=1,iosize=\$iosize
    flowop bwlimit name=serverlimit1, target=readfile1
    flowop closefile name=closefile1,fd=1
    flowop openfile name=openfile2,filesetname=bigfileset,fd=1
    flowop readwholefile name=readfile2,fd=1,iosize=\$iosize
    flowop bwlimit name=serverlimit2, target=readfile2
    flowop closefile name=closefile2,fd=1
    flowop openfile name=openfile3,filesetname=bigfileset,fd=1
    flowop readwholefile name=readfile3,fd=1,iosize=\$iosize
    flowop bwlimit name=serverlimit3, target=readfile3
    flowop closefile name=closefile3,fd=1
    flowop openfile name=openfile4,filesetname=bigfileset,fd=1
    flowop readwholefile name=readfile4,fd=1,iosize=\$iosize
    flowop bwlimit name=serverlimit4, target=readfile4
    flowop closefile name=closefile4,fd=1
    flowop openfile name=openfile5,filesetname=bigfileset,fd=1
    flowop readwholefile name=readfile5,fd=1,iosize=\$iosize
    flowop bwlimit name=serverlimit5, target=readfile5
    flowop closefile name=closefile5,fd=1
    flowop openfile name=openfile6,filesetname=bigfileset,fd=1
    flowop readwholefile name=readfile6,fd=1,iosize=\$iosize
    flowop bwlimit name=serverlimit6, target=readfile6
    flowop closefile name=closefile6,fd=1
    flowop openfile name=openfile7,filesetname=bigfileset,fd=1
    flowop readwholefile name=readfile7,fd=1,iosize=\$iosize
    flowop bwlimit name=serverlimit7, target=readfile7
    flowop closefile name=closefile7,fd=1
    flowop openfile name=openfile8,filesetname=bigfileset,fd=1
    flowop readwholefile name=readfile8,fd=1,iosize=\$iosize
    flowop bwlimit name=serverlimit8, target=readfile8
    flowop closefile name=closefile8,fd=1
    flowop openfile name=openfile9,filesetname=bigfileset,fd=1
    flowop readwholefile name=readfile9,fd=1,iosize=\$iosize
    flowop bwlimit name=serverlimit9, target=readfile9
    flowop closefile name=closefile9,fd=1
    flowop openfile name=openfile10,filesetname=bigfileset,fd=1
    flowop readwholefile name=readfile10,fd=1,iosize=\$iosize
    flowop bwlimit name=serverlimit10, target=readfile10
    flowop closefile name=closefile10,fd=1
    flowop appendfile name=appendlog,filesetname=logfiles,iosize=\$appendsize,fd=2
    flowop bwlimit name=serverlimitA, target=appendlog
  }
}

echo  \"Web-server Version 3.0 personality successfully loaded\""
		;;

	"wpx")	# Webproxy personality
		fbwarmup=120
		nfiles="`echo "$fssize * 0.9 / 1048576" | bc`" # we only prealloc 80%
		fbperson="#
# Webproxy personality, as found in Filebench 1.4.9.1, webproxy.f
#

set \$dir=$fspath
set \$nfiles=$nfiles
# set \$meandirwidth=1000000
set \$meandirwidth=20
set \$meanfilesize=1m
set \$nthreads=16
set \$meaniosize=1m
set \$iosize=1m

define fileset name=bigfileset,path=\$dir,size=\$meanfilesize,entries=\$nfiles,
               dirwidth=\$meandirwidth,prealloc=80

define process name=proxycache,instances=1
{
  thread name=proxycache,memsize=10m,instances=\$nthreads
  {
    flowop deletefile name=deletefile1,filesetname=bigfileset
    flowop createfile name=createfile1,filesetname=bigfileset,fd=1
    flowop appendfile name=appendfile1,iosize=\$meaniosize,fd=1
    flowop bwlimit name=serverlimit1, target=appendfile1
    flowop closefile name=closefile1,fd=1
    flowop openfile name=openfile2,filesetname=bigfileset,fd=1
    flowop readwholefile name=readfile2,fd=1,iosize=\$iosize
    flowop bwlimit name=serverlimit2, target=readfile2
    flowop closefile name=closefile2,fd=1
    flowop openfile name=openfile3,filesetname=bigfileset,fd=1
    flowop readwholefile name=readfile3,fd=1,iosize=\$iosize
    flowop bwlimit name=serverlimit3, target=readfile3
    flowop closefile name=closefile3,fd=1
    flowop openfile name=openfile4,filesetname=bigfileset,fd=1
    flowop readwholefile name=readfile4,fd=1,iosize=\$iosize
    flowop bwlimit name=serverlimit4, target=readfile4
    flowop closefile name=closefile4,fd=1
    flowop openfile name=openfile5,filesetname=bigfileset,fd=1
    flowop readwholefile name=readfile5,fd=1,iosize=\$iosize
    flowop bwlimit name=serverlimit5, target=readfile5
    flowop closefile name=closefile5,fd=1
    flowop openfile name=openfile6,filesetname=bigfileset,fd=1
    flowop readwholefile name=readfile6,fd=1,iosize=\$iosize
    flowop bwlimit name=serverlimit6, target=readfile6
    flowop closefile name=closefile6,fd=1
  }
}

echo  \"Web proxy-server Version 3.0 personality successfully loaded\""
		;;

	"fsv")	# Fileserver personality
		fbwarmup=120
		nfiles="`echo "$fssize * 0.7 / 2097152" | bc`"
		fbperson="#
# Fileserver personality, as found in Filebench 1.4.9.1, fileserver.f
#

set \$dir=$fspath
set \$nfiles=$nfiles
set \$meandirwidth=20
#set \$meanfilesize=1m
set \$meanfilesize=2m
set \$nthreads=16
set \$iosize=1m
set \$appendsize=16k

define fileset name=bigfileset, path=\$dir, size=\$meanfilesize, entries=\$nfiles,
               dirwidth=\$meandirwidth, prealloc=97

define process name=filereader, instances=1
{
  thread name=filereaderthread, memsize=10m, instances=\$nthreads
  {
    flowop createfile name=createfile1,filesetname=bigfileset,fd=1
    flowop writewholefile name=wrtfile1,srcfd=1,fd=1,iosize=\$iosize
    flowop bwlimit name=serverlimitW, target=wrtfile1
    flowop closefile name=closefile1,fd=1
    flowop openfile name=openfile1,filesetname=bigfileset,fd=1
    flowop appendfile name=appendfile1,iosize=\$appendsize,fd=1
    flowop bwlimit name=serverlimitA, target=appendfile1
    flowop closefile name=closefile2,fd=1
    flowop openfile name=openfile2,filesetname=bigfileset,fd=1
    flowop readwholefile name=readfile1,fd=1,iosize=\$iosize
    flowop bwlimit name=serverlimitR, target=readfile1
    flowop closefile name=closefile3,fd=1
    flowop deletefile name=deletefile1,filesetname=bigfileset
    flowop statfile name=statfile1,filesetname=bigfileset
  }
}

echo  \"File-server Version 3.0 personality successfully loaded\""
		;;
	esac

	echo -e "$fbperson" > fbperson.f

	echo -e "load fbperson\ncreate filesets\ncreate processes
stats clear\nsleep $fbwarmup\nstats snap" > $fbprof
	iters=`echo $explen/$profgran | bc`
	for i in $(seq 1 $iters); do
		echo -e "stats clear\nsleep $profgran\nstats snap" >> $fbprof
	done
	echo -e "shutdown processes\nquit" >> $fbprof
}
