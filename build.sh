#!/bin/bash

VREV="$(git log -n1 --pretty=oneline 2> /dev/null | cut -d' ' -f1 | tr -d '\n')"

CF="-DHAVE_LIMITS_H -DSTDC_HEADERS -DHAVE_MEMCPY=1 -DVREV=\"$VREV\" -I$(readlink -f .) -D_GNU_SOURCE=1 -O0 -pipe -Wall -Wextra -g -DPACKAGE_DATA_DIR=\".\""
[[ -z "$CC" ]] && CC=gcc
#DEPS=($(pkg-config --print-requires-private ecore-con edbus elementary enotify))
#echo "DEPENDENCIES: ${DEPS[@]}"
CFLAGS="$(pkg-config --cflags eina evas ecore ecore-con elementary azy)"
#echo "DEPENDENCY CFLAGS: $CFLAGS"
LIBS="$(pkg-config --libs eina evas ecore ecore-con elementary azy)"
#echo "DEPENDENCY LIBS: $LIBS"
#echo

link=0
compile=0

#edje_cc -id data/theme data/theme/default.edc default.edj || exit 1

if [[ -f ./emg ]] ; then
	for x in *.h ; do # src/{bin,include,lib}/*.h ; do
		if [[ "$x" -nt ./emg ]] ; then
			compile=1
			break;
		fi
	done
fi

for x in *.c ; do
	[[ $compile == 0 && -f "${x/.c/.o}" && "$x" -ot "${x/.c/.o}" ]] && continue
#	echo "$CC -c $x -o ${x/.c/.o} $CFLAGS $CF || exit 1"
	echo "$CC $x"
	($CC -c $x -o "${x/.c/.o}" $CFLAGS $CF || exit 1)&
	link=1
done

[[ $link == 0 ]] && exit 1
wait
#echo "$CXX *.o -o emg -L/usr/lib -lc $LIBS"
echo "$CC *.o -o emg"
$CC *.o -o emg -L/usr/lib -lc $LIBS
