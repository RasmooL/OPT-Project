#!/bin/sh
# embedded options to qsub - start with #PBS
# -- Name of the job ---
#PBS -N Lundbeck
# ?- specify queue --
#PBS -q hpc
# ?- number of processors/cores/nodes --
#PBS -l nodes=1:ppn=9
# ?- user email address --
#PBS -M s142944@student.dtu.dk
# ?- mail notification ?-
#PBS -m abe
# -- run in the current working (submission) directory --
if test X$PBS_ENVIRONMENT = XPBS_BATCH; then cd $PBS_O_WORKDIR; fi

lscpu
./opt
