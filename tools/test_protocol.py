#!/usr/bin/env python3
"""Small standard-library tests for the QBX UDP wire format and H.264 packetizer."""
import io
import argparse
import importlib.util
import pathlib
import struct
import unittest
from unittest import mock

ROOT = pathlib.Path(__file__).resolve().parents[1]
HEADER = struct.Struct('<IHHII')
HELLO = struct.Struct('<II32s')
HELLO_ACK = struct.Struct('<IIHH')
TRACKING = struct.Struct('<Q3f4f3f3fI')
CONTROLLER = struct.Struct('<QBBHIff2f3f4f')
FRAME_PREFIX = struct.Struct('<QIIHHHH')
VIDEO_CONFIG = struct.Struct('<IIIIHH')

spec = importlib.util.spec_from_file_location('stream_test_video', ROOT / 'tools' / 'stream_test_video.py')
streamer = importlib.util.module_from_spec(spec)
assert spec and spec.loader
spec.loader.exec_module(streamer)


class ProtocolTests(unittest.TestCase):
    def test_struct_sizes_match_cpp_layout(self):
        self.assertEqual(HEADER.size, 16)
        self.assertEqual(HELLO.size, 40)
        self.assertEqual(HELLO_ACK.size, 12)
        self.assertEqual(TRACKING.size, 64)
        self.assertEqual(CONTROLLER.size, 60)
        self.assertEqual(FRAME_PREFIX.size, 24)
        self.assertEqual(VIDEO_CONFIG.size, 20)

    def test_packet_header_and_mtu(self):
        payload = VIDEO_CONFIG.pack(1920, 1080, 72, 8_000_000, 1, 1)
        packet = streamer.make_packet(streamer.TYPE_VIDEO_CONFIG, 123, payload)
        magic, version, kind, sequence, length = HEADER.unpack_from(packet)
        self.assertEqual((magic, version, kind, sequence, length),
                         (streamer.MAGIC, streamer.PROTOCOL_VERSION, streamer.TYPE_VIDEO_CONFIG, 123, VIDEO_CONFIG.size))
        self.assertEqual(len(packet), HEADER.size + VIDEO_CONFIG.size)
        self.assertLessEqual(HEADER.size + FRAME_PREFIX.size + streamer.CHUNK_BYTES, streamer.MAX_DATAGRAM)

    def test_access_unit_split_by_aud(self):
        # AUD + SPS + IDR, then AUD + P-frame. NAL payloads are intentionally tiny test fixtures.
        sample = (
            b'\x00\x00\x00\x01\x09\xf0'
            b'\x00\x00\x00\x01\x67\x42\x00\x1e'
            b'\x00\x00\x00\x01\x65\x88\x84'
            b'\x00\x00\x00\x01\x09\xf0'
            b'\x00\x00\x00\x01\x41\x9a\x22'
        )
        frames = list(streamer.access_units(io.BytesIO(sample)))
        self.assertEqual(len(frames), 2)
        self.assertTrue(streamer.has_idr(frames[0]))
        self.assertFalse(streamer.has_idr(frames[1]))
        self.assertTrue(frames[0].startswith(b'\x00\x00\x00\x01\x09'))
        self.assertTrue(frames[1].startswith(b'\x00\x00\x00\x01\x09'))

    def test_chunk_count_and_chunk_payload_limit(self):
        data = bytes(300_000)
        chunk_count = (len(data) + streamer.CHUNK_BYTES - 1) // streamer.CHUNK_BYTES
        self.assertGreater(chunk_count, 1)
        self.assertLessEqual(chunk_count, 16384)
        for index in range(chunk_count):
            part = data[index * streamer.CHUNK_BYTES:(index + 1) * streamer.CHUNK_BYTES]
            prefix = FRAME_PREFIX.pack(1, 8, len(data), index, chunk_count, 1, 0)
            packet = streamer.make_packet(streamer.TYPE_FRAME_CHUNK, index, prefix + part)
            self.assertLessEqual(len(packet), streamer.MAX_DATAGRAM)

    def test_desktop_capture_command_is_mono_preview(self):
        args = argparse.Namespace(source='desktop', width=1280, height=720, layout='mono', fps=60,
                                  ffmpeg='ffmpeg', encoder='libx264', bitrate='5M')
        with mock.patch.object(streamer.os, 'name', 'nt'), mock.patch.object(streamer.subprocess, 'Popen', return_value=object()) as popen:
            streamer.create_ffmpeg(args)
        command = popen.call_args.args[0]
        self.assertEqual(command[command.index('-f') + 1], 'gdigrab')
        self.assertIn('desktop', command)
        self.assertIn('scale=1280:720', command)
        self.assertNotIn('-filter_complex', command)

    def test_finite_test_stream_duration_is_passed_to_ffmpeg(self):
        args = argparse.Namespace(source="test", layout="mono", width=1280, height=720, fps=60,
                                  ffmpeg="ffmpeg", bitrate="5M", encoder="libx264", preset="ultrafast",
                                  tune="zerolatency", h264_profile="high", rate_control="vbr",
                                  maxrate_factor=1.5, vbv_buffer_ms=100, threads=0, gop_seconds=1.0,
                                  duration_seconds=1.5)
        with mock.patch.object(streamer.subprocess, "Popen") as popen:
            popen.return_value = mock.Mock()
            streamer.create_ffmpeg(args)
            command = popen.call_args.args[0]
        self.assertIn("-t", command)
        self.assertEqual(command[command.index("-t") + 1], "1.5")

    def test_custom_preset_and_gop_are_used(self):
        args = argparse.Namespace(source='test', width=1280, height=720, layout='mono', fps=60,
                                  ffmpeg='ffmpeg', encoder='libx264', bitrate='5M',
                                  preset='veryfast', gop_seconds=0.5, draw_mouse=False)
        with mock.patch.object(streamer.subprocess, 'Popen', return_value=object()) as popen:
            streamer.create_ffmpeg(args)
        command = popen.call_args.args[0]
        self.assertEqual(command[command.index('-g') + 1], '30')
        self.assertEqual(command[command.index('-keyint_min') + 1], '30')
        self.assertEqual(command[command.index('-preset') + 1], 'veryfast')

    def test_cbr_rate_control_and_custom_profile_are_used(self):
        args = argparse.Namespace(source='test', width=1280, height=720, layout='mono', fps=90,
                                  ffmpeg='ffmpeg', encoder='libx264', bitrate='8M', preset='ultrafast',
                                  tune='zerolatency', h264_profile='main', rate_control='cbr',
                                  maxrate_factor=1.0, vbv_buffer_ms=40, threads=4, gop_seconds=0.5)
        with mock.patch.object(streamer.subprocess, 'Popen', return_value=object()) as popen:
            streamer.create_ffmpeg(args)
        command = popen.call_args.args[0]
        self.assertEqual(command[command.index('-profile:v') + 1], 'main')
        self.assertEqual(command[command.index('-threads') + 1], '4')
        self.assertEqual(command[command.index('-minrate') + 1], '8000000')
        self.assertEqual(command[command.index('-maxrate') + 1], '8000000')
        self.assertIn('nal-hrd=cbr:force-cfr=1', command[command.index('-x264-params') + 1])

    def test_vbr_peak_rate_respects_configured_factor(self):
        args = argparse.Namespace(source='test', width=1280, height=720, layout='mono', fps=60,
                                  ffmpeg='ffmpeg', encoder='libx264', bitrate='5M', preset='ultrafast',
                                  rate_control='vbr', maxrate_factor=2.0, vbv_buffer_ms=100,
                                  h264_profile='high', threads=0, gop_seconds=1.0)
        with mock.patch.object(streamer.subprocess, 'Popen', return_value=object()) as popen:
            streamer.create_ffmpeg(args)
        command = popen.call_args.args[0]
        self.assertEqual(command[command.index('-maxrate') + 1], '10000000')
        self.assertNotIn('-minrate', command)

    def test_desktop_capture_side_by_side_duplicates_the_image(self):
        args = argparse.Namespace(source='desktop', width=1920, height=1080, layout='sbs', fps=72,
                                  ffmpeg='ffmpeg', encoder='libx264', bitrate='8M')
        with mock.patch.object(streamer.os, 'name', 'nt'), mock.patch.object(streamer.subprocess, 'Popen', return_value=object()) as popen:
            streamer.create_ffmpeg(args)
        command = popen.call_args.args[0]
        graph = command[command.index('-filter_complex') + 1]
        self.assertIn('scale=960:1080', graph)
        self.assertIn('split=2[left][right]', graph)
        self.assertIn('hstack=inputs=2[v]', graph)

    def test_compositor_mirror_capture_uses_sbs_window_without_duplication(self):
        args = argparse.Namespace(source='mirror', width=1280, height=720, layout='sbs', fps=72,
                                  ffmpeg='ffmpeg', encoder='libx264', bitrate='5M')
        with mock.patch.object(streamer.os, 'name', 'nt'), mock.patch.object(streamer.subprocess, 'Popen', return_value=object()) as popen:
            streamer.create_ffmpeg(args)
        command = popen.call_args.args[0]
        self.assertEqual(command[command.index('-i') + 1], 'title=QuestBridge XR Compositor')
        self.assertIn('scale=1280:720', command)
        self.assertNotIn('-filter_complex', command)

    def test_compositor_mirror_refuses_non_windows(self):
        args = argparse.Namespace(source='mirror', width=1280, height=720, layout='sbs', fps=72,
                                  ffmpeg='ffmpeg', encoder='libx264', bitrate='5M')
        with mock.patch.object(streamer.os, 'name', 'posix'):
            with self.assertRaisesRegex(ValueError, 'requires Windows gdigrab'):
                streamer.create_ffmpeg(args)


if __name__ == '__main__':
    unittest.main(verbosity=2)
