import socket
import argparse
import sys
import time
import paho.mqtt.client as mqtt_client

# Global variables
irrigation_time = 5         # Time in seconds
light_treshold = 240        # Light treshold
irrigation_every = 60       # Time in seconds
time_on = 1                 # Time in minutes

sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

def process_gateway_data(data, mqttc=None):
    # /barn_number/topic/=payload\n
    data = data.split("\n")
    for i in range(len(data)):
        if data[i][0] != "/":
            continue
        data[i] = data[i][1:]
        barn_number, topic, payload = data[i].split("/")
        payload = payload.split("=")[1]
        if mqttc:
            mqttc.publish(f"/{barn_number}/{topic}", payload)
        if (topic == "keep_alive"):
            return
        print(f"/{barn_number}/{topic}/={payload}")
        if (topic == "light"):
            light_value = int(payload)
            if (light_value > light_treshold):
                print(f"Turning on lights in barn {barn_number} for {time_on} minutes")
                sock.send(f"/{barn_number}/lights/=on?{time_on}\n".encode("utf-8"))
            

def recv(sock):
    data = sock.recv(1)
    buf = b""
    while data.decode("utf-8") != "\n":
        buf += data
        data = sock.recv(1)
    return buf

def main(ip, port, mqtt):
    sock.connect((ip, port))

    # The callback for when the client receives a CONNACK response from the server.
    def on_connect(client, userdata, flags, reason_code, properties):
        print(f"Connected with result code {reason_code}")
        # Subscribing in on_connect() means that if we lose the connection and
        # reconnect then subscriptions will be renewed.
        client.subscribe("/#")

    # The callback for when a PUBLISH message is received from the server.
    def on_message(client, userdata, msg):
        barn_number, topic = msg.topic[1:].split("/")
        msg.payload = msg.payload.decode("utf-8")
        if (topic != "lights" and topic != "irrigation"):
            return
        sock.send(f"/{barn_number}/{topic}/={msg.payload}\n".encode("utf-8"))
        print(f"Sent: /{barn_number}/{topic}/={msg.payload}")

    if mqtt:
        mqttc = mqtt_client.Client(mqtt_client.CallbackAPIVersion.VERSION2)
        mqttc.on_connect = on_connect
        mqttc.on_message = on_message

        mqttc.connect("localhost", 1883, 60)

    start_time = time.time()
    while True: 
        if time.time() - start_time > irrigation_every:
            print(f"Sending irrigation time: {irrigation_time}")
            sock.send(f"/-1/irrigation/={irrigation_time}\n".encode("utf-8"))
            start_time = time.time()
        data = recv(sock)
        if mqtt:
            process_gateway_data(data.decode("utf-8"), mqttc)
            mqttc.loop(timeout=0.1)
        else:
            process_gateway_data(data.decode("utf-8"))

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--ip", dest="ip", type=str)
    parser.add_argument("--port", dest="port", type=int)
    parser.add_argument("--mqtt", dest="mqtt", type=bool, default=False)
    args = parser.parse_args()

    main(args.ip, args.port, args.mqtt)

