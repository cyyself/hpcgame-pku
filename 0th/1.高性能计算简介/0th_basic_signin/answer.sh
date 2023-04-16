#!/bin/bash
srun -N1 -n1 -c1 -o output.dat ./program "$1"
seff "$(cat job_id.dat)" > seff.dat