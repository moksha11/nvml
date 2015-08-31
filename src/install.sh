#make clean all
make DEBUG=0 -j4
sudo make install
sudo cp nondebug/*.* /usr/lib64/
sudo cp nondebug/*.* /usr/lib/


cd pmc_driver/pmc/
./install.sh
