
GRID_SIZES="2000 4000 8000 16000"
#ITERS=$(seq 10 10 100)
ITERS=1

for grid_size in $GRID_SIZES ; do
    for iters in $ITERS ; do
        for migration_policy in 0 1 2 ; do
            for initialisation_policy in 0 1 ; do
                pm2-load jacobi $grid_size 16 $iters $migration_policy $initialisation_policy
                sleep 2
            done
        done
    done
done
