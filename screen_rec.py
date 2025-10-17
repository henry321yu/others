import mss
import numpy as np
import pyaudiowpatch as pyaudio
import wave
import threading
import time
from moviepy import editor  # 新版 moviepy 用法
import keyboard
import cv2
from datetime import datetime
import os

# === 錄影參數設定 ===
FPS = 60                # 每秒影格數 (越高畫面越順暢，但檔案越大)
BITRATE = "8000k"       # 影片輸出位元率 (例如: 4000k、6000k、8000k)

# === 自動命名 ===
now = datetime.now()
timestamp = now.strftime("%Y-%m-%d %H-%M-%S")

VIDEO_FILENAME = f"{timestamp}_temp.mp4"
AUDIO_FILENAME = f"{timestamp}_audio.wav"
OUTPUT_FILENAME = f"{timestamp}.mp4"

# === 音訊設定 ===
samplerate = 44100
channels = 2

# === 全域控制旗標 ===
stop_flag = False

# === 錄音函式（PyAudioWPatch + WASAPI loopback）===
def record_audio():
    global stop_flag
    print("開始錄製系統聲音...")
    frames = []

    import pyaudiowpatch as pyaudio
    import wave

    p = pyaudio.PyAudio()

    # 找出支援 loopback 的裝置
    input_device_index = None
    for i in range(p.get_device_count()):
        device_info = p.get_device_info_by_index(i)
        if 'loopback' in device_info['name'].lower():
            input_device_index = i
            break

    if input_device_index is None:
        print("未找到支援 loopback 的裝置。")
        return
    else:
        device_info = p.get_device_info_by_index(input_device_index)
        samplerate = int(device_info['defaultSampleRate'])
        print(f"使用裝置：{device_info['name']}，預設取樣率：{samplerate}")

    stream = p.open(format=pyaudio.paInt16,
                    channels=channels,
                    rate=samplerate,
                    input=True,
                    input_device_index=input_device_index,
                    frames_per_buffer=1024)

    try:
        while not stop_flag:
            data = stream.read(1024)
            frames.append(data)
    except Exception as e:
        print(f"錄音中發生錯誤: {e}")

    stream.stop_stream()
    stream.close()
    p.terminate()

    # 儲存 WAV
    with wave.open(AUDIO_FILENAME, 'wb') as wf:
        wf.setnchannels(channels)
        wf.setsampwidth(2)
        wf.setframerate(samplerate)
        wf.writeframes(b''.join(frames))

    print(f"系統聲音錄音完成，檔案儲存為 {AUDIO_FILENAME}")


# === 錄螢幕函式 ===
def record_screen():
    global stop_flag
    print("開始錄影...")
    sct = mss.mss()
    monitor = sct.monitors[1]
    frames = []
    frame_interval = 1.0 / FPS

    while not stop_flag:
        start_time = time.time()
        img = np.array(sct.grab(monitor))
        frame = img[:, :, :3]  # 移除 alpha 通道
        frames.append(frame)

        # 控制錄影頻率
        elapsed = time.time() - start_time
        if elapsed < frame_interval:
            time.sleep(frame_interval - elapsed)

    print("錄影完成，正在儲存影片...")
    height, width, _ = frames[0].shape
    fourcc = cv2.VideoWriter_fourcc(*"mp4v")
    out = cv2.VideoWriter(VIDEO_FILENAME, fourcc, FPS, (width, height))
    for f in frames:
        out.write(f)
    out.release()
    print("影片儲存完成。")


# === 安全監聽 End 鍵（不會報錯） ===
def stop_on_pause():
    global stop_flag
    print("可按 Pause/Break 鍵停止錄影。")
    while not stop_flag:
        if keyboard.is_pressed("pause"):  # 安全判斷鍵盤狀態
            stop_flag = True
            print("偵測到 Pause/Break 鍵，停止錄影中...")
            break
        time.sleep(0.001)  # 避免佔用過多 CPU

# === 主程式 ===
if __name__ == "__main__":
    print("錄影程式啟動中...")
    print("提示：按下 Pause/Break 鍵可結束錄影。")


    t1 = threading.Thread(target=record_audio)
    t1.start()

    # 等 0.2 秒再啟動，減少不同步
    time.sleep(4)    
    t2 = threading.Thread(target=record_screen)
    t2.start()

    # 安全監聽鍵盤停止
    threading.Thread(target=stop_on_pause, daemon=True).start()

    t1.join()
    t2.join()

    print("合併聲音與畫面中...")
    video_clip = editor.VideoFileClip(VIDEO_FILENAME)
    audio_clip = editor.AudioFileClip(AUDIO_FILENAME)
    final_clip = video_clip.set_audio(audio_clip)

    final_clip.write_videofile(
        OUTPUT_FILENAME,
        codec='libx264',
        audio_codec='aac',
        bitrate=BITRATE,
        fps=FPS
    )

    # === 刪除暫存檔案（含重試機制） ===
    print("\n清理暫存檔案中...")
    for temp_file in [VIDEO_FILENAME, AUDIO_FILENAME]:
        if not os.path.exists(temp_file):
            continue
        for attempt in range(30):  # 最多重試 10 次
            try:
                os.remove(temp_file)
                print(f"已刪除暫存檔案：{temp_file}")
                break
            except Exception as e:
                print(f"無法刪除 {temp_file}（第 {attempt + 1} 次嘗試）: {e}")
                time.sleep(0.5)
        else:
            print(f"無法刪除 {temp_file}，請手動移除。")

    # === 完成訊息與自動結束 ===
    print("\n完成！輸出檔案：", OUTPUT_FILENAME)
    print("程式將於 3 秒後自動關閉...")
    time.sleep(3)
    os._exit(0)
