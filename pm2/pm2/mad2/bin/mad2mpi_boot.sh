#!/bin/sh
##########

echo "Calling recon"
recon   -v ${MAD2_ROOT}/.mad2_conf
echo Calling lamboot ...
lamboot -v ${MAD2_ROOT}/.mad2_conf

##########
