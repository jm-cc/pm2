#! /bin/sh

for d in *; do
    if [ -x ${d}/autogen.sh ]; then
	echo "# ## $d ###########"
	( cd $d && ./autogen.sh )
    fi
done
