import mss
import numpy as np
import pyaudiowpatch as pyaudio
import wave
import threading
import time
from moviepy import editor
import keyboard
import cv2
from datetime import datetime
import os

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
channels = 2
audio_samplerate = 44100

# === 全域控制旗標 ===
stop_flag = False

# === 全域列表存放錄音與錄影資料 ===
audio_frames = []
audio_timestamps = []
video_frames = []
video_timestamps = []

# === 錄音函式（WASAPI loopback）===
def record_audio():
    global stop_flag, audio_frames, audio_timestamps, audio_samplerate
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
        return
    else:
        audio_samplerate = int(device_info['defaultSampleRate'])
        print(f"使用裝置：{device_info['name']}，取樣率：{audio_samplerate}")

    stream = p.open(format=pyaudio.paInt16,
                    channels=channels,
                    rate=audio_samplerate,
                    input=True,
                    input_device_index=input_device_index,
                    frames_per_buffer=1024)

    try:
        while not stop_flag:
            data = stream.read(1024)
            timestamp = time.time()
            audio_frames.append(data)
            audio_timestamps.append(timestamp)
    except Exception as e:
        print(f"錄音中發生錯誤: {e}")

    stream.stop_stream()
    stream.close()
    p.terminate()

    with wave.open(AUDIO_FILENAME, 'wb') as wf:
        wf.setnchannels(channels)
        wf.setsampwidth(2)
        wf.setframerate(audio_samplerate)
        wf.writeframes(b''.join(audio_frames))

    print(f"錄音完成，檔案：{AUDIO_FILENAME}")


# === 錄螢幕函式 ===
def record_screen():
    global stop_flag, video_frames, video_timestamps
    print("開始錄影...")
    sct = mss.mss()
    monitor = sct.monitors[1]
    frame_interval = 1.0 / FPS

    while not stop_flag:
        start_time = time.time()
        img = np.array(sct.grab(monitor))
        frame = img[:, :, :3]
        video_frames.append(frame)
        video_timestamps.append(start_time)

        elapsed = time.time() - start_time
        if elapsed < frame_interval:
            time.sleep(frame_interval - elapsed)

    print("錄影完成，正在儲存影片...")
    height, width, _ = video_frames[0].shape
    fourcc = cv2.VideoWriter_fourcc(*"mp4v")
    out = cv2.VideoWriter(VIDEO_FILENAME, fourcc, FPS, (width, height))
    for f in video_frames:
        out.write(f)
    out.release()
    print("影片暫存完成")


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


# === 主程式 ===
if __name__ == "__main__":
    print("錄影程式啟動中...")

    t_screen = threading.Thread(target=record_screen)
    t_audio = threading.Thread(target=record_audio)
    threading.Thread(target=stop_on_pause, daemon=True).start()

    t_screen.start()
    time.sleep(0.001)  # 螢幕錄影先啟動
    t_audio.start()

    t_screen.join()
    t_audio.join()

    # ---------------- 對齊音訊與影片 ----------------
    print("對齊音訊與影片...")
    video_start = video_timestamps[0]
    video_end = video_timestamps[-1]

    aligned_data = []
    for ts, frame in zip(audio_timestamps, audio_frames):
        if video_start <= ts <= video_end:
            aligned_data.append(frame)

    with wave.open(AUDIO_FILENAME, 'wb') as wf:
        wf.setnchannels(channels)
        wf.setsampwidth(2)
        wf.setframerate(audio_samplerate)
        wf.writeframes(b''.join(aligned_data))

    # ---------------- 合併影片與音訊 ----------------
    print("合併影片與音訊...")
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

    # ---------------- 刪除暫存檔案 ----------------
    for temp_file in [VIDEO_FILENAME, AUDIO_FILENAME]:
        if os.path.exists(temp_file):
            try:
                os.remove(temp_file)
            except:
                pass

    print(f"完成，輸出檔案：{OUTPUT_FILENAME}")
