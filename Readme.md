# MD5-CPC
This project is forked from Marc Steven's Hashclash project [hashclash](https://marc-stevens.nl/p/hashclash/) and follows GPL.

# Details
Same to hashclash, this tool depends on boost. `make` will download libboost and finish everything.

# Compilation
In Makefile.local, one needs to specify the CUDA SDK information to enable CUDA. Several variables need to be set properly, including `CUDA_TK`, `CUDA_SDK`, `CUDA_SAMPLE_DIR` and `CUDA_SAMPLE_DIR`.

# Performance Tuning
In scripts/cpc.sh, several variables are defined:

`CPUS` The number of cores used in the attack.

`CUDAS` The count of CUDA devices used in the attack.

## With CUDA
Using CUDA devices for the 1st stage birthday search is strongly recommended. The default cpc.sh is set to use 1 CPU thread and 3 CUDA devices for the birthday searching.

`$BIRTHDAYSEARCH --inputfile1 "$file1" --inputfile2 "$file2" --hybridbits     0 --pathtyperange 2 --maxblocks 9 --maxmemory 100 --threads 1 --cuda_enable`

## Without CUDA
For platforms without CUDA devices, the number of CPU cores and luck affect the performance significantly. To enable CPU-only birthday searching, cpc.sh needs to be modified as follows.

`$BIRTHDAYSEARCH --inputfile1 "$file1" --inputfile2 "$file2" --hybridbits     0 --pathtyperange 2 --maxblocks 9 --maxmemory 100 --threads $(CPUS)`

# Comments
Sources provided AS IS and entirely credit to [Marc Stevens](https://marc-stevens.nl/)


