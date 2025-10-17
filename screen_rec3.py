import mss
import numpy as np
import pyaudiowpatch as pyaudio
import wave
import threading
import time
import keyboard
import cv2
from datetime import datetime
import os
from moviepy.editor import VideoFileClip, AudioFileClip

# === 錄影參數設定 ===
FPS = 60
BITRATE = "8000k"

# === 自動命名 ===
now = datetime.now()
timestamp = now.strftime("%Y-%m-%d %H-%M-%S")
VIDEO_FILENAME = f"{timestamp}_temp.mp4"
AUDIO_FILENAME = f"{timestamp}_audio.wav"
OUTPUT_FILENAME = f"{timestamp}.mp4"

# === 音訊設定 ===
CHANNELS = 2
AUDIO_SAMPLERATE = 44100
FRAMES_PER_BUFFER = 1024

# === 全域控制旗標 ===
stop_flag = False

# === 偵測 Pause 鍵停止 ===
def stop_on_pause():
    global stop_flag
    print("按 [Pause] 鍵停止錄影...")
    while not stop_flag:
        if keyboard.is_pressed("pause"):
            stop_flag = True
            print("偵測到 Pause 鍵，停止錄影中...")
            break
        time.sleep(0.01)

# === 錄音函式（即時寫入 WAV）===
def record_audio():
    global stop_flag
    print("開始錄製系統聲音...")
    p = pyaudio.PyAudio()

    input_device_index = None
    for i in range(p.get_device_count()):
        device_info = p.get_device_info_by_index(i)
        if 'loopback' in device_info['name'].lower():
            input_device_index = i
            break

    if input_device_index is None:
        print("未找到支援 loopback 的裝置。")
        stop_flag = True
        return
    else:
        samplerate = int(device_info['defaultSampleRate'])
        print(f"使用裝置：{device_info['name']}，取樣率：{samplerate}")

    stream = p.open(format=pyaudio.paInt16,
                    channels=CHANNELS,
                    rate=samplerate,
                    input=True,
                    input_device_index=input_device_index,
                    frames_per_buffer=FRAMES_PER_BUFFER)

    with wave.open(AUDIO_FILENAME, 'wb') as wf:
        wf.setnchannels(CHANNELS)
        wf.setsampwidth(2)
        wf.setframerate(samplerate)

        while not stop_flag:
            data = stream.read(FRAMES_PER_BUFFER, exception_on_overflow=False)
            wf.writeframes(data)

    stream.stop_stream()
    stream.close()
    p.terminate()
    print(f"錄音完成: {AUDIO_FILENAME}")

# === 錄螢幕函式（即時寫入影片）===
def record_screen():
    global stop_flag
    print("開始錄影...")
    sct = mss.mss()
    monitor = sct.monitors[1]
    frame_interval = 1.0 / FPS

    # 初始化影片寫入器
    img = np.array(sct.grab(monitor))
    height, width, _ = img.shape
    fourcc = cv2.VideoWriter_fourcc(*"mp4v")
    out = cv2.VideoWriter(VIDEO_FILENAME, fourcc, FPS, (width, height))

    while not stop_flag:
        start_time = time.time()
        img = np.array(sct.grab(monitor))
        frame = img[:, :, :3]
        out.write(frame)

        elapsed = time.time() - start_time
        if elapsed < frame_interval:
            time.sleep(frame_interval - elapsed)

    out.release()
    print(f"錄影完成: {VIDEO_FILENAME}")

# === 主程式 ===
if __name__ == "__main__":
    print("錄影程式啟動中...")

    # 啟動監控 Pause 鍵
    threading.Thread(target=stop_on_pause, daemon=True).start()

    # 啟動錄影與錄音
    t_screen = threading.Thread(target=record_screen)
    t_audio = threading.Thread(target=record_audio)

    t_screen.start()
    time.sleep(0.001)  # 先啟動錄影
    t_audio.start()

    t_screen.join()
    t_audio.join()

    # ---------------- 將影片與音訊直接合併 ----------------
    print("合併影片與音訊...")
    video_clip = VideoFileClip(VIDEO_FILENAME)
    audio_clip = AudioFileClip(AUDIO_FILENAME)
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

    print(f"完成，輸出檔案: {OUTPUT_FILENAME}")
