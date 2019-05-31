cd source/rlinterface/proto
protoc --grpc_out=. --plugin=protoc-gen-grpc=`which grpc_cpp_plugin` RLAPI.proto
protoc --cpp_out=. RLAPI.proto
cd ../../..
mv source/rlinterface/proto/RLAPI.pb.{cc,cpp}
mv source/rlinterface/proto/RLAPI.grpc.pb.{cc,cpp}
echo "Generated the required cpp files from the protobuf"

mkdir -p source/tools/clients/python/zero_ad/proto/zero_ad
cp source/rlinterface/proto/RLAPI.proto source/tools/clients/python/zero_ad
cd source/tools/clients/
python -m grpc_tools.protoc --python_out=python --grpc_python_out=python -Ipython python/zero_ad/RLAPI.proto
rm python/zero_ad/RLAPI.proto
echo "Generated the required python files from the protobuf!"
