import argparse
import RLAPI_pb2_grpc
import RLAPI_pb2
import grpc
import json
import sys

parser = argparse.ArgumentParser()
parser.add_argument('--host', default='localhost:50051')
parser.add_argument('--type', default='scenario')
parser.add_argument('--name', default='Battle for the Tiber')
args = parser.parse_args()

channel = grpc.insecure_channel(args.host)
stub = RLAPI_pb2_grpc.RLAPIStub(channel)

print('Connecting...', file=sys.stderr)
scenario = RLAPI_pb2.ScenarioConfig(type=args.type, name=args.name)
req = RLAPI_pb2.ConnectRequest(scenario=scenario)
obs = stub.Connect(req)
print('Connected', file=sys.stderr)
print(obs.content)
