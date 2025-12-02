import paramiko

host = "10.241.0.114"
username = "sgrc-325"
password = "eos31470."

# 建立 SSH Client
ssh = paramiko.SSHClient()
ssh.set_missing_host_key_policy(paramiko.AutoAddPolicy())

print("Connecting to server...")
ssh.connect(host, username=username, password=password)
print("Connected!")

# 要執行的指令
commands = [
    'cd /Users/sgrc-325/Desktop',
    'python "wi-fi 2 usage.py"'
]

for cmd in commands:
    print(f"Running: {cmd}")
    stdin, stdout, stderr = ssh.exec_command(cmd)
    output = stdout.read().decode()
    error = stderr.read().decode()

    if output:
        print("--- Output ---")
        print(output)
    if error:
        print("--- Error ---")
        print(error)

# 關閉 SSH
ssh.close()
print("Done.")
