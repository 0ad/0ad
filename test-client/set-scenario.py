import argparse
import RLAPI_pb2_grpc
import RLAPI_pb2
import grpc
import json
import sys

parser = argparse.ArgumentParser()
parser.add_argument('--host', default='localhost:50051')
parser.add_argument('--map', default='scenarios/Battle for the Tiber')
args = parser.parse_args()

channel = grpc.insecure_channel(args.host)
stub = RLAPI_pb2_grpc.RLAPIStub(channel)

print('Connecting...', file=sys.stderr)
req = RLAPI_pb2.ConnectRequest(map=args.map)
obs = stub.Connect(req)
print('Connected', file=sys.stderr)
print(obs.content)
