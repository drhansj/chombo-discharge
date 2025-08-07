#!/bin/bash
#SBATCH --qos=debug
#SBATCH --time=00:30:00
#SBATCH --nodes=2
#SBATCH --constraint=cpu

srun -N 2 -n 40 ./program2d.Linux.64.CC.ftn.OPTHIGH.MPI.ex example.inputs

