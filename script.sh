clear
make clean
make
cp data/image52.5 data/image.5
./tests/test_debug.sh && ./tests/test_format.sh && ./tests/test_mount.sh && ./tests/test_create.sh && ./tests/test_stat.sh && ./tests/test_copyout.sh
# ./solution/test_goatfs data/image.5 5 copyout-test-1 /tmp
# md5sum /tmp/1.txt | awk '{ print $1}'