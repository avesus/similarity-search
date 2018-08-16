# rm -rf build/
mkdir build/
cd build/
cmake .. -DCMAKE_BUILD_TYPE=Debug
# cmake .. -DCMAKE_BUILD_TYPE=Release
make
data_dir="../../data"
data_set="netflix"
metric="product"

gdb --args \
./simple_lsh \
	-t ${data_dir}/${data_set}/${data_set}_base.fvecs \
	-b ${data_dir}/${data_set}/${data_set}_base.fvecs \
	-q ${data_dir}/${data_set}/${data_set}_query.fvecs  \
	-g ${data_dir}/${data_set}/${data_set}_${metric}_groundtruth.lshbox \
	--transformed_dim 1 \
	--num_bit 32 
