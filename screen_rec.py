import mss
import numpy as np
import sounddevice as sd
import wave
import threading
import time
from moviepy import editor  # 新版 moviepy 用法
import keyboard  # 監控鍵盤
import cv2
from datetime import datetime

# 取得當前時間
now = datetime.now()
timestamp = now.strftime("%Y-%m-%d %H-%M-%S")

# 自動生成檔名
VIDEO_FILENAME = f"{timestamp}_temp.mp4"
AUDIO_FILENAME = f"{timestamp}_audio.wav"
OUTPUT_FILENAME = f"{timestamp}.mp4"

samplerate = 44100
channels = 2

# 全域旗標，用來結束錄影
stop_flag = False

# === 錄音函式 ===
def record_audio():
    global stop_flag
    print("開始錄音...")
    audio_data = []

    def callback(indata, frames, time_info, status):
        if stop_flag:
            raise sd.CallbackStop()
        audio_data.append(indata.copy())

    with sd.InputStream(samplerate=samplerate, channels=channels, dtype='int16', callback=callback):
        while not stop_flag:
            sd.sleep(100)

    # 合併所有音訊塊
    audio_array = np.concatenate(audio_data, axis=0)
    with wave.open(AUDIO_FILENAME, 'wb') as wf:
        wf.setnchannels(channels)
        wf.setsampwidth(2)
        wf.setframerate(samplerate)
        wf.writeframes(audio_array.tobytes())
    print("錄音完成。")

# === 錄螢幕函式 ===
def record_screen():
    global stop_flag
    print("開始錄影...")
    sct = mss.mss()
    monitor = sct.monitors[1]
    frames = []

    while not stop_flag:
        img = np.array(sct.grab(monitor))
        frame = img[:, :, :3]
        frames.append(frame)

        # 檢查按鍵連續按下
        if keyboard.is_pressed("[") and keyboard.is_pressed("]"):
            start = time.time()
            while keyboard.is_pressed("[") and keyboard.is_pressed("]"):
                if time.time() - start >= 1.0:
                    stop_flag = True
                    break
            if stop_flag:
                break

        time.sleep(0.02)  # 控制錄影頻率約 50 FPS

    print("錄影完成。")

    # 儲存影片
    height, width, _ = frames[0].shape
    fourcc = cv2.VideoWriter_fourcc(*"mp4v")
    out = cv2.VideoWriter(VIDEO_FILENAME, fourcc, 20.0, (width, height))
    for f in frames:
        out.write(f)
    out.release()
    print("影片儲存完成。")

# === 主程式 ===
if __name__ == "__main__":
    t1 = threading.Thread(target=record_audio)
    t2 = threading.Thread(target=record_screen)

    t1.start()
    t2.start()

    t1.join()
    t2.join()

    print("合併聲音與畫面中...")
    video_clip = editor.VideoFileClip(VIDEO_FILENAME)
    audio_clip = editor.AudioFileClip(AUDIO_FILENAME)
    final_clip = video_clip.set_audio(audio_clip)
    final_clip.write_videofile(OUTPUT_FILENAME, codec='libx264', audio_codec='aac')

    print("✅ 完成！輸出檔案：", OUTPUT_FILENAME)
