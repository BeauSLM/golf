#!/usr/bin/env sh

make -k

# NOTE: ik using globs is nicer but I want the test outputs in order so I do it this way
for testnum in $(seq 1 30)
do
    filename="scan.t$testnum"
    printf "\n\nBEGIN OUTPUT FOR %s\n" "$filename"
    echo "================"
    ./golf "reference/ms1/$filename"
    exitcode=$?
    echo "================"
    printf "END OUTPUT FOR %s\n" "$filename"
    printf "Exit when run on %s: %s\n\n" "$filename" "$exitcode"
done
