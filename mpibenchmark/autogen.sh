#! /bin/sh

export M4PATH=./building-tools:../building-tools:${M4PATH}

${AUTOCONF:-autoconf}
