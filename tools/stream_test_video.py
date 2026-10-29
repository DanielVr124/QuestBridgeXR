#!/usr/bin/env python3
"""Stream an H.264 test pattern, Windows desktop, or QBX compositor mirror to Quest.

Requires FFmpeg with libx264. Desktop is duplicated into both eyes; mirror mode captures
the experimental OpenXR runtime's side-by-side preview and forwards its two eye images.
Mirror mode is not a direct GPU swapchain-to-Quest transport and adds capture/encode latency.
"""
from __future__ import annotations

import argparse
import ipaddress
import os
import shutil
import socket
import struct
import subprocess
import sys
import time
from typing import Iterator, Optional

try:
    from qbx_config import DEFAULT_CONFIG_PATH, load_config
except ModuleNotFoundError:  # Allows tests that import this file by path.
    sys.path.insert(0, str(__import__("pathlib").Path(__file__).resolve().parent))
    from qbx_config import DEFAULT_CONFIG_PATH, load_config

MAGIC = 0x31584251  # 'QBX1'
PROTOCOL_VERSION = 1
TYPE_VIDEO_CONFIG = 8
TYPE_FRAME_CHUNK = 6
MAX_DATAGRAM = 1200
HEADER = struct.Struct('<IHHII')
VIDEO_CONFIG = struct.Struct('<IIIIHH')
FRAME_PREFIX = struct.Struct('<QIIHHHH')
CHUNK_BYTES = MAX_DATAGRAM - HEADER.size - FRAME_PREFIX.size
CODEC_H264 = 1
LAYOUT_MONO = 0
LAYOUT_SBS = 1


def start_codes(buf: bytearray) -> list[tuple[int, int]]:
    found: list[tuple[int, int]] = []
    i = 0
    n = len(buf)
    while i + 3 <= n:
        if i + 4 <= n and buf[i:i + 4] == b'\x00\x00\x00\x01':
            found.append((i, 4))
            i += 4
        elif buf[i:i + 3] == b'\x00\x00\x01':
            found.append((i, 3))
            i += 3
        else:
            i += 1
    return found


def is_aud(buf: bytearray, pos: int, prefix_len: int) -> bool:
    nal_pos = pos + prefix_len
    return nal_pos < len(buf) and (buf[nal_pos] & 0x1F) == 9  # access unit delimiter


def has_idr(buf: bytes) -> bool:
    data = bytearray(buf)
    return any(pos + prefix_len < len(data) and (data[pos + prefix_len] & 0x1F) == 5
               for pos, prefix_len in start_codes(data))


def access_units(stream) -> Iterator[bytes]:
    """Turn Annex-B H.264 bytes into access units using x264's AUD NAL units."""
    buf = bytearray()
    while True:
        chunk = stream.read(16384)
        eof = not chunk
        if chunk:
            buf.extend(chunk)
        codes = start_codes(buf)
        auds = [pos for pos, size in codes if is_aud(buf, pos, size)]
        if len(auds) >= 2:
            first, next_start = auds[0], auds[1]
            if first:
                del buf[:first]
                next_start -= first
            frame = bytes(buf[:next_start])
            del buf[:next_start]
            if frame:
                yield frame
            continue
        if eof:
            if auds:
                if auds[0]:
                    del buf[:auds[0]]
                if buf:
                    yield bytes(buf)
            break
        if not auds and len(buf) > 2 * 1024 * 1024:
            # Keep a little tail to handle a start code split across reads.
            del buf[:-4]


def make_packet(packet_type: int, sequence: int, payload: bytes) -> bytes:
    return HEADER.pack(MAGIC, PROTOCOL_VERSION, packet_type, sequence & 0xFFFFFFFF, len(payload)) + payload


def send_packet(sock: socket.socket, target: tuple[str, int], packet_type: int,
                sequence: int, payload: bytes) -> int:
    return sock.sendto(make_packet(packet_type, sequence, payload), target)


def parse_bitrate(value: str) -> int:
    text = value.strip().upper()
    multiplier = 1
    if text.endswith("K"):
        multiplier, text = 1_000, text[:-1]
    elif text.endswith("M"):
        multiplier, text = 1_000_000, text[:-1]
    elif text.endswith("G"):
        multiplier, text = 1_000_000_000, text[:-1]
    number = float(text)
    result = int(number * multiplier)
    if result <= 0 or result > 2_000_000_000:
        raise ValueError("bitrate must be between 1 and 2,000,000,000 bits/s")
    return result


def create_ffmpeg(args: argparse.Namespace) -> subprocess.Popen:
    eye_width = args.width // 2 if args.layout == 'sbs' else args.width
    if args.layout == 'sbs' and args.width % 2:
        raise ValueError('--width must be even for side-by-side mode')

    if args.source == 'test':
        source = f'testsrc2=size={eye_width}x{args.height}:rate={args.fps}'
        if args.layout == 'sbs':
            source += ',split=2[l][r];[l][r]hstack=inputs=2'
        cmd = [
            args.ffmpeg, '-hide_banner', '-loglevel', 'error', '-re',
            '-f', 'lavfi', '-i', source,
        ]
    elif args.source == 'mirror':
        # Capture the runtime's side-by-side eye preview window.
        if os.name != 'nt':
            raise ValueError('--source mirror requires Windows gdigrab and the QuestBridge compositor window')
        cmd = [args.ffmpeg, '-hide_banner', '-loglevel', 'error', '-f', 'gdigrab',
               '-framerate', str(args.fps), '-draw_mouse', '0', '-i', 'title=QuestBridge XR Compositor']
        if args.layout == 'sbs':
            cmd += ['-vf', f'scale={args.width}:{args.height}']
        else:
            # For a mono client, use the left-eye half of the SBS preview.
            cmd += ['-vf', f'crop=iw/2:ih:0:0,scale={args.width}:{args.height}']
    else:
        # Windows-only desktop capture. Mono duplicates the desktop image for both eyes;
        # SBS duplicates the same image into each half, so this is a 2D preview, not VR capture.
        if os.name != 'nt':
            raise ValueError('--source desktop requires Windows gdigrab')
        cmd = [
            args.ffmpeg, '-hide_banner', '-loglevel', 'error',
            '-f', 'gdigrab', '-framerate', str(args.fps), '-draw_mouse', '1' if getattr(args, 'draw_mouse', True) else '0', '-i', 'desktop',
        ]
        if args.layout == 'sbs':
            graph = (f'[0:v]scale={eye_width}:{args.height},split=2[left][right];'
                     f'[left][right]hstack=inputs=2[v]')
            cmd += ['-filter_complex', graph, '-map', '[v]']
        else:
            cmd += ['-vf', f'scale={args.width}:{args.height}']

    duration_seconds = getattr(args, 'duration_seconds', None)
    if duration_seconds is not None:
        if not 0.1 <= float(duration_seconds) <= 3600.0:
            raise ValueError('--duration-seconds must be in 0.1..3600 seconds')
        cmd += ['-t', str(float(duration_seconds))]

    bitrate_bps = parse_bitrate(str(args.bitrate))
    rate_control = str(getattr(args, 'rate_control', 'vbr'))
    maxrate_factor = float(getattr(args, 'maxrate_factor', 1.5))
    vbv_buffer_ms = int(getattr(args, 'vbv_buffer_ms', 100))
    vbv_buffer_bps = max(bitrate_bps // 10, int(bitrate_bps * vbv_buffer_ms / 1000))
    maxrate_bps = bitrate_bps if rate_control == 'cbr' else int(bitrate_bps * maxrate_factor)
    cmd += [
        '-an', '-c:v', args.encoder, '-pix_fmt', 'yuv420p', '-b:v', str(bitrate_bps),
        '-maxrate', str(maxrate_bps), '-bufsize', str(vbv_buffer_bps),
        '-g', str(max(1, round(args.fps * float(getattr(args, 'gop_seconds', 1.0))))),
    ]
    # The packetizer is currently H.264 Annex-B only. These low-latency controls
    # apply to libx264; fail clearly for unsupported encoder choices below.
    if args.encoder != 'libx264':
        raise ValueError('this test packetizer currently requires --encoder libx264')
    tune = str(getattr(args, 'tune', 'zerolatency'))
    h264_profile = str(getattr(args, 'h264_profile', 'high'))
    threads = int(getattr(args, 'threads', 0))
    x264_params = 'aud=1:repeat-headers=1:scenecut=0'
    if rate_control == 'cbr':
        x264_params += ':nal-hrd=cbr:force-cfr=1'
        cmd += ['-minrate', str(bitrate_bps)]
    cmd += [
        '-profile:v', h264_profile,
        '-preset', str(getattr(args, 'preset', 'ultrafast')), '-tune', tune,
        '-threads', str(threads),
        '-keyint_min', str(max(1, round(args.fps * float(getattr(args, 'gop_seconds', 1.0))))),
        '-x264-params', x264_params,
        '-f', 'h264', 'pipe:1'
    ]
    print('Starting FFmpeg:', ' '.join(cmd), flush=True)
    return subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=None, bufsize=0)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--config', default=str(DEFAULT_CONFIG_PATH),
                        help='JSON settings file (defaults to config/questbridge.json)')
    parser.add_argument('--quest-ip', help='Quest headset Wi-Fi IPv4 address (overrides config)')
    parser.add_argument('--source', choices=('test', 'desktop', 'mirror'), help='test pattern, Windows desktop, or QuestBridge compositor stereo mirror (overrides config)')
    parser.add_argument('--port', type=int, help='Quest UDP receive port (overrides config)')
    parser.add_argument('--width', type=int, help='encoded frame width (overrides config)')
    parser.add_argument('--height', type=int, help='encoded frame height (overrides config)')
    parser.add_argument('--fps', type=int, help='frame rate (overrides config)')
    parser.add_argument('--bitrate', help='encoder bitrate, e.g. 8000000 or 8M (overrides config)')
    parser.add_argument('--layout', choices=('mono', 'sbs'), help='mono duplicates image per eye; sbs splits horizontally')
    parser.add_argument('--encoder', help='FFmpeg encoder; only libx264 is supported by this packetizer')
    parser.add_argument('--preset', choices=('ultrafast', 'superfast', 'veryfast', 'faster', 'fast', 'medium'),
                        help='libx264 speed/quality preset (overrides config)')
    parser.add_argument('--tune', choices=('zerolatency', 'fastdecode', 'film', 'animation', 'grain', 'stillimage'),
                        help='libx264 tune (overrides config)')
    parser.add_argument('--h264-profile', choices=('baseline', 'main', 'high'), help='H.264 compatibility/feature profile')
    parser.add_argument('--rate-control', choices=('cbr', 'vbr'), help='CBR holds rate more tightly; VBR allows bursts for complex frames')
    parser.add_argument('--maxrate-factor', type=float, help='VBR peak bitrate multiplier, 1.0–3.0')
    parser.add_argument('--vbv-buffer-ms', type=int, help='Encoder rate-control buffer duration in milliseconds')
    parser.add_argument('--threads', type=int, help='Encoder worker threads; 0 lets FFmpeg choose')
    parser.add_argument('--udp-send-buffer-kib', type=int, help='OS UDP send buffer size in KiB')
    parser.add_argument('--gop-seconds', type=float, help='keyframe interval in seconds (overrides config)')
    parser.add_argument('--duration-seconds', type=float, help='stop the test stream after this many seconds (0.1–3600); omitted means run until stopped')
    mouse = parser.add_mutually_exclusive_group()
    mouse.add_argument('--draw-mouse', dest='draw_mouse', action='store_true', help='capture mouse cursor')
    mouse.add_argument('--no-draw-mouse', dest='draw_mouse', action='store_false', help='do not capture mouse cursor')
    parser.set_defaults(draw_mouse=None)
    parser.add_argument('--config-repeat-seconds', type=float, help='repeat VIDEO_CONFIG interval (overrides config)')
    parser.add_argument('--ffmpeg', default='ffmpeg', help='FFmpeg executable')
    args = parser.parse_args()

    try:
        cfg = load_config(args.config)
    except (OSError, ValueError) as exc:
        print(f'Configuration error: {exc}', file=sys.stderr)
        return 2

    net = cfg['network']
    stream = cfg['stream']
    args.quest_ip = args.quest_ip or net['quest_ip']
    args.port = args.port or net['quest_local_port']
    args.source = args.source or stream['source']
    args.width = args.width or stream['width']
    args.height = args.height or stream['height']
    args.fps = args.fps or stream['fps']
    args.bitrate = args.bitrate or str(stream['bitrate_bps'])
    args.layout = args.layout or stream['layout']
    args.encoder = args.encoder or stream['encoder']
    args.preset = args.preset or stream['preset']
    args.tune = args.tune or stream['tune']
    args.h264_profile = args.h264_profile or stream['h264_profile']
    args.rate_control = args.rate_control or stream['rate_control']
    args.maxrate_factor = args.maxrate_factor if args.maxrate_factor is not None else stream['maxrate_factor']
    args.vbv_buffer_ms = args.vbv_buffer_ms if args.vbv_buffer_ms is not None else stream['vbv_buffer_ms']
    args.threads = args.threads if args.threads is not None else stream['threads']
    args.udp_send_buffer_kib = args.udp_send_buffer_kib if args.udp_send_buffer_kib is not None else stream['udp_send_buffer_kib']
    args.gop_seconds = args.gop_seconds if args.gop_seconds is not None else stream['gop_seconds']
    args.draw_mouse = args.draw_mouse if args.draw_mouse is not None else stream['draw_mouse']
    args.config_repeat_seconds = args.config_repeat_seconds if args.config_repeat_seconds is not None else stream['config_repeat_seconds']

    if not shutil.which(args.ffmpeg) and not (args.ffmpeg and os.path.isfile(args.ffmpeg)):
        print('FFmpeg was not found. Install FFmpeg with libx264 and make it available on PATH.', file=sys.stderr)
        return 2
    if not (64 <= args.width <= 4096 and 64 <= args.height <= 4096 and 1 <= args.fps <= 240):
        parser.error('width/height/fps are outside supported bounds')
    if args.port < 1 or args.port > 65535:
        parser.error('--port must be in 1..65535')
    if args.layout == 'sbs' and args.width % 2:
        parser.error('--width must be even for side-by-side mode')
    if args.source == 'desktop' and os.name != 'nt':
        parser.error('--source desktop is only supported on Windows (FFmpeg gdigrab)')
    if args.gop_seconds <= 0 or args.gop_seconds > 10:
        parser.error('--gop-seconds must be > 0 and <= 10')
    if args.duration_seconds is not None and not 0.1 <= args.duration_seconds <= 3600:
        parser.error('--duration-seconds must be in 0.1..3600')
    if args.config_repeat_seconds <= 0 or args.config_repeat_seconds > 30:
        parser.error('--config-repeat-seconds must be > 0 and <= 30')
    if args.rate_control not in ('cbr', 'vbr'):
        parser.error('--rate-control must be cbr or vbr')
    if not 1.0 <= args.maxrate_factor <= 3.0:
        parser.error('--maxrate-factor must be in 1.0..3.0')
    if not 10 <= args.vbv_buffer_ms <= 2000:
        parser.error('--vbv-buffer-ms must be in 10..2000')
    if not 0 <= args.threads <= 64:
        parser.error('--threads must be in 0..64')
    if not 64 <= args.udp_send_buffer_kib <= 16384:
        parser.error('--udp-send-buffer-kib must be in 64..16384')

    try:
        ipaddress.IPv4Address(args.quest_ip)
    except ipaddress.AddressValueError:
        parser.error('--quest-ip must be an IPv4 address')
    target = (args.quest_ip, args.port)
    layout = LAYOUT_SBS if args.layout == 'sbs' else LAYOUT_MONO
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_SNDBUF, args.udp_send_buffer_kib * 1024)
    sequence = 1
    frame_id = 1
    try:
        bitrate_bps = parse_bitrate(args.bitrate)
    except ValueError as exc:
        parser.error(str(exc))
    config_payload = VIDEO_CONFIG.pack(args.width, args.height, args.fps, bitrate_bps, CODEC_H264, layout)
    for _ in range(3):
        send_packet(sock, target, TYPE_VIDEO_CONFIG, sequence, config_payload)
        sequence += 1
        time.sleep(0.06)
    label = 'Windows desktop preview' if args.source == 'desktop' else 'test pattern'
    print(f'Sending {label} to {target[0]}:{target[1]} ({args.width}x{args.height} @ {args.fps}, layout={args.layout}, '
          f'bitrate={args.bitrate}, {args.rate_control}, preset={args.preset}, tune={args.tune}, '
          f'UDP send buffer={args.udp_send_buffer_kib} KiB)', flush=True)

    process: Optional[subprocess.Popen] = None
    sent_bytes = 0
    sent_frames = 0
    last_report = time.monotonic()
    last_config = last_report
    try:
        process = create_ffmpeg(args)
        assert process.stdout is not None
        for encoded in access_units(process.stdout):
            if not encoded:
                continue
            count = (len(encoded) + CHUNK_BYTES - 1) // CHUNK_BYTES
            max_frame_bytes = int(cfg['transport']['max_frame_mib']) * 1024 * 1024
            if len(encoded) > max_frame_bytes or count > 32768:
                print(f'Dropping oversized encoded frame ({len(encoded)} bytes; limit {max_frame_bytes})', file=sys.stderr)
                continue
            pts_ns = time.monotonic_ns()
            flags = 1 if has_idr(encoded) else 0
            for index in range(count):
                part = encoded[index * CHUNK_BYTES:(index + 1) * CHUNK_BYTES]
                prefix = FRAME_PREFIX.pack(pts_ns, frame_id, len(encoded), index, count, CODEC_H264, flags)
                send_packet(sock, target, TYPE_FRAME_CHUNK, sequence, prefix + part)
                sequence += 1
                sent_bytes += len(part)
            sent_frames += 1
            now = time.monotonic()
            if now - last_config >= args.config_repeat_seconds:
                send_packet(sock, target, TYPE_VIDEO_CONFIG, sequence, config_payload)
                sequence += 1
                last_config = now
            if now - last_report >= 1.0:
                print(f'frames={sent_frames}  last={len(encoded)} bytes  rate≈{sent_bytes * 8 / max(now - last_report, 0.001) / 1e6:.1f} Mb/s', flush=True)
                sent_bytes = 0
                last_report = now
            frame_id = (frame_id + 1) & 0xFFFFFFFF
        code = process.wait()
        if code:
            print(f'FFmpeg exited with code {code}. Check whether encoder "{args.encoder}" is available.', file=sys.stderr)
            return code
    except KeyboardInterrupt:
        print('\nStopping test stream…', flush=True)
    except (OSError, ValueError) as exc:
        print(f'Video sender error: {exc}', file=sys.stderr)
        return 1
    finally:
        if process and process.poll() is None:
            process.terminate()
            try:
                process.wait(timeout=2)
            except subprocess.TimeoutExpired:
                process.kill()
        sock.close()
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
