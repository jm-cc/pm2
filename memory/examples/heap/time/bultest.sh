#!/bin/bash

rm /tmp/prof_file_user_sjeuland
#rm /tmp/prof_file_user_silvaan
#make time_01 ; pm2-load -d time_01 --marcel-maxarity 2 #--marcel-xtop
#make time_02 ; pm2-load -d time_02 --marcel-maxarity 2 #--marcel-xtop
#make time_03 ; pm2-load -d time_03 --marcel-maxarity 2 #--marcel-xtop
#make time_04 ; pm2-load -d time_04 #--marcel-nvp 4 --marcel-maxarity 2 #--marcel-xtop
#make time_05 ; pm2-load -d time_05 #--marcel-nvp 4 --marcel-maxarity 2 #--marcel-xtop
#make time_06 ; pm2-load -d time_06 --marcel-maxarity 2 #--marcel-xtop
#make time_07 ; pm2-load -d time_07 #--marcel-nvp 4 --marcel-maxarity 2 #--marcel-xtop
#make time_08 ; pm2-load -d time_08 #--marcel-nvp 4 --marcel-maxarity 2 #--marcel-xtop
#make time_09 ; pm2-load -d time_09 #--marcel-nvp 4 --marcel-maxarity 2 #--marcel-xtop
#make time_10 ; pm2-load -d time_10 --marcel-maxarity 2 #--marcel-xtop
make time_11 ; pm2-load -d time_11 --marcel-nvp 4 #--marcel-maxarity 2 #--debug:marcel-log #--marcel-xtop
#make time_11_mono ; pm2-load -d time_11_mono --marcel-nvp 4 #-marcel-maxarity 2 #--marcel-xtop

#make bugheap ;  pm2-load -d bugheap --marcel-maxarity 2

cd ~ ; bubbles -x 2000 -y 1000 /tmp/prof_file_user_sjeuland ; cd -
#pm2load bubbles /tmp/prof_file_user_silvaan
