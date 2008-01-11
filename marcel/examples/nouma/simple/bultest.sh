#!/bin/bash

rm /tmp/prof_file_user_sjeuland
#rm /tmp/prof_file_user_silvaan
#make test_01 ; pm2-load -d test_01 #--marcel-nvp 4 --marcel-maxarity 2 #--marcel-xtop
#make test_02 ; pm2-load test_02 #--marcel-nvp 4 --marcel-maxarity 2 #--marcel-xtop
#make test_03 ; pm2-load test_03 #--marcel-nvp 4 --marcel-maxarity 3 #--marcel-xtop
#make test_04 ; pm2-load -d test_04 --marcel-nvp 4 --marcel-maxarity 2 #--marcel-xtop
#make test_05 ; pm2-load -d test_05 --marcel-nvp 4 --marcel-maxarity 2 #--marcel-xtop
make test_06 ; pm2-load -d test_06 --marcel-maxarity 2 #--marcel-xtop
#make test_06 ; pm2-load -d test_06 --marcel-maxarity 2 #--marcel-xtop
#make move_pages ; pm2-load move_pages

cd ~ ; bubbles -x 2000 -y 1000 /tmp/prof_file_user_sjeuland ; cd -
#pm2load bubbles /tmp/prof_file_user_silvaan
