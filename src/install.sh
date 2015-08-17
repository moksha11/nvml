#make clean all
make -j4
sudo make install
sudo cp /home/stewart/codes/nvmalloc/nvml/src/nondebug/*.* /usr/lib64/
sudo cp /home/stewart/codes/nvmalloc/nvml/src/nondebug/*.* /usr/lib/
