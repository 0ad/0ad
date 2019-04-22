Regenerate the GRPC CPP files with:

    protoc --grpc_out=. --plugin=protoc-gen-grpc=`which grpc_cpp_plugin` RLAPI.proto
    protoc --cpp_out=. RLAPI.proto
