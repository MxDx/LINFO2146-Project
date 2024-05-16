import socket
import argparse
import sys
import time
import paho.mqtt.client as mqtt

def process_gateway_data(data):
    # /barn_number/topic/=payload\n
    data = data.split("\n")
    for i in range(len(data)):
        if data[i][0] != "/":
            continue
        data[i] = data[i][1:]
        barn_number, topic, payload = data[i].split("/")
        payload = payload.split("=")[1]
        print(f"Barn number: {barn_number}, Topic: {topic}, Payload: {payload}")


def recv(sock):
    data = sock.recv(1)
    buf = b""
    while data.decode("utf-8") != "\n":
        buf += data
        data = sock.recv(1)
    return buf

def main(ip, port):
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.connect((ip, port))

    while True: 
        # sock.send(b"/1/lights/=on\n")
        data = recv(sock)
        process_gateway_data(data.decode("utf-8"))

        # time.sleep(1)


if __name__ == "__main__":

    parser = argparse.ArgumentParser()
    parser.add_argument("--ip", dest="ip", type=str)
    parser.add_argument("--port", dest="port", type=int)
    args = parser.parse_args()

    main(args.ip, args.port)

