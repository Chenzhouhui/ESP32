import cv2
import serial
import numpy as np
import time
import os

# 1. 配置：修改为你的串口号和波特率
# 视频数据量大，波特率建议设为 2,000,000 (2M)
video_path = os.path.join(os.path.dirname(__file__), 'test.mp4') # 默认读取脚本同目录视频

cap = cv2.VideoCapture(video_path)
if not cap.isOpened():
    print(f"无法打开视频文件: {video_path}")
    raise SystemExit(1)

BAUD = 2000000
FRAME_BYTES = 128 * 128 * 2 + 4
MAX_LINK_FPS = BAUD / 10.0 / FRAME_BYTES
TARGET_FPS = min(6.0, MAX_LINK_FPS * 0.85)
FRAME_INTERVAL = 1.0 / TARGET_FPS
print(f"串口理论上限约 {MAX_LINK_FPS:.2f} FPS，当前发送 {TARGET_FPS:.2f} FPS")

try:
    ser = serial.Serial('COM4', BAUD, timeout=1, write_timeout=2)
except serial.SerialException as e:
    print(f"串口打开失败: COM4, 错误: {e}")
    print("请关闭占用 COM4 的串口工具/监视器，或结束残留的 python test.py 进程后重试")
    cap.release()
    raise SystemExit(1)

next_send_time = time.perf_counter()

while cap.isOpened():
    ret, frame = cap.read()
    if not ret: break

    # 2. 图像处理：缩放并转为 RGB565 格式
    frame = cv2.resize(frame, (128, 128))
    img_rgb = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
    
    r = (img_rgb[:,:,0] >> 3).astype(np.uint16) << 11
    g = (img_rgb[:,:,1] >> 2).astype(np.uint16) << 5
    b = (img_rgb[:,:,2] >> 3).astype(np.uint16)
    rgb565 = (r | g | b).byteswap() 

    # 3. 发送数据：增加同步头避免错位
    ser.write(b'\xAA\xBB\xCC\xDD') # 4字节同步头
    ser.write(rgb565.tobytes())   # 32768 字节像素数据
    
    # 按链路可承载速率节流，避免串口缓冲堆积导致花屏
    next_send_time += FRAME_INTERVAL
    sleep_s = next_send_time - time.perf_counter()
    if sleep_s > 0:
        time.sleep(sleep_s)
    else:
        next_send_time = time.perf_counter()

cap.release()
ser.close()
print("视频发送完成")