#!/bin/bash

if [ -z "$3" ] ; then
    echo "Error. Syntax: $0 <machine1> <machine2> <network1> ... <networkn>"
    exit
fi

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
    if [ ! -s architecture ] ; then
	ssh $machine1 uname -m > architecture
    fi
    arch=`cat architecture` #echo "Arch = $arch"
    rm -f architecture

    #compilation du test d'échantillonage avec une flavor prédéfinie
    prog=$(pm2-which -f $flavor sampling-prog 2>/dev/null)
    if [ -z $prog ] ; then
        echo "***Compilation of the sampling program"

        echo "ssh $machine1 make FLAVOR=$flavor -C $PM2_ROOT/nmad/sampling sampling-prog 2>&1 | tee /tmp/compil"

        ssh $machine1 make FLAVOR=$flavor -C $PM2_ROOT/nmad/sampling sampling-prog 2>&1 | tee /tmp/compil
        rm -f /tmp/compil
    fi
 
    LEONIE_FLAVOR=$(pm2-flavor get --flavor=leonie 2>/dev/null)
    if [ -z "$LEONIE_FLAVOR" ] ; then
        pm2-create-sample-flavors leonie
    fi
    LEONIE_DIR=$(pm2-config --flavor=leonie --bindir leonie 2>/dev/null)
    if [ ! -x "$LEONIE_DIR"/leonie ] ; then
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

    # lancement de l'échantillonnage
    sampling_file="${PM2_ROOT}/nmad/sampling/${network}_${arch}_samplings.nm_ns"
    echo "***Launching of the sampling of ${network}"
    pm2-load --network $network_file --flavor $flavor sampling-prog  2>&1 | tee $sampling_file

    #cat $sampling_file
    echo "***Sampling Finished"
done


