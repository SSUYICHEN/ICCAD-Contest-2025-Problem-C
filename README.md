#### This is README file for the final project of "Physical Design for Nanometer ICs"
#### Author: ³¯¤Ñ´I (D11K42001) and ³¯«äÀ[ (R12943161)  
#### Date: 2025/06/10

## Synopsis
This program targets the ICCAD Contest 2025 Problem C.
For more information, please see [the contest page](https://www.iccad-contest.org/Problems.html).

## Build

First, one should download the OpenRoad tool and reset it to the version that we develop on:

```
git clone --recursive https://github.com/The-OpenROAD-Project/OpenROAD-flow-scripts
cd OpenROAD-flow-scripts
git reset --hard 00c7965
git submodule update --init --recursive
```

Since we have modified some source files in OpenRoad, please copy these files to corresponding directories and overwrite the original files:

```
cp ../src_rsz/Resizer.i ./tools/OpenROAD/src/rsz/src
cp ../src_rsz/Resizer.tcl ./tools/OpenROAD/src/rsz/src
cp ../src_rsz/Resizer.cc ./tools/OpenROAD/src/rsz/src
cp ../src_rsz/RepairSetup.hh ./tools/OpenROAD/src/rsz/src
cp ../src_rsz/RepairSetup.cc ./tools/OpenROAD/src/rsz/src
cp ../src_rsz/Resizer.hh ./tools/OpenROAD/src/rsz/include/rsz
```

Then, build the OpenRoad tool by the following commands.

```
cd ./tools/OpenROAD/
sudo ./etc/DependencyInstaller.sh -dev -all
cd ../..
./build_openroad.sh --local
source ./env.sh
```

Finally, build our program by:

```
cd ..
make
```

## Execution

The help message concludes the details for execution:

```
Usage: ./opt <main_folder> <alpha> <beta> <gamma> <output_def> <output_pl> <output_eco>
Example: ./opt ./ICCAD25_PorbC 0.334 0.333 0.333 pd25.def pd25.pl pd25.changelist
```

Please note that the contest requires the updated `.pl` file and the ECO changelist, but the evaluation script only takes a `.def` file, so we decide to output all of them.

To obtain the metrics, use the following commands under the folder `./ICCAD25_PorbC/openRoad_eval_script`, which give the TNS, power, and total wire length (in HPWL) of the design.

```
openroad eval_my.tcl
```

For the matrics of the original design, use the following commands under the same folder.

```
openroad eval_def.tcl
```

Similarly, using the following command under the same folder generates the baseline design called `baseline.def` and reports the matrics of it.
```
openroad eval_baseline.tcl
```

We also provide a simple program called `discal` to calculate the displacement bewteen two `.def` files, which can be used by

```
./discal <original_def> <new_def>
```
