#!/bin/sh

DIRNAME=`dirname $0`
MAIN_DIR=`cd $DIRNAME ; cd ../../ ; pwd`

$JAVA_HOME/bin/java -classpath `find $MAIN_DIR/lib -name "*.jar" | tr '\012' ':'` org.runtime.manuals.DumpCourse $MAIN_DIR/data/properties/runtime.props $*

