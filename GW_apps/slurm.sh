#!/bin/bash
#SBATCH -A m1411
#SBATCH -C cpu
#SBATCH --qos=debug
#SBATCH --time=30:00
#SBATCH --nodes=1
#SBATCH --ntasks-per-node=128

srun --ntasks 128 --cpus-per-task 1 --cpu-bind=cores ./program2d.Linux.64.CC.ftn.OPTHIGH.MPI.ex positive2d.inputs
