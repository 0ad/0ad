import RLAPI_pb2_grpc
import RLAPI_pb2
import grpc
import time

channel = grpc.insecure_channel('localhost:50051')
stub = RLAPI_pb2_grpc.RLAPIStub(channel)
print('created stub')

# TODO: Create ConnectRequest
req = RLAPI_pb2.ConnectRequest()
print('connecting...')
obs = stub.Connect(req)
print('Connected')

while True:
    # TODO: Create Action...
    action = RLAPI_pb2.Actions()
    obs = stub.Step(action)
    print('stepping')
    time.sleep(1)

