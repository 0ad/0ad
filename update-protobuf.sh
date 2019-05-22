cd source/rlinterface/proto
protoc --grpc_out=. --plugin=protoc-gen-grpc=`which grpc_cpp_plugin` RLAPI.proto
protoc --cpp_out=. RLAPI.proto
cd ../../..
mv source/rlinterface/proto/RLAPI.pb.{cc,cpp}
mv source/rlinterface/proto/RLAPI.grpc.pb.{cc,cpp}
echo "Generated the required cpp files from the protobuf"

python -m grpc_tools.protoc --python_out=test-client --grpc_python_out=test-client -Isource/rlinterface/proto source/rlinterface/proto/RLAPI.proto
echo "Generated the required python files from the protobuf!"
