#!/usr/bin/env sh

echo "REFERENCE OUTPUT"
echo "================"

# NOTE: ik using globs is nicer but I want the test outputs in order so I do it this way
for testnum in $(seq 1 30)
do
    filename="scan.t$testnum"
    printf "\n\nBEGIN OUTPUT FOR %s\n" "$filename"
    echo "================"
    # NOTE: `golf_reference` is a symlink pointing to the reference compiler,
    #       and `ms1` is a symlink pointing to the ms1 directory with all the tests
    ./golf_reference "ms1/$filename"
    exitcode=$?
    echo "================"
    printf "END OUTPUT FOR %s\n" "$filename"
    printf "Exit when run on %s: %s\n\n" "$filename" "$exitcode"
done
