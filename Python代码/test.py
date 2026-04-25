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

BAUD = 3000000
FRAME_W = 128
FRAME_H = 128
FRAME_PIXELS = FRAME_W * FRAME_H
FULL_PAYLOAD_BYTES = FRAME_PIXELS

BLOCK_SIZE = 8
BLOCK_PIXELS = BLOCK_SIZE * BLOCK_SIZE
BLOCKS_X = FRAME_W // BLOCK_SIZE
BLOCKS_Y = FRAME_H // BLOCK_SIZE
BLOCK_COUNT = BLOCKS_X * BLOCKS_Y
DELTA_MASK_BYTES = BLOCK_COUNT // 8

FRAME_TYPE_FULL = 0x01
FRAME_TYPE_DELTA = 0x02

HEADER = b'\xAA\xBB\xCC\xDD'

MAX_PACKET_BYTES = len(HEADER) + 1 + 2 + FULL_PAYLOAD_BYTES
MAX_LINK_FPS = BAUD / 10.0 / MAX_PACKET_BYTES
src_fps = cap.get(cv2.CAP_PROP_FPS)
if src_fps is None or src_fps <= 1.0:
    src_fps = 30.0
SOURCE_FRAME_INTERVAL = 1.0 / src_fps
SERIAL_BUDGET_BPS = BAUD / 10.0 * 0.96
print(f"串口全帧上限约 {MAX_LINK_FPS:.2f} FPS（RGB332），启用按字节自适应限流，源视频 {src_fps:.2f} FPS")

try:
    ser = serial.Serial('COM4', BAUD, timeout=1, write_timeout=2)
except serial.SerialException as e:
    print(f"串口打开失败: COM4, 错误: {e}")
    print("请关闭占用 COM4 的串口工具/监视器，或结束残留的 python test.py 进程后重试")
    cap.release()
    raise SystemExit(1)

next_frame_time = time.perf_counter()
last_budget_time = time.perf_counter()
budget_bytes = MAX_PACKET_BYTES
prev_rgb332 = None

def make_delta_payload(curr_flat: np.ndarray, prev_flat: np.ndarray):
    curr_blocks = curr_flat.reshape(BLOCKS_Y, BLOCK_SIZE, BLOCKS_X, BLOCK_SIZE).transpose(0, 2, 1, 3).reshape(BLOCK_COUNT, BLOCK_PIXELS)
    prev_blocks = prev_flat.reshape(BLOCKS_Y, BLOCK_SIZE, BLOCKS_X, BLOCK_SIZE).transpose(0, 2, 1, 3).reshape(BLOCK_COUNT, BLOCK_PIXELS)
    changed = np.any(curr_blocks != prev_blocks, axis=1)
    changed_indices = np.flatnonzero(changed)

    mask = bytearray(DELTA_MASK_BYTES)
    payload_parts = []
    for idx in changed_indices:
        mask[idx >> 3] |= 1 << (idx & 0x07)
        payload_parts.append(curr_blocks[idx].tobytes())

    payload = bytes(mask) + b''.join(payload_parts)
    return payload

def send_packet(packet_type: int, payload: bytes):
    global budget_bytes, last_budget_time

    payload_len = len(payload)
    if payload_len > 0xFFFF:
        raise ValueError("payload too large")

    packet_bytes = len(HEADER) + 1 + 2 + payload_len

    while True:
        now = time.perf_counter()
        elapsed = now - last_budget_time
        if elapsed > 0:
            budget_bytes = min(MAX_PACKET_BYTES * 2, budget_bytes + elapsed * SERIAL_BUDGET_BPS)
            last_budget_time = now

        if budget_bytes >= packet_bytes:
            budget_bytes -= packet_bytes
            break

        need_bytes = packet_bytes - budget_bytes
        time.sleep(need_bytes / SERIAL_BUDGET_BPS)

    ser.write(HEADER)
    ser.write(bytes([packet_type, payload_len & 0xFF, (payload_len >> 8) & 0xFF]))
    if payload_len > 0:
        ser.write(payload)

while cap.isOpened():
    ret, frame = cap.read()
    if not ret: break

    # 2. 图像处理：先居中裁剪为正方形，再缩放到 128x128，确保铺满屏幕
    height, width = frame.shape[:2]
    side = min(height, width)
    y0 = (height - side) // 2
    x0 = (width - side) // 2
    frame = frame[y0:y0 + side, x0:x0 + side]
    frame = cv2.resize(frame, (FRAME_W, FRAME_H), interpolation=cv2.INTER_AREA)
    b2 = frame[:, :, 0] >> 6
    g3 = frame[:, :, 1] >> 5
    r3 = frame[:, :, 2] >> 5
    rgb332 = ((r3 << 5) | (g3 << 2) | b2).astype(np.uint8)
    curr_flat = rgb332.reshape(-1)

    # 3. 自适应发送：全帧/差分帧二选一
    if prev_rgb332 is None:
        payload = curr_flat.tobytes()
        send_packet(FRAME_TYPE_FULL, payload)
    else:
        delta_payload = make_delta_payload(curr_flat, prev_rgb332)
        if len(delta_payload) < FULL_PAYLOAD_BYTES:
            payload = delta_payload
            send_packet(FRAME_TYPE_DELTA, payload)
        else:
            payload = curr_flat.tobytes()
            send_packet(FRAME_TYPE_FULL, payload)

    prev_rgb332 = curr_flat.copy()
    
    # 按源视频节奏推进帧；链路节奏由 send_packet 的字节预算控制
    next_frame_time += SOURCE_FRAME_INTERVAL
    sleep_s = next_frame_time - time.perf_counter()
    if sleep_s > 0:
        time.sleep(sleep_s)
    else:
        next_frame_time = time.perf_counter()

cap.release()
ser.close()
print("视频发送完成")