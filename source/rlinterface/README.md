Regenerate the GRPC CPP files with:

    protoc --grpc_out=. --plugin=protoc-gen-grpc=`which grpc_cpp_plugin` RLInterface.proto
    protoc --cpp_out=. RLInterface.proto
