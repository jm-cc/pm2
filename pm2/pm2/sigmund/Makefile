all: sigmund supertrace

sigmund:
	echo "Building sigmund\n"; cd sigmund; make; cd ..

supertrace:
	echo "Building supertrace\n"; cd supertrace; make; cd ..

clean:
	cd sigmund; make clean; cd ..
	cd supertrace; make clean; cd ..

.PHONY: all
