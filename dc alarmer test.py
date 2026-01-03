import requests

WEBHOOK_URL = "https://discord.com/api/webhooks/1449977134989840405/ME2fc5_gNJOQXKv1HAhzo7z22LYNRygHV8iM3cpird5atljJ22GcnX4NNRQU5LC2-T-v"

payload = {
    "content": "🚨 Discord 告警測試：系統正常運作"
}

response = requests.post(WEBHOOK_URL, json=payload)

if response.status_code == 204:
    print("告警發送成功")
else:
    print("告警發送失敗", response.status_code, response.text)
