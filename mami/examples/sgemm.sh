for matrix_size in 128 256 512 1024 2048 4096 ; do
    for migration_policy in 0 1 2 ; do
        for initialisation_policy in 0 1 ; do
            #for i in $(seq 1 100) ; do
                pm2-load sgemm $matrix_size $migration_policy $initialisation_policy
                sleep 2
            #done
        done
    done
done


