#make clean all
make DEBUG=0 -j4
sudo make install
sudo cp /home/stewart/Dropbox/nvml/src/nondebug/*.* /usr/lib64/
sudo cp /home/stewart/Dropbox/nvml/src/nondebug/*.* /usr/lib/


cd pmc_driver/pmc/
./install.sh
