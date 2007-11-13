#!/bin/bash

# ./sampling.sh <machine1> <machine2> <reseau1> <...>

#Directory /home/ebrunet/build/build-i686//leonie/leonie/bin not found

prog=$0
machine1=$1 #echo "machine1 = $machine1"
machine2=$2 #echo "machine2 = $machine2"

shift
shift

for network in $*; do
    #echo "network = $network"

    flavor="nmad-sampling-"$network #echo "flavor = $flavor"

    # flavor creation
    pm2-flavor set --flavor=$flavor --modules="init nmad tbx ntbx" \
        --common="fortran_target_none" --all="opt build_static" \
        --nmad="$network sched_opt strat_default mad3_emu"

    #récupération de l'architecture sur laquelle on va lancer l'échantillonnnage
    ssh $machine1 arch > architecture
    arch=`cat architecture` #echo "Arch = $arch"
    rm -f architecture

    #compilation du test d'échantillonage avec une flavor prédéfinie
    if [ ! -f $PM2_BUILD_DIR/${arch}/$flavor/examples/bin/sampling-prog ] ; then
        echo "***Compilation of the sampling program"

        echo "ssh $machine1 make FLAVOR=$flavor -C $PM2_ROOT/nmad/sampling sampling-prog 2>&1 | tee /tmp/compil"

        ssh $machine1 make FLAVOR=$flavor -C $PM2_ROOT/nmad/sampling sampling-prog 2>&1 | tee /tmp/compil
        rm -f /tmp/compil
    fi

    if [ ! -f $PM2_BUILD_DIR/${arch}/leonie/leonie/bin/leonie ] ; then
        echo "***Compilation of leonie"
        make FLAVOR=leonie -C $PM2_ROOT/leonie 2>&1 | tee /tmp/compil
        rm -f /tmp/compil
    fi

    #construction des fichiers de config
    #*** network.cfg
    network_file="${PM2_ROOT}/nmad/sampling/networks.cfg"

    (cat <<EOF
networks : ({
	name  : $network-net;
	hosts : ($machine1, $machine2);
	dev   : $network;
});
EOF
    ) > $network_file

    #cat $network_file


    #*** $network_$arch.cfg
    config_file="${PM2_ROOT}/nmad/sampling/${network}_${arch}.cfg"

    (cat <<EOF
application : {
     name     : sampling-prog;
     flavor   : $flavor;
     networks : {
          include  : $network_file;
          channels : ({
               name  : channel;
               net   : $network-net;
               hosts : ($machine1, $machine2);
          });
     };
};
EOF
    ) > $config_file

    #cat $config_file

    # lancement de l'échantillonnage
    sampling_file="${PM2_ROOT}/nmad/sampling/${network}_${arch}_samplings.nm_ns"
    echo "***Launching of the sampling of ${network}"
    leonie --x --p $config_file  2>&1 | tee $sampling_file

    #cat $sampling_file
    echo "***Sampling Finished"
done


