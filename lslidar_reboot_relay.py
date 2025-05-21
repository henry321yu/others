import socket
import time

RELAY_PORT = 2371
ras_ip='10.241.156.153'

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

print("reboot lidar...")

print("OFF 5s")
sock.sendto(b'OFF', (ras_ip, RELAY_PORT))  # 關閉繼電器
time.sleep(5)

print("ON")
sock.sendto(b'ON', (ras_ip, RELAY_PORT))   # 打開繼電器
time.sleep(1)

sock.close()