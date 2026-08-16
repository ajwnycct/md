
## MD: a Max external for fast discovery of motivic patterns in symbolic music via lossy compression

### Description

C source for the MD Max external.    

MD (<ins>m</ins>otive <ins>d</ins>iscovery) is for real-time identification of maximal motivic patterns in monophonic music ("maximal" meaning MD keeps only motives that are not substrings of other motives). MD treats each potential motive as a pair of complete graphs — one for pitch, one for inter-onset interval — whose edges are the deltas between all pairwise combinations of N consecutive note events, where 3 ≤ N ≤ 16. N is an upper bound: matches are reported for prefix subgraphs of order 3 through N, so a shorter shared opening still registers as a motive. A graph is labelled a motive if it occurs at least twice and is maximal. Each delta is compressed to one of three states — increase, decrease, or no change — stored as a two-bit crumb. The user can query MD at any moment for the top motives by size or occurrence count. Graphs are returned as hexadecimal strings and must be expanded by an auxiliary Max object (under development).

### Dependencies

* [cmake](https://cmake.org/), version 3.31.10 or later
* [git](https://git-scm.com/), version 2.50.1 or later
* [Max](https://cycling74.com/), version 8.6.5+ or version 9.0.7+
* a C23-compliant compiler; must support _BitInt widths up to 256 
* a cmake-supported build system, e.g. Unix Makefiles or XCode on Mac


This is an alpha version, tested on arm64 Mac (M3) using Apple Clang compiler version 17 with the following flags:  
```-Xclang -fexperimental-max-bitint-width=256```

### Building MD

#### Clone and `cd` into the repository

```
git clone --recurse-submodules git@github.com:ajwnycct/md.git
cd md
```
> The URL above assumes communication with Github via SSH. Modify as desired for communication via HTTPS or Github CLI.

#### Build example (Unix Makefiles)

```
cmake -S md -B build -D CMAKE_BUILD_TYPE=RELEASE
cmake --build build --target install
```

#### Build with UNIT test support

```
cmake -S md -B build -D CMAKE_BUILD_TYPE=UNIT
cmake --build build --target install
```
> Run unit tests from `./md/help/md-help.maxpat`. 

### Using MD

If the build process succeeds, the external is written to `./md/externals/md.mxo`.    

On Apple hardware, cmake copies `./md` to `~/Documents/Max 8/Packages/` and `~/Documents/Max 9/Packages/` so that the external is available when Max is launched. On other platforms, you will need to manually copy the `.mxo` file.

To get acquainted with how to use MD in a Max patch, launch `./md/help/md-help.maxpat` in Max.
