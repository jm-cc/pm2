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

    flavor="mad4-sampling-"$network #echo "flavor = $flavor"

    #récupération de l'architecture sur laquelle on va lancer l'échantillonnnage
    ssh $machine1 arch > architecture
    arch=`cat architecture` #echo "Arch = $arch"
    rm -f architecture

#compilation du test d'échantillonage avec une flavor prédéfinie
    if [ ! -f $PM2_BUILD_DIR/${arch}/$flavor/examples/bin/sampling-prog ] ; then
        echo "***Compilation of the sampling program"

        echo "ssh $machine1 make FLAVOR=$flavor -C $PM2_ROOT/mad4/examples/ sampling-prog > /tmp/compil  2>&1"

        ssh $machine1 make FLAVOR=$flavor -C $PM2_ROOT/mad4/examples/ sampling-prog > /tmp/compil  2>&1
        rm -f /tmp/compil
    fi

    if [ ! -f $PM2_BUILD_DIR/${arch}/leonie/leonie/bin/leonie ] ; then
        echo "***Compilation of leonie"
        make FLAVOR=leonie -C $PM2_ROOT/leonie > /tmp/compil  2>&1
        rm -f /tmp/compil
    fi

#construction des fichiers de config
#*** network.cfg
    network_file="${PM2_ROOT}/mad4/sampling/networks.cfg"

    if [ ! -f $network_file ] ; then
        #echo "Le fichier network.cfg n'existe pas"
        echo "***Creation  of the network file $network_file"
        touch $network_file
    #else
    #    echo "Le fichier network.cfg existe"
    fi

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
    config_file="${PM2_ROOT}/mad4/sampling/${network}_${arch}.cfg"

    if [ ! -f $config_file ] ; then
        #echo "Le fichier ${network}_${arch}.cfg n'existe pas"
        echo "***Creation  of the configuration file $config_file"
        touch $config_file
    #else
    #    echo "Le fichier ${network}_${arch}.cfg existe"
    fi

    (cat <<EOF
application : {
     name     : sampling;
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
    sampling_file="${PM2_ROOT}/mad4/sampling/${network}_${arch}_samplings.nm_ns"
    if [ ! -f $sampling_file ] ; then
        #echo "Le fichier ${network}_${arch}_samplings.nm_ns n'existe pas"
        #echo "Création  de $sampling_file"
        touch $sampling_file
    #else
    #    echo "Le fichier ${network}_${arch}_samplings.nm_ns existe"
    fi

    pm2-conf -f $flavor $machine1 $machine2 > /tmp/pm2conf  2>&1
    rm -f /tmp/pm2conf
    echo "***Launching of the sampling of ${network}"
    pm2-load -f $flavor --protocol $network sampling-prog > $sampling_file 2>&1

    #cat $sampling_file
    echo "***Sampling Finished"
done


