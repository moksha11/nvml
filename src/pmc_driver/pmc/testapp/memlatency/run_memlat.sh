#LD_PRELOAD=/home/sudarsun/NVM_EMUL/pmc_driver/pmc/librdpmc.so 
LD_PRELOAD=/usr/lib/librdpmc.so taskset --cpu-list 4 ./MemoryCacheProfiler 3
#./MemoryCacheProfiler 3
