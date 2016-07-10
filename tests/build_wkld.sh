#!/bin/bash

# Generates the designated filebench personality and profile files
# @1: filebench personality name

compileprof () {
	case $1 in
	"var")	# Varmail personality
		nfiles="`echo "$fssize * 0.4 / 1048576" | bc`"
		fbperson="#
# Varmail personality, as found in Filebench 1.5-alpha1, varmail.f
#

set \$dir=$fspath
set \$nfiles=$nfiles
set \$meandirwidth=10000
set \$filesize=cvar(type=cvar-gamma,parameters=mean:1048576;gamma:1.5)
set \$nthreads=16
set \$iosize=1m
set \$meanappendsize=16k

define fileset name=bigfileset,path=\$dir,size=\$filesize,entries=\$nfiles,
					dirwidth=\$meandirwidth,prealloc=80

define process name=filereader,instances=1
{
  thread name=filereaderthread,memsize=10m,instances=\$nthreads
  {
    flowop deletefile name=deletefile1,filesetname=bigfileset
    flowop createfile name=createfile2,filesetname=bigfileset,fd=1
    flowop appendfile name=appendfile2,iosize=\$meanappendsize,fd=1
    #flowop appendfilerand name=appendfilerand2,iosize=\$meanappendsize,fd=1
    flowop fsync name=fsyncfile2,fd=1
    flowop closefile name=closefile2,fd=1
    flowop openfile name=openfile3,filesetname=bigfileset,fd=1
    flowop readwholefile name=readfile3,fd=1,iosize=\$iosize
    flowop appendfile name=appendfile3,iosize=\$meanappendsize,fd=1
    #flowop appendfilerand name=appendfilerand3,iosize=\$meanappendsize,fd=1
    flowop fsync name=fsyncfile3,fd=1
    flowop closefile name=closefile3,fd=1
    flowop openfile name=openfile4,filesetname=bigfileset,fd=1
    flowop readwholefile name=readfile4,fd=1,iosize=\$iosize
    flowop closefile name=closefile4,fd=1
  }
}

echo  \"Varmail Version 3.0 personality successfully loaded\""
		;;

	"wsv")	# Webserver personality
		nfiles="`echo "$fssize * 0.4 / 1048576" | bc`"
		fbperson="#
# Webserver personality, as found in Filebench 1.5-alpha1, webserver.f
#

set \$dir=$fspath
set \$nfiles=$nfiles
set \$meandirwidth=20
set \$filesize=cvar(type=cvar-gamma,parameters=mean:1048576;gamma:1.5)
set \$nthreads=1
set \$iosize=1m
set \$meanappendsize=128k

define fileset name=bigfileset,path=\$dir,size=\$filesize,entries=\$nfiles,
					dirwidth=\$meandirwidth,prealloc=100
define fileset name=logfiles,path=\$dir,size=\$filesize,entries=1,
					dirwidth=\$meandirwidth,prealloc

define process name=filereader,instances=1
{
  thread name=filereaderthread,memsize=10m,instances=\$nthreads
  {
    flowop openfile name=openfile1,filesetname=bigfileset,fd=1
    flowop readwholefile name=readfile1,fd=1,iosize=\$iosize
    flowop closefile name=closefile1,fd=1
    flowop openfile name=openfile2,filesetname=bigfileset,fd=1
    flowop readwholefile name=readfile2,fd=1,iosize=\$iosize
    flowop closefile name=closefile2,fd=1
    flowop openfile name=openfile3,filesetname=bigfileset,fd=1
    flowop readwholefile name=readfile3,fd=1,iosize=\$iosize
    flowop closefile name=closefile3,fd=1
    flowop openfile name=openfile4,filesetname=bigfileset,fd=1
    flowop readwholefile name=readfile4,fd=1,iosize=\$iosize
    flowop closefile name=closefile4,fd=1
    flowop openfile name=openfile5,filesetname=bigfileset,fd=1
    flowop readwholefile name=readfile5,fd=1,iosize=\$iosize
    flowop closefile name=closefile5,fd=1
    flowop openfile name=openfile6,filesetname=bigfileset,fd=1
    flowop readwholefile name=readfile6,fd=1,iosize=\$iosize
    flowop closefile name=closefile6,fd=1
    flowop openfile name=openfile7,filesetname=bigfileset,fd=1
    flowop readwholefile name=readfile7,fd=1,iosize=\$iosize
    flowop closefile name=closefile7,fd=1
    flowop openfile name=openfile8,filesetname=bigfileset,fd=1
    flowop readwholefile name=readfile8,fd=1,iosize=\$iosize
    flowop closefile name=closefile8,fd=1
    flowop openfile name=openfile9,filesetname=bigfileset,fd=1
    flowop readwholefile name=readfile9,fd=1,iosize=\$iosize
    flowop closefile name=closefile9,fd=1
    flowop openfile name=openfile10,filesetname=bigfileset,fd=1
    flowop readwholefile name=readfile10,fd=1,iosize=\$iosize
    flowop closefile name=closefile10,fd=1
    flowop appendfilerand name=appendlog,filesetname=logfiles,iosize=\$meanappendsize,fd=2
  }
}

echo  \"Web-server Version 3.1 personality successfully loaded\""
		;;

	"wpx")	# Webproxy personality
		nfiles="`echo "$fssize * 0.5 / 1048576" | bc`" # we only prealloc 80%
		fbperson="#
# Webproxy personality, as found in Filebench 1.5-alpha1, webproxy.f
#

set \$dir=$fspath
set \$nfiles=$nfiles
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
    flowop appendfilerand name=appendfilerand1,iosize=\$meaniosize,fd=1
    flowop closefile name=closefile1,fd=1
    flowop openfile name=openfile2,filesetname=bigfileset,fd=1
    flowop readwholefile name=readfile2,fd=1,iosize=\$iosize
    flowop closefile name=closefile2,fd=1
    flowop openfile name=openfile3,filesetname=bigfileset,fd=1
    flowop readwholefile name=readfile3,fd=1,iosize=\$iosize
    flowop closefile name=closefile3,fd=1
    flowop openfile name=openfile4,filesetname=bigfileset,fd=1
    flowop readwholefile name=readfile4,fd=1,iosize=\$iosize
    flowop closefile name=closefile4,fd=1
    flowop openfile name=openfile5,filesetname=bigfileset,fd=1
    flowop readwholefile name=readfile5,fd=1,iosize=\$iosize
    flowop closefile name=closefile5,fd=1
    flowop openfile name=openfile6,filesetname=bigfileset,fd=1
    flowop readwholefile name=readfile6,fd=1,iosize=\$iosize
    flowop closefile name=closefile6,fd=1
  }
}

echo  \"Web proxy-server Version 3.0 personality successfully loaded\""
		;;

	"fsv")	# Fileserver personality
		nfiles="`echo "$fssize * 0.4 / 2097152" | bc`"
		fbperson="#
# Fileserver personality, as found in Filebench 1.5-alpha1, fileserver.f
#

set \$dir=$fspath
set \$nfiles=$nfiles
set \$meandirwidth=20
set \$meanfilesize=2m
set \$filesize=cvar(type=cvar-gamma,parameters=mean:2097152;gamma:1.5)
set \$nthreads=16
set \$iosize=1m
set \$meanappendsize=16k

define fileset name=bigfileset,path=\$dir,size=\$filesize,entries=\$nfiles,
					dirwidth=\$meandirwidth,prealloc=80

define process name=filereader,instances=1
{
  thread name=filereaderthread,memsize=10m,instances=\$nthreads
  {
    flowop createfile name=createfile1,filesetname=bigfileset,fd=1
    flowop writewholefile name=wrtfile1,srcfd=1,fd=1,iosize=\$iosize
    flowop closefile name=closefile1,fd=1
    flowop openfile name=openfile1,filesetname=bigfileset,fd=1
    flowop appendfilerand name=appendfilerand1,iosize=\$meanappendsize,fd=1
    flowop closefile name=closefile2,fd=1
    flowop openfile name=openfile2,filesetname=bigfileset,fd=1
    flowop readwholefile name=readfile1,fd=1,iosize=\$iosize
    flowop closefile name=closefile3,fd=1
    flowop deletefile name=deletefile1,filesetname=bigfileset
    flowop statfile name=statfile1,filesetname=bigfileset
  }
}

echo  \"File-server Version 3.0 personality successfully loaded\""
		;;
	esac

	echo -e "$fbperson\n\npsrun -$profgran $explen" > fbperson.f
}
