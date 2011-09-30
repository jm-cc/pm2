#!/bin/bash


# Config
ROOTDIR=/tmp/benchmarks
SVNURL=svn://scm.gforge.inria.fr/svn/pm2/trunk

# Set environment
PERFDIR=${ROOTDIR}/perfs
WWWDIR=${ROOTDIR}/www
WORKDIR=${ROOTDIR}/tmp

export PT_AUTOBENCH=y
export LANG=C
DAY=`date +%d`
MONTH=`date +%b`
YEAR=`date +%G`


#######################################################

# arg1: testclass, testname
update_json_file()
{
    testclass=$1
    testname=$2
    dir="${WWWDIR}/json/${YEAR}/${MONTH}"
    file="${dir}/${testclass}-${testname}-${MONTH}${YEAR}.json"

    [ ! -d ${dir} ] && mkdir -p ${dir}

    echo    "{"                         > ${file}
    echo    "  label: '${testclass}'," >> ${file}
    echo -n "  data: ["                >> ${file}
    for d in `seq 1 ${DAY}`; do
	if [ -f "${testclass}-${TESTNAME}-$d.perf" ]; then
	    perf=`cat "${testclass}-${TESTNAME}-$d.perf"`
	else
	    perf=""
	fi
	echo -n "[$d, ${perf}], "      >> ${file}
    done
    echo "]"                           >> ${file}
    echo "}"                           >> ${file}
}

#######################################################

## Prepare directories
[ ! -d ${ROOTDIR} ] && mkdir ${ROOTDIR}
[ ! -d ${PERFDIR} ] && mkdir ${PERFDIR}
[ ! -d ${WWWDIR} ] && mkdir ${WWWDIR}
[ ! -d ${WORKDIR} ] && mkdir ${WORKDIR}

## Get PM2/Tbx and PM2/Marcel
cd ${WORKDIR}

if [ ! -d src ]; then
    mkdir src
    cd src
    svn co ${SVNURL}/tbx
    svn co ${SVNURL}/marcel
else
    cd ${WORKDIR}/src/tbx && svn up 
    cd ${WORKDIR}/src/marcel && svn up
fi

## Build Tbx, Marcel (SMP) and PthreadMicro
cd ${WORKDIR}
export PKG_CONFIG_PATH=${PKG_CONFIG_PATH}:${WORKDIR}/build/tbx/lib/pkgconfig:${WORKDIR}/build/marcel/lib/pkgconfig
export PATH=${PATH}:${WORKDIR}/build/marcel/bin

# rm -rf build && mkdir build
# cd ${WORKDIR}/src/tbx && ./autogen.sh && ./configure --enable-optimize --prefix=${WORKDIR}/build/tbx && make && make install
# cd ${WORKDIR}/src/marcel && ./autogen.sh && ./configure --enable-optimize --prefix=${WORKDIR}/build/marcel --with-libtype=pthread && make && make install
# cd ${WORKDIR}/src/marcel/benchmarks/pthread-micro && ./autogen.sh && ./configure --prefix=${WORKDIR}/build/benchs && make && make install

## Execute benchmarks
[ ! -d ${PERFDIR}/${YEAR}/${MONTH} ] && mkdir -p ${PERFDIR}/${YEAR}/${MONTH}

cd ${PERFDIR}/${YEAR}/${MONTH}
for test in ${WORKDIR}/build/benchs/bin/*; do
    ## Nptl execution
    RAW_RESULTS=`${test}`  # nom_binaire|temps|nbcpus|op/s
    TESTNAME=`echo ${RAW_RESULTS} | cut -d "|" -f 1 | sed s/_//g`
    NPTL_RES=`echo ${RAW_RESULTS} | cut -d "|" -f 4`
    echo ${NPTL_RES} > "nptl-${TESTNAME}-${DAY}.perf"
    update_json_file "nptl" ${TESTNAME}

    ## Marcel execution
    RAW_RESULTS=`marcel-exec ${test}`
    MARCEL_RES=`echo ${RAW_RESULTS} | cut -d "|" -f 4`
    echo ${MARCEL_RES} > "marcel-${TESTNAME}-${DAY}.perf"
    update_json_file "marcel" ${TESTNAME}
done

## Upload WWWDIR
mkdir /tmp/titi
rsync -avz ${WWWDIR}/ /tmp/titi/
#rsync -avz $BENCH_PATH/www/ scm.gforge.inria.fr:/home/groups/pm2/htdocs/marcel/benchs/