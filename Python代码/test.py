import cv2
import serial
import time
import os

# 1. 配置：修改为你的串口号和波特率
# 视频数据量大，波特率建议设为 2,000,000 (2M)
video_path = os.path.join(os.path.dirname(__file__), 'test.mp4') # 默认读取脚本同目录视频

cap = cv2.VideoCapture(video_path)
if not cap.isOpened():
    print(f"无法打开视频文件: {video_path}")
    raise SystemExit(1)

BAUD = 3000000
FRAME_BYTES = 128 * 128 + 4
MAX_LINK_FPS = BAUD / 10.0 / FRAME_BYTES
src_fps = cap.get(cv2.CAP_PROP_FPS)
if src_fps is None or src_fps <= 1.0:
    src_fps = 30.0
TARGET_FPS = min(src_fps, MAX_LINK_FPS * 0.96)
FRAME_INTERVAL = 1.0 / TARGET_FPS
print(f"串口理论上限约 {MAX_LINK_FPS:.2f} FPS（RGB332），源视频 {src_fps:.2f} FPS，当前发送 {TARGET_FPS:.2f} FPS")

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

    # 2. 图像处理：先居中裁剪为正方形，再缩放到 128x128，确保铺满屏幕
    height, width = frame.shape[:2]
    side = min(height, width)
    y0 = (height - side) // 2
    x0 = (width - side) // 2
    frame = frame[y0:y0 + side, x0:x0 + side]
    frame = cv2.resize(frame, (128, 128), interpolation=cv2.INTER_AREA)
    b2 = frame[:, :, 0] >> 6
    g3 = frame[:, :, 1] >> 5
    r3 = frame[:, :, 2] >> 5
    rgb332 = (r3 << 5) | (g3 << 2) | b2

    # 3. 发送数据：增加同步头避免错位
    ser.write(b'\xAA\xBB\xCC\xDD') # 4字节同步头
    ser.write(rgb332.tobytes())   # 16384 字节像素数据
    
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