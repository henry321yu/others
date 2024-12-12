import cv2

# 啟動攝像頭 (預設為攝像頭編號 0)
cap = cv2.VideoCapture(0)

if not cap.isOpened():
    print("無法啟動攝像頭")
    exit()

while True:
    # 讀取攝像頭影像
    ret, frame = cap.read()

    if not ret:
        print("無法獲取影像")
        break

    # 顯示影像
    cv2.imshow("Webcam", frame)

    # 按下 'q' 鍵退出
    if cv2.waitKey(1) & 0xFF == 27:
        break

# 釋放攝像頭並關閉視窗
cap.release()
cv2.destroyAllWindows()
