import RLAPI_pb2_grpc
import RLAPI_pb2
import grpc
import time
import json

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
    cmds = RLAPI_pb2.Actions()
    cmd1 = {"type": "aichat", "message": "/msg hello??"}
    cmd2 = {"type": "aichat", "message": "/msg anyone there??"}
    cmds.actions.extend([
        RLAPI_pb2.Action(content=json.dumps(cmd1)),
        RLAPI_pb2.Action(content=json.dumps(cmd2))
    ])
    obs = stub.Step(cmds)
    print('stepping... State is')
    print(json.loads(obs.content))
    time.sleep(1)

