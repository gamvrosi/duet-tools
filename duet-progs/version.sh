#!/bin/bash
#
# report a useful version for releases
#
# Copyright 2016, George Amvrosiadis <gamvrosi@gmail.com>
# Released under the GNU GPLv2
 
v="v1.2"

which git &> /dev/null
if [ $? == 0 -a -d .git ]; then
	if head=`git rev-parse --verify HEAD 2>/dev/null`; then
		if tag=`git describe --tags 2>/dev/null`; then
			v="$tag"
		fi

		# Are there uncommitted changes?
		git update-index --refresh --unmerged > /dev/null
		if git diff-index --name-only HEAD | grep -v "^scripts/package" \
		   | read dummy; then
			v="$v"-dirty
		fi
	fi
fi

echo "#ifndef __BUILD_VERSION" > .build-version.h
echo "#define __BUILD_VERSION" >> .build-version.h
echo "#define DUET_BUILD_VERSION \"Duet $v\"" >> .build-version.h
echo "#endif" >> .build-version.h

diff -q version.h .build-version.h >& /dev/null

if [ $? == 0 ]; then
	rm .build-version.h
	exit 0
fi

mv .build-version.h version.h
