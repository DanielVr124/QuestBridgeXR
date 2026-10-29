#!/usr/bin/env python3
"""QuestBridge Studio: customize profiles, export Quest settings, and launch dev tools."""
from __future__ import annotations

from qbx_paths import project_root
from openxr_runtime import discover_runtimes, launch_with_runtime, resolve_runtime_manifest
from network_setup import discover_pc_ipv4, discover_quest_ipv4

import copy
import json
import os
import pathlib
import shutil
import subprocess
import sys
import tkinter as tk
import webbrowser
from tkinter import filedialog, messagebox, simpledialog, ttk
from tkinter.scrolledtext import ScrolledText
from typing import Any

ROOT = project_root(__file__)
TOOLS = ROOT / "tools"
GENERATED = ROOT / ".generated"

# Hover text is deliberately specific: each control explains its effect, trade-offs,
# supported range, and whether it changes the current streaming/client path.
SETTING_HELP = {
    "network.pc_ip": "IPv4 address of the Windows PC on the same LAN as the Quest. The Quest client sends tracking here. Use ipconfig to find the active Wi-Fi/Ethernet IPv4 address; a VPN address is usually not correct.",
    "network.quest_ip": "IPv4 address of the Quest on Wi-Fi. The Windows sender transmits video datagrams to this address. It can change when the Quest reconnects to Wi-Fi.",
    "network.host_port": "UDP port used by the Windows host for the connection handshake, tracking, and controller data. The Quest INI and Windows host must agree. Keep it on a trusted private network.",
    "network.quest_local_port": "UDP port bound by the Quest client for incoming video and control packets. This must match the sender's destination port and should not already be used by another Quest app.",
    "network.telemetry_port": "Local-only UDP port where the Windows host mirrors validated tracking/controller packets for diagnostics. Use 0 to disable. It is sent to 127.0.0.1, not to other machines.",
    "stream.source": "Test creates a repeatable generated pattern. Desktop captures the Windows desktop and duplicates it to both eyes. Mirror captures the experimental runtime's side-by-side compositor window through FFmpeg; this is a capture/encode bridge, not zero-copy GPU streaming or timewarp.",
    "stream.layout": "Mono sends one image and duplicates it for both eyes. SBS expects left and right eye images packed side-by-side in one frame. Choose mono for the desktop preview and SBS only when the sender produces the intended two-eye layout.",
    "stream.width": "Total encoded frame width in pixels. In SBS mode this includes both eyes, so each eye receives half this width. Higher resolution preserves detail but raises encoding, network, decode, and GPU costs.",
    "stream.height": "Encoded frame height in pixels. Higher values preserve vertical detail but increase bitrate and decoder workload. Start at 720p when diagnosing network or thermal issues.",
    "stream.fps": "Target video frame rate. Higher FPS can reduce motion judder, but demands more encoder time, network throughput, and Quest decoding. This controls the sender rate; the Quest runtime still schedules headset display independently.",
    "stream.bitrate_mbps": "Target H.264 bitrate in megabits per second. Raising it reduces compression artifacts on detailed or fast-moving scenes, but needs more Wi-Fi capacity. If frames stall, lower bitrate before increasing resolution.",
    "stream.preset": "libx264 speed/efficiency trade-off. Ultrafast uses least CPU but typically needs more bitrate for the same quality; slower presets compress better but may add load and latency. Start with ultrafast for bring-up.",
    "stream.tune": "libx264 content/latency tuning. zerolatency disables encoder buffering features for lower latency; fastdecode reduces decoder work; film/animation/grain/stillimage tune encoding for particular content. Non-zerolatency tunes may add buffering or be less suitable for interactive VR.",
    "stream.h264_profile": "H.264 feature/compatibility profile. Baseline is most conservative; Main is a middle ground; High can compress more efficiently but may be unsupported by some decoder combinations. Try Main if a particular Quest decoder rejects High.",
    "stream.rate_control": "CBR attempts to hold the configured bitrate more tightly; VBR can use bursts up to the peak limit for complex frames. VBR often looks better at a given average rate, while CBR is more predictable for constrained links.",
    "stream.maxrate_factor": "In VBR mode, peak bitrate = target bitrate × this factor. Higher values can improve complex frames but create larger network bursts. CBR ignores this factor and keeps peak rate near target.",
    "stream.vbv_buffer_ms": "Approximate rate-control buffer duration. Smaller values constrain encoder buffering and bursts, often reducing latency at the expense of quality stability; larger values can smooth bitrate but tolerate/produce longer bursts.",
    "stream.threads": "FFmpeg encoder worker-thread count. 0 lets FFmpeg choose automatically. Fewer threads can reduce contention but may limit throughput; more threads can improve throughput while using more CPU and scheduling resources.",
    "stream.udp_send_buffer_kib": "Operating-system UDP send queue size for video, in KiB. A larger queue can absorb brief bursts but can also allow stale video to queue up. If latency grows under congestion, try a smaller buffer; OS limits may clamp the request.",
    "stream.gop_seconds": "Interval between H.264 keyframes. Shorter intervals recover sooner after packet loss or a late join but increase bitrate. Around 0.5–1.0 seconds is a reasonable test range.",
    "stream.config_repeat_seconds": "How often the sender repeats stream dimensions/codec/layout metadata. Repetition helps a running Quest client recover after a sender starts or restarts; it is a small amount of extra UDP traffic.",
    "stream.draw_mouse": "When desktop capture is selected, controls whether FFmpeg includes the Windows mouse cursor in the preview. It has no effect on generated test patterns.",
    "tracking.enabled": "Sends headset position and orientation telemetry to the Windows host. Disable this only for network diagnostics; it does not turn off tracking inside the Quest's own OpenXR runtime.",
    "tracking.controllers_enabled": "Sends left/right Touch controller pose, trigger/grip, thumbstick, and supported button states to the host. It does not implement haptics or a full Windows OpenXR input system by itself.",
    "tracking.rate_hz": "Maximum outgoing tracking/controller packet rate. Higher rates make telemetry fresher but add more packet traffic and CPU work. Actual pose acquisition/rendering cadence remains controlled by the Quest runtime.",
    "tracking.position_scale": "Multiplies reported HMD/controller positions. 1.0 preserves measured metres. This is a diagnostic coordinate scale, not a physical IPD or world-scale setting for Windows VR applications.",
    "tracking.thumbstick_deadzone": "Magnitude near the thumbstick center treated as zero, then rescaled to the full range. Raise slightly to hide stick drift; too high makes small movements less responsive.",
    "tracking.invert_thumbstick_x": "Negates left/right thumbstick values before sending them to the host. Useful for custom input adapters that expect the opposite horizontal sign.",
    "tracking.invert_thumbstick_y": "Negates up/down thumbstick values before sending them to the host. Useful when an input consumer uses the opposite vertical convention.",
    "tracking.swap_face_buttons": "Swaps each controller's primary/secondary face-button states in the telemetry packet. On Touch controllers this exchanges X/Y on the left and A/B on the right; it does not change button labels inside Quest menus.",
    "tracking.swap_hands": "Swaps the left/right hand identity in outgoing controller packets. Use only when an external input consumer has the hands reversed; physical controller locations themselves are not mirrored.",
    "tracking.controller_pose": "Grip is the pose centered around how the controller is held. Aim is the pointing pose intended for ray/weapon aiming. The selected OpenXR binding must be supported by the active Quest interaction profile.",
    "tracking.reference_space": "LOCAL is a room-independent origin near the start position; this project defaults the virtual floor offset to -1.60 m for a head-height-like LOCAL origin. STAGE attempts to use the floor/room-scale reference space and falls back to LOCAL if unsupported; when using STAGE, start by setting the room floor offset near 0.0 m because STAGE is normally floor referenced.",
    "tracking.trigger_deadzone": "Trigger values at or below this fraction are treated as zero; the remaining range is scaled back to 0–1. Increase slightly to reduce analog trigger noise, but large values reduce fine trigger control.",
    "tracking.trigger_curve": "Applies a power curve to the normalized trigger value. 1.0 is linear; values below 1.0 make the trigger ramp up faster; values above 1.0 provide finer control near the start of the pull.",
    "tracking.offset_x": "Adds a fixed metre offset to outgoing HMD and controller X positions after position scaling. This is a telemetry calibration setting, not a change to the Quest tracking origin itself.",
    "tracking.offset_y": "Adds a fixed metre offset to outgoing HMD and controller Y positions after position scaling. Positive Y follows the OpenXR tracking coordinate convention used by the client.",
    "tracking.offset_z": "Adds a fixed metre offset to outgoing HMD and controller Z positions after position scaling. Positive/negative direction follows the OpenXR coordinate convention; use small changes and inspect telemetry.",
    "display.brightness": "Adds/subtracts a constant from decoded video RGB values. This affects the streamed image only, not the physical Quest panel or system brightness. Large adjustments clip highlights/shadows.",
    "display.contrast": "Scales RGB values around middle gray. 1.0 is neutral; higher values increase contrast but clip more detail; lower values flatten the image.",
    "display.saturation": "Scales color intensity relative to luminance. 1.0 is neutral, 0.0 is grayscale, and values above 1.0 increase saturation. This runs in the Quest video shader.",
    "display.gamma": "Applies a power-law brightness curve to decoded RGB. 1.0 is neutral. Higher values generally brighten midtones; lower values darken midtones. Extreme settings can distort colors.",
    "display.aspect_mode": "Stretch fills the entire eye view but may distort proportions. Fit preserves the full image and adds black bars when needed. Crop fills the view while cropping edges. Source aspect is computed per eye for SBS input.",
    "display.flip_horizontal": "Mirrors each eye's image horizontally without exchanging left and right eye assignments. Useful for correcting a mirrored source.",
    "display.flip_vertical": "Flips the decoded image vertically. Use only if the source appears upside down; this does not alter head tracking orientation.",
    "display.swap_eyes": "Exchanges left/right image selection. Use only when the source's eye order is reversed; mono sources look the same either way.",
    "transport.max_frame_mib": "Maximum encoded frame/access-unit size accepted by the Quest receiver. Larger values allow unusually complex keyframes but allocate more memory and can prolong frame assembly. Keep this as low as your stream permits.",
    "transport.incomplete_frame_timeout_ms": "Time allowed for all UDP chunks of an encoded frame to arrive. Lower values discard lossy/incomplete frames quickly to protect latency; higher values may recover on slow networks but can show older frames later.",
    "transport.quest_receive_buffer_kib": "Requested kernel UDP receive-buffer capacity on the Quest in KiB. More buffer can reduce packet drops during bursts but may increase queuing latency; Android/driver limits may clamp it.",
    "room.environment": "Choose from 208 generated scenery environments, including dreamcore and liminal spaces, cozy cabins and reading nooks, rainy cafes, peaceful gardens, fairy worlds, Japanese scenes, aquatic worlds, cosmic skies, pastel dreamscapes, city nights, zen retreats, and mystical forests. Scenery is generated on-device and designed to stay lightweight.",
    "room.room_size": "Sets the walkable room footprint. Small is a compact cozy gaming room, Medium gives a comfortable desk-and-couch room, and Large allows more movement. Virtual movement is bounded by the room's outer walls.",
    "room.screen_destination": "Controls where the PC video stream appears. Monitor shows it on the desk display, TV shows it on the wall TV, and Both mirrors the same stream onto both. These are mirrored copies, not independent desktop captures yet.",
    "room.monitor_count": "Number of desk monitors rendered in the room (1–3). All monitors currently mirror the same decoded PC stream; separate capture sources are not implemented.",
    "room.show_tv": "Show or hide the wall-mounted television. When it is not selected as a PC stream destination, it displays the selected environment image as ambient art.",
    "room.show_couch": "Show or hide the 3D couch with cushions. It is real geometry in the room scene, but it does not have animated cushions, physics, or automatic sitting detection.",
    "room.show_chair": "Show or hide the 3D desk chair behind the starting position. The virtual view can be lowered with the seated-view toggle, but this does not move your physical body or replace physical room safety.",
    "room.room_controls_enabled": "Enables controller shortcuts inside the virtual room: left thumbstick click cycles where the PC stream appears, right thumbstick click changes the outside scenery, and left X toggles the seated camera height. Disable this if you want the room to remain static while using controllers.",
    "room.sitting_toggle_enabled": "Allows left-controller X to toggle a seated camera offset. This lowers the virtual eye viewpoint without modifying the actual headset tracking or teleporting the player to a chair; turn it off if you do not want a button to affect view height.",
    "room.sitting_head_drop_m": "Vertical amount by which the virtual camera is lowered while seated mode is active, in metres. It is applied to the room view only; physical head tracking still adds your real movements. Use a small change at a time for comfort.",
    "room.movement_enabled": "Allows the left Touch thumbstick to move you around the virtual room and the right thumbstick to turn. Physical head tracking continues to work independently.",
    "room.movement_speed": "Virtual walking speed in metres per second. Lower values are gentler and may reduce discomfort; higher values traverse the room faster. This affects thumbstick locomotion, not actual tracked walking.",
    "room.smooth_turn": "When enabled, right-stick horizontal movement continuously turns the virtual view. When disabled, it uses discrete snap turns, which some users find more comfortable.",
    "room.turn_speed": "Angular speed for smooth turning, in radians per second at full right-stick deflection. Only used when Smooth Turn is enabled.",
    "room.snap_turn_degrees": "Angle used for each right-stick snap turn, between 15 and 60 degrees. Only used when Smooth Turn is disabled.",
    "room.comfort_vignette": "Adds a subtle darkening at the periphery of the entire rendered eye image. It may help some users tolerate virtual movement, but can reduce peripheral visibility; disable it if you dislike the effect.",
    "room.floor_offset_y": "Vertical placement of the virtual room floor in metres relative to the selected OpenXR reference space. Default -1.60 assumes a head-height-like LOCAL origin. If using STAGE (floor referenced), start near 0.0. If you see the floor at your knees or above your head, adjust in 0.05 m steps; this moves the virtual room, not your physical tracking.",
    "fake_vr.enabled": "Enables the experimental fake-VR mouse-look bridge in the Windows host. It translates Quest head rotation into normal Windows mouse deltas; it does not inject code into Roblox. The bridge must still be toggled on with F8 while an allowlisted window is foreground.",
    "fake_vr.allowed_processes": "Comma-separated executable names allowed to receive fake-VR mouse input. Defaults to RobloxPlayerBeta.exe. Use exact process names, keep this narrow, and never add unrelated system processes.",
    "fake_vr.yaw_enabled": "Use horizontal head rotation (turning left/right) to move the game's mouse-look camera. Many games that lock the mouse use this for camera turning.",
    "fake_vr.pitch_enabled": "Use vertical head rotation (looking up/down) to move the game's mouse-look camera. Some games clamp pitch or use a different sensitivity, so tune it per title.",
    "fake_vr.invert_pitch": "Reverse the up/down head-to-mouse direction. Toggle this if looking up in the headset makes the game camera look down.",
    "fake_vr.sensitivity_px_per_radian": "Mouse movement scale for each radian of head rotation. Higher values turn the camera more for the same head motion. Start around 500–1000 and adjust with the game's sensitivity settings.",
    "fake_vr.pitch_sensitivity": "Additional scale applied to vertical mouse motion relative to horizontal motion. Use a lower value if pitch feels too twitchy.",
    "fake_vr.deadzone_radians": "Tiny head-angle changes below this threshold are ignored to reduce sensor jitter. Too high makes small head movements unresponsive.",
    "fake_vr.max_delta_pixels": "Maximum injected relative mouse movement per update. Caps abrupt jumps if tracking stutters or the headset recenters. This can make very fast turns feel slower.",
}


class Tooltip:
    """Small delayed hover tooltip for Tk widgets; uses no third-party dependencies."""
    def __init__(self, widget: tk.Widget, text: str, delay_ms: int = 450) -> None:
        self.widget = widget
        self.text = text
        self.delay_ms = delay_ms
        self.after_id: str | None = None
        self.window: tk.Toplevel | None = None
        widget.bind("<Enter>", self._schedule, add="+")
        widget.bind("<Leave>", self.hide, add="+")
        widget.bind("<ButtonPress>", self.hide, add="+")

    def _schedule(self, _event: tk.Event | None = None) -> None:
        self._cancel_schedule()
        self.after_id = self.widget.after(self.delay_ms, self.show)

    def _cancel_schedule(self) -> None:
        if self.after_id:
            try:
                self.widget.after_cancel(self.after_id)
            except tk.TclError:
                pass
            self.after_id = None

    def show(self) -> None:
        self.after_id = None
        if self.window or not self.widget.winfo_exists():
            return
        x = self.widget.winfo_rootx() + min(self.widget.winfo_width(), 320) + 10
        y = self.widget.winfo_rooty() + self.widget.winfo_height() + 6
        self.window = tk.Toplevel(self.widget)
        self.window.wm_overrideredirect(True)
        self.window.wm_geometry(f"+{x}+{y}")
        label = tk.Label(self.window, text=self.text, justify="left", wraplength=380,
                         background="#fff8d8", foreground="#202020",
                         borderwidth=1, relief="solid", padx=9, pady=7,
                         font=("Segoe UI", 9))
        label.pack()

    def hide(self, _event: tk.Event | None = None) -> None:
        self._cancel_schedule()
        if self.window:
            self.window.destroy()
            self.window = None
try:
    from qbx_config import (
        DEFAULT_CONFIG, DEFAULT_CONFIG_PATH, PROFILE_DIR, apply_profile, load_config, profile_filename, profile_names,
        save_config, save_quest_ini, save_fake_vr_ini, validate_config, ROOM_ENVIRONMENTS,
        ROOM_ENVIRONMENT_LABELS, ROOM_ENVIRONMENT_BY_LABEL,
    )
except ModuleNotFoundError:
    sys.path.insert(0, str(TOOLS))
    from qbx_config import (
        DEFAULT_CONFIG, DEFAULT_CONFIG_PATH, PROFILE_DIR, apply_profile, load_config, profile_filename, profile_names,
        save_config, save_quest_ini, save_fake_vr_ini, validate_config, ROOM_ENVIRONMENTS,
        ROOM_ENVIRONMENT_LABELS, ROOM_ENVIRONMENT_BY_LABEL,
    )


class QuestBridgeStudio(tk.Tk):
    def __init__(self) -> None:
        super().__init__()
        self.title("QuestBridge Studio")
        self.geometry("1180x860")
        self.minsize(980, 720)
        self._setup_visual_theme()
        self.config_data = load_config()
        self.vars: dict[str, tk.Variable] = {}
        self.host_process: subprocess.Popen | None = None
        self.stream_process: subprocess.Popen | None = None
        self.telemetry_process: subprocess.Popen | None = None
        self._build_ui()
        self._load_form(self.config_data)
        self.protocol("WM_DELETE_WINDOW", self._on_close)

    def _setup_visual_theme(self) -> None:
        """Apply a cohesive dark, blue-violet Studio theme without changing settings behavior."""
        self.palette = {
            "bg": "#0b1020", "surface": "#121a2d", "surface2": "#18233b",
            "surface3": "#202d49", "text": "#edf2ff", "muted": "#a3b1ce",
            "accent": "#8b7cff", "accent_hover": "#a49aff", "cyan": "#64d9ff",
            "border": "#2a3857", "warning": "#ffd28a",
        }
        self.configure(background=self.palette["bg"])
        style = ttk.Style(self)
        try:
            style.theme_use("clam")
        except tk.TclError:
            pass
        p = self.palette
        style.configure(".", background=p["bg"], foreground=p["text"],
                        font=("Segoe UI", 10), bordercolor=p["border"],
                        lightcolor=p["surface"], darkcolor=p["surface"])
        style.configure("TFrame", background=p["bg"])
        style.configure("Card.TFrame", background=p["surface"], relief="flat")
        style.configure("TLabel", background=p["bg"], foreground=p["text"])
        style.configure("Muted.TLabel", background=p["bg"], foreground=p["muted"])
        style.configure("Hero.TLabel", background=p["bg"], foreground=p["text"],
                        font=("Segoe UI", 24, "bold"))
        style.configure("Eyebrow.TLabel", background=p["bg"], foreground=p["cyan"],
                        font=("Segoe UI", 9, "bold"))
        style.configure("TButton", background=p["surface3"], foreground=p["text"],
                        borderwidth=0, padding=(12, 9), focusthickness=0)
        style.map("TButton", background=[("active", "#334365"), ("pressed", p["accent"])],
                  foreground=[("disabled", "#65718b"), ("active", "#ffffff")])
        style.configure("Accent.TButton", background=p["accent"], foreground="#ffffff",
                        padding=(14, 10), font=("Segoe UI", 10, "bold"))
        style.map("Accent.TButton", background=[("active", p["accent_hover"]),
                                                ("pressed", "#7060ed")])
        style.configure("TEntry", fieldbackground=p["surface2"], foreground=p["text"],
                        insertcolor=p["text"], bordercolor=p["border"],
                        padding=7, lightcolor=p["border"], darkcolor=p["border"])
        style.configure("TCombobox", fieldbackground=p["surface2"], background=p["surface3"],
                        foreground=p["text"], arrowcolor=p["cyan"], padding=7)
        style.map("TCombobox", fieldbackground=[("readonly", p["surface2"])],
                  foreground=[("readonly", p["text"])])
        style.configure("TCheckbutton", background=p["bg"], foreground=p["text"], padding=5)
        style.map("TCheckbutton", background=[("active", p["bg"])],
                  foreground=[("active", p["cyan"])])
        style.configure("TNotebook", background=p["bg"], borderwidth=0, tabmargins=(2, 4, 2, 0))
        style.configure("TNotebook.Tab", background=p["surface"], foreground=p["muted"],
                        padding=(15, 10), borderwidth=0)
        style.map("TNotebook.Tab", background=[("selected", p["surface2"]), ("active", p["surface3"])],
                  foreground=[("selected", p["cyan"]), ("active", p["text"])])
        style.configure("TSeparator", background=p["border"])
        style.configure("Vertical.TScrollbar", background=p["surface"], troughcolor=p["bg"],
                        arrowcolor=p["muted"], bordercolor=p["bg"])

    def _build_ui(self) -> None:
        self.columnconfigure(0, weight=1)
        self.rowconfigure(2, weight=1)

        header = ttk.Frame(self, padding=(24, 20, 24, 14))
        header.grid(row=0, column=0, sticky="ew")
        header.columnconfigure(0, weight=1)
        ttk.Label(header, text="QUESTBRIDGE  /  XR CONTROL CENTER", style="Eyebrow.TLabel").grid(row=0, column=0, sticky="w")
        ttk.Label(header, text="QuestBridge Studio", style="Hero.TLabel").grid(row=1, column=0, sticky="w", pady=(3, 1))
        ttk.Label(header, text="Your space. Your stream. Your settings.", style="Muted.TLabel").grid(row=2, column=0, sticky="w")
        self.status_var = tk.StringVar(value="●  READY     Settings are saved to config/questbridge.json.")
        ttk.Label(header, textvariable=self.status_var, wraplength=930,
                  foreground=self.palette["cyan"]).grid(row=3, column=0, sticky="w", pady=(12, 0))
        tk.Frame(self, height=2, background=self.palette["accent"]).grid(
            row=0, column=0, sticky="new", padx=24, pady=(0, 0))

        top = ttk.Frame(self, padding=(24, 8, 24, 12))
        top.grid(row=1, column=0, sticky="ew", padx=16, pady=(0, 8))
        top.columnconfigure(0, weight=1)
        self.profile_var = tk.StringVar(value="Balanced")
        ttk.Label(top, text="Profile").grid(row=0, column=1, padx=(0, 6))
        self.profile_box = ttk.Combobox(top, textvariable=self.profile_var, values=profile_names(), state="readonly", width=18)
        self.profile_box.grid(row=0, column=2, padx=(0, 6))
        self.profile_box.bind("<<ComboboxSelected>>", self._on_profile_selected)
        ttk.Button(top, text="Save Profile As…", command=self._save_profile_as).grid(row=0, column=3)

        notebook = ttk.Notebook(self)
        notebook.grid(row=2, column=0, sticky="nsew", padx=24, pady=(0, 12))
        self._add_network_tab(notebook)
        self._add_video_tab(notebook)
        self._add_tracking_tab(notebook)
        self._add_display_tab(notebook)
        self._add_transport_tab(notebook)
        self._add_room_tab(notebook)
        self._add_fake_vr_tab(notebook)
        self._add_play_modes_tab(notebook)

        actions = ttk.Frame(self, padding=(24, 10, 24, 20))
        actions.grid(row=3, column=0, sticky="ew")
        actions.columnconfigure(0, weight=1)
        row1 = ttk.Frame(actions)
        row1.grid(row=0, column=0, sticky="ew")
        for col in range(6):
            row1.columnconfigure(col, weight=1)
        ttk.Button(row1, text="Save Settings", style="Accent.TButton", command=self._save_settings).grid(row=0, column=0, padx=(0, 6), sticky="ew")
        ttk.Button(row1, text="Import JSON…", command=self._import_config).grid(row=0, column=1, padx=6, sticky="ew")
        ttk.Button(row1, text="Export Quest INI…", command=self._export_quest_ini).grid(row=0, column=2, padx=6, sticky="ew")
        ttk.Button(row1, text="Install Quest INI via ADB", command=self._install_quest_ini).grid(row=0, column=3, padx=6, sticky="ew")
        ttk.Button(row1, text="Open Config Folder", command=self._open_config_folder).grid(row=0, column=4, padx=6, sticky="ew")
        reset_button = ttk.Button(row1, text="Restore Defaults", command=self._restore_defaults)
        reset_button.grid(row=0, column=5, padx=(6, 0), sticky="ew")
        Tooltip(reset_button, "Restore the built-in Balanced settings to the active config file. This does not delete or overwrite your saved named profiles.")
        row2 = ttk.Frame(actions)
        row2.grid(row=1, column=0, sticky="ew", pady=(8, 0))
        for col in range(4):
            row2.columnconfigure(col, weight=1)
        ttk.Button(row2, text="Start Windows Host", command=self._start_host).grid(row=0, column=0, padx=(0, 6), sticky="ew")
        ttk.Button(row2, text="Start Video Stream", command=self._start_stream).grid(row=0, column=1, padx=6, sticky="ew")
        ttk.Button(row2, text="Monitor Telemetry", command=self._start_telemetry_monitor).grid(row=0, column=2, padx=6, sticky="ew")
        ttk.Button(row2, text="Stop Dev Processes", command=self._stop_processes).grid(row=0, column=3, padx=(6, 0), sticky="ew")
        row3 = ttk.Frame(actions)
        row3.grid(row=2, column=0, sticky="ew", pady=(8, 0))
        row3.columnconfigure(0, weight=1)
        row3.columnconfigure(1, weight=1)
        row3.columnconfigure(2, weight=1)
        ttk.Button(row3, text="Run Setup Diagnostics", command=lambda: self._run_diagnostics(False)).grid(row=0, column=0, padx=(0, 6), sticky="ew")
        ttk.Button(row3, text="Test PC Host Connection", command=lambda: self._run_diagnostics(True)).grid(row=0, column=1, padx=6, sticky="ew")
        ttk.Button(row3, text="Quick Network Setup…", command=self._quick_network_setup).grid(row=0, column=2, padx=(6, 0), sticky="ew")

    def _quick_network_setup(self) -> None:
        """Detect PC/Quest LAN addresses and let the user selectively apply them."""
        self.status_var.set("Checking the PC network route and looking for one authorized Quest via ADB…")
        self.update_idletasks()
        pc_ip = discover_pc_ipv4()
        quest_ip, quest_note = discover_quest_ipv4()

        window = tk.Toplevel(self)
        window.title("Quick Network Setup")
        window.geometry("680x390")
        window.minsize(600, 340)
        window.transient(self)
        window.grab_set()
        body = ttk.Frame(window, padding=18)
        body.pack(fill="both", expand=True)
        ttk.Label(body, text="Find the two LAN addresses used by QuestBridge XR",
                  font=("Segoe UI", 13, "bold")).pack(anchor="w", pady=(0, 8))
        ttk.Label(
            body,
            text="The PC and Quest should be on the same private Wi-Fi/LAN. Detection is read-only until you choose Apply.",
            wraplength=620,
        ).pack(anchor="w", pady=(0, 12))
        ttk.Label(body, text=f"Windows PC IPv4: {pc_ip or 'Not detected'}",
                  font=("Consolas", 10)).pack(anchor="w", pady=3)
        ttk.Label(body, text=f"Quest Wi-Fi IPv4: {quest_ip or 'Not detected'}",
                  font=("Consolas", 10)).pack(anchor="w", pady=3)
        ttk.Label(body, text=quest_note, wraplength=620, foreground="#a3b1ce").pack(anchor="w", pady=(7, 10))
        ttk.Label(
            body,
            text="Quest IP discovery needs one authorized ADB device (usually USB debugging). If no Quest is connected right now, you can still apply the PC address and enter the Quest address later in the Connection tab.",
            wraplength=620,
        ).pack(anchor="w", pady=(0, 14))

        buttons = ttk.Frame(body)
        buttons.pack(fill="x", side="bottom")
        found_any = bool(pc_ip or quest_ip)

        def apply_addresses() -> None:
            updated = []
            if pc_ip and "network.pc_ip" in self.vars:
                self.vars["network.pc_ip"].set(pc_ip)
                updated.append(f"PC={pc_ip}")
            if quest_ip and "network.quest_ip" in self.vars:
                self.vars["network.quest_ip"].set(quest_ip)
                updated.append(f"Quest={quest_ip}")
            if not updated:
                window.destroy()
                return
            if not self._save_settings():
                return
            self.status_var.set("Quick network setup saved: " + ", ".join(updated))
            window.destroy()

        ttk.Button(
            buttons, text="Apply Found Addresses", command=apply_addresses,
            state=("normal" if found_any else "disabled"),
        ).pack(side="left")
        ttk.Button(buttons, text="Close", command=window.destroy).pack(side="right")

    def _tab(self, notebook: ttk.Notebook, title: str) -> ttk.Frame:
        outer = ttk.Frame(notebook)
        outer.columnconfigure(0, weight=1)
        outer.rowconfigure(0, weight=1)
        canvas = tk.Canvas(outer, borderwidth=0, highlightthickness=0)
        scrollbar = ttk.Scrollbar(outer, orient="vertical", command=canvas.yview)
        canvas.configure(yscrollcommand=scrollbar.set)
        canvas.grid(row=0, column=0, sticky="nsew")
        scrollbar.grid(row=0, column=1, sticky="ns")
        frame = ttk.Frame(canvas, padding=24)
        frame.columnconfigure(1, weight=1)
        window_id = canvas.create_window((0, 0), window=frame, anchor="nw")
        frame.bind("<Configure>", lambda _e, c=canvas: c.configure(scrollregion=c.bbox("all")))
        canvas.bind("<Configure>", lambda e, c=canvas, w=window_id: c.itemconfigure(w, width=e.width))
        # Mouse wheel scrolling is scoped to a tab so it does not hijack other controls.
        canvas.bind("<Enter>", lambda _e, c=canvas: c.bind_all("<MouseWheel>", lambda ev: c.yview_scroll(int(-ev.delta / 120), "units")))
        canvas.bind("<Leave>", lambda _e, c=canvas: c.unbind_all("<MouseWheel>"))
        notebook.add(outer, text=title)
        return frame

    def _entry(self, parent: ttk.Frame, key: str, label: str, default: Any, row: int, help_text: str = "") -> None:
        var = tk.StringVar(value=str(default))
        self.vars[key] = var
        explanation = SETTING_HELP.get(key) or help_text or f"{label}. The active implementation reads this setting after you save and restart the relevant component."
        label_widget = ttk.Label(parent, text=label)
        label_widget.grid(row=row, column=0, sticky="w", pady=7, padx=(0, 16))
        entry = ttk.Entry(parent, textvariable=var, width=25)
        entry.grid(row=row, column=1, sticky="ew", pady=7)
        Tooltip(label_widget, explanation)
        Tooltip(entry, explanation)
        if help_text:
            help_label = ttk.Label(parent, text=help_text, foreground="#666666", wraplength=330)
            help_label.grid(row=row, column=2, sticky="w", padx=(12, 0), pady=7)
            Tooltip(help_label, explanation)

    def _combo(self, parent: ttk.Frame, key: str, label: str, value: Any, values: tuple[str, ...], row: int, help_text: str = "", editable: bool = False) -> None:
        var = tk.StringVar(value=str(value))
        self.vars[key] = var
        explanation = SETTING_HELP.get(key) or help_text or f"{label}. Choose the option that best matches your network, headset, and PC; save and restart the active component to apply it."
        label_widget = ttk.Label(parent, text=label)
        label_widget.grid(row=row, column=0, sticky="w", pady=7, padx=(0, 16))
        combo = ttk.Combobox(parent, textvariable=var, values=values, state="normal" if editable else "readonly", width=23)
        combo.grid(row=row, column=1, sticky="w", pady=7)
        Tooltip(label_widget, explanation)
        Tooltip(combo, explanation)
        if help_text:
            help_label = ttk.Label(parent, text=help_text, foreground="#666666", wraplength=330)
            help_label.grid(row=row, column=2, sticky="w", padx=(12, 0), pady=7)
            Tooltip(help_label, explanation)

    def _check(self, parent: ttk.Frame, key: str, label: str, value: bool, row: int, help_text: str = "") -> None:
        var = tk.BooleanVar(value=bool(value))
        self.vars[key] = var
        explanation = SETTING_HELP.get(key) or help_text or f"{label}. Enable or disable this behavior. Save settings and restart the relevant component for the change to take effect."
        check = ttk.Checkbutton(parent, text=label, variable=var)
        check.grid(row=row, column=0, columnspan=2, sticky="w", pady=7)
        Tooltip(check, explanation)
        if help_text:
            help_label = ttk.Label(parent, text=help_text, foreground="#666666", wraplength=330)
            help_label.grid(row=row, column=2, sticky="w", padx=(12, 0), pady=7)
            Tooltip(help_label, explanation)

    def _add_network_tab(self, notebook: ttk.Notebook) -> None:
        f = self._tab(notebook, "Connection")
        cfg = self.config_data["network"]
        self._entry(f, "network.pc_ip", "Windows PC LAN IPv4", cfg["pc_ip"], 0, "Used in the Quest app's host_ip setting.")
        self._entry(f, "network.quest_ip", "Quest Wi-Fi IPv4", cfg["quest_ip"], 1, "Destination for video streaming from this PC.")
        self._entry(f, "network.host_port", "Windows host UDP port", cfg["host_port"], 2, "Default: 9944. Allow it through Windows Firewall on a trusted LAN.")
        self._entry(f, "network.quest_local_port", "Quest receive UDP port", cfg["quest_local_port"], 3, "The Quest client binds here for control replies and video packets.")
        self._entry(f, "network.telemetry_port", "Local telemetry feed UDP port", cfg["telemetry_port"], 4, "Windows host forwards validated tracking/controller packets to 127.0.0.1 here; 0 disables it.")
        ttk.Separator(f).grid(row=5, column=0, columnspan=3, sticky="ew", pady=16)
        ttk.Label(f, text="Tip: keep telemetry on localhost and use a trusted local network for Quest traffic. Avoid exposing the UDP ports to the public internet.", wraplength=650).grid(row=6, column=0, columnspan=3, sticky="w")

    def _add_video_tab(self, notebook: ttk.Notebook) -> None:
        f = self._tab(notebook, "Video")
        cfg = self.config_data["stream"]
        self._combo(f, "stream.source", "Video source", cfg["source"], ("test", "desktop", "mirror"), 0, "Mirror requires the experimental runtime running and the QuestBridge XR Compositor window visible; it captures the stereo mirror through FFmpeg gdigrab.")
        self._combo(f, "stream.layout", "Eye layout", cfg["layout"], ("mono", "sbs"), 1, "SBS expects a single image with left and right eye halves.")
        self._combo(f, "stream.width", "Encoded width (px)", cfg["width"], ("854", "960", "1280", "1600", "1920", "2560", "2880", "3840"), 2, "Choose a preset or type any integer from 64 to 4096; SBS widths must be even.", editable=True)
        self._combo(f, "stream.height", "Encoded height (px)", cfg["height"], ("480", "540", "720", "800", "900", "1080", "1440", "2160"), 3, "Choose a preset or type any integer from 64 to 4096.", editable=True)
        self._combo(f, "stream.fps", "Frame rate (FPS)", cfg["fps"], ("30", "45", "60", "72", "80", "90", "120", "144"), 4, "Choose a preset or type an integer from 1 to 240 FPS.", editable=True)
        self._entry(f, "stream.bitrate_mbps", "Bitrate (Mbps)", f"{cfg['bitrate_bps'] / 1_000_000:.2f}".rstrip("0").rstrip("."), 5, "Higher bitrate improves detail but uses more Wi-Fi bandwidth.")
        self._combo(f, "stream.preset", "Encoder preset", cfg["preset"], ("ultrafast", "superfast", "veryfast", "faster", "fast", "medium"), 6, "Faster presets trade compression efficiency for lower CPU cost.")
        self._combo(f, "stream.tune", "Encoder tune", cfg["tune"], ("zerolatency", "fastdecode", "film", "animation", "grain", "stillimage"), 7)
        self._combo(f, "stream.h264_profile", "H.264 profile", cfg["h264_profile"], ("baseline", "main", "high"), 8)
        self._combo(f, "stream.rate_control", "Rate control", cfg["rate_control"], ("cbr", "vbr"), 9)
        self._entry(f, "stream.maxrate_factor", "VBR peak factor (1–3)", cfg["maxrate_factor"], 10, "Target bitrate × factor defines the VBR peak; CBR ignores it.")
        self._entry(f, "stream.vbv_buffer_ms", "Encoder buffer (ms)", cfg["vbv_buffer_ms"], 11, "Shorter buffers can reduce latency but make bitrate less smooth.")
        self._entry(f, "stream.threads", "Encoder threads (0=auto)", cfg["threads"], 12, "0 lets FFmpeg pick the worker-thread count.")
        self._entry(f, "stream.udp_send_buffer_kib", "UDP send buffer (KiB)", cfg["udp_send_buffer_kib"], 13, "OS send queue; large buffers can hide bursts but also queue stale video.")
        self._entry(f, "stream.gop_seconds", "Keyframe interval (seconds)", cfg["gop_seconds"], 14, "Shorter intervals recover from packet loss sooner but use more bandwidth.")
        self._entry(f, "stream.config_repeat_seconds", "Stream config resend (seconds)", cfg["config_repeat_seconds"], 15, "Helps the client recover after a sender restart.")
        self._check(f, "stream.draw_mouse", "Capture mouse cursor in desktop mode", cfg["draw_mouse"], 16)

    def _add_tracking_tab(self, notebook: ttk.Notebook) -> None:
        f = self._tab(notebook, "Tracking & Input")
        cfg = self.config_data["tracking"]
        self._check(f, "tracking.enabled", "Send headset tracking to Windows", cfg["enabled"], 0)
        self._check(f, "tracking.controllers_enabled", "Send Touch controller poses and input", cfg["controllers_enabled"], 1)
        self._entry(f, "tracking.rate_hz", "Tracking/input send rate (Hz)", cfg["rate_hz"], 2, "Caps network updates; headset rendering still runs at the runtime's cadence.")
        self._entry(f, "tracking.position_scale", "Position scale", cfg["position_scale"], 3, "1.0 is normal; adjust if app-space movement feels too large/small.")
        self._entry(f, "tracking.thumbstick_deadzone", "Thumbstick deadzone (0–0.9)", cfg["thumbstick_deadzone"], 4, "Small values ignore center drift. Values are rescaled after the deadzone.")
        self._check(f, "tracking.invert_thumbstick_x", "Invert thumbstick X axis", cfg["invert_thumbstick_x"], 5)
        self._check(f, "tracking.invert_thumbstick_y", "Invert thumbstick Y axis", cfg["invert_thumbstick_y"], 6)
        self._combo(f, "tracking.controller_pose", "Controller pose source", cfg["controller_pose"], ("grip", "aim"), 7, "Grip is hand-held pose; aim is pointing direction.")
        self._combo(f, "tracking.reference_space", "Tracking origin", cfg["reference_space"], ("local", "stage"), 8, "Stage is used only if the Quest runtime reports support.")
        self._check(f, "tracking.swap_face_buttons", "Swap primary/secondary face-button mapping", cfg["swap_face_buttons"], 9)
        self._check(f, "tracking.swap_hands", "Swap left/right controller identity", cfg["swap_hands"], 10)
        self._entry(f, "tracking.trigger_deadzone", "Trigger deadzone (0–0.5)", cfg["trigger_deadzone"], 11)
        self._entry(f, "tracking.trigger_curve", "Trigger curve (0.25–3.0)", cfg["trigger_curve"], 12)
        self._entry(f, "tracking.offset_x", "Position offset X (m)", cfg["offset_x"], 13)
        self._entry(f, "tracking.offset_y", "Position offset Y (m)", cfg["offset_y"], 14)
        self._entry(f, "tracking.offset_z", "Position offset Z (m)", cfg["offset_z"], 15)
        ttk.Label(f, text="These settings affect the telemetry sent by this Quest client. They do not themselves create a Windows OpenXR input system or add controller haptics.", foreground="#666666", wraplength=720).grid(row=16, column=0, columnspan=3, sticky="w", pady=(18, 0))

    def _add_display_tab(self, notebook: ttk.Notebook) -> None:
        f = self._tab(notebook, "Display")
        cfg = self.config_data["display"]
        self._entry(f, "display.brightness", "Brightness adjustment (-0.5 to 0.5)", cfg["brightness"], 0, "Affects decoded video only; does not change the Quest panel itself.")
        self._entry(f, "display.contrast", "Contrast (0.25 to 2.5)", cfg["contrast"], 1, "1.0 is neutral.")
        self._entry(f, "display.saturation", "Saturation (0–2)", cfg["saturation"], 2)
        self._entry(f, "display.gamma", "Gamma (0.5–2.5)", cfg["gamma"], 3)
        self._combo(f, "display.aspect_mode", "Image aspect behavior", cfg["aspect_mode"], ("stretch", "fit", "crop"), 4)
        self._check(f, "display.flip_horizontal", "Flip each eye horizontally", cfg["flip_horizontal"], 5)
        self._check(f, "display.flip_vertical", "Flip image vertically", cfg["flip_vertical"], 6)
        self._check(f, "display.swap_eyes", "Swap left/right eye assignment", cfg["swap_eyes"], 7)
        ttk.Label(f, text="Color/aspect/flip controls are applied in the Quest video shader. They affect the decoded stream, not the headset panel or system brightness.", foreground="#666666", wraplength=720).grid(row=8, column=0, columnspan=3, sticky="w", pady=(18, 0))

    def _add_transport_tab(self, notebook: ttk.Notebook) -> None:
        f = self._tab(notebook, "Advanced")
        cfg = self.config_data["transport"]
        self._entry(f, "transport.max_frame_mib", "Maximum encoded frame size (MiB)", cfg["max_frame_mib"], 0, "A safety limit for incoming/outgoing video access units.")
        self._entry(f, "transport.incomplete_frame_timeout_ms", "Incomplete-frame timeout (ms)", cfg["incomplete_frame_timeout_ms"], 1, "Drop partial frames quickly to avoid showing stale video.")
        self._entry(f, "transport.quest_receive_buffer_kib", "Quest receive buffer (KiB)", cfg["quest_receive_buffer_kib"], 2, "OS UDP receive queue; large buffers can reduce packet drops but may add latency.")
        ttk.Label(f, text="QBX deliberately caps UDP datagrams at 1200 bytes to avoid IP fragmentation. Packet size is not user-adjustable until fragmentation/loss recovery is implemented.", foreground="#666666", wraplength=720).grid(row=3, column=0, columnspan=3, sticky="w", pady=(18, 0))

    def _add_room_tab(self, notebook: ttk.Notebook) -> None:
        f = self._tab(notebook, "Virtual Room")
        cfg = self.config_data["room"]
        self._combo(f, "room.environment", "Window scenery",
                    ROOM_ENVIRONMENT_LABELS.get(cfg["environment"], cfg["environment"]),
                    tuple(ROOM_ENVIRONMENT_LABELS.values()), 0)
        self._combo(f, "room.room_size", "Room size", cfg["room_size"], ("small", "medium", "large"), 1)
        self._combo(f, "room.screen_destination", "PC stream appears on", cfg["screen_destination"], ("monitor", "tv", "both"), 2)
        self._entry(f, "room.monitor_count", "Desk monitor count (1–3)", cfg["monitor_count"], 3)
        self._check(f, "room.show_tv", "Show wall TV", cfg["show_tv"], 4)
        self._check(f, "room.show_couch", "Show couch", cfg["show_couch"], 5)
        self._check(f, "room.show_chair", "Show desk chair behind spawn", cfg["show_chair"], 6)
        self._check(f, "room.room_controls_enabled", "Enable in-room controller shortcuts", cfg["room_controls_enabled"], 7)
        self._check(f, "room.sitting_toggle_enabled", "Enable seated-view toggle (left X)", cfg["sitting_toggle_enabled"], 8)
        self._entry(f, "room.sitting_head_drop_m", "Seated camera drop (m)", cfg["sitting_head_drop_m"], 9)
        self._check(f, "room.movement_enabled", "Enable joystick room locomotion", cfg["movement_enabled"], 10)
        self._entry(f, "room.movement_speed", "Walk speed (m/s)", cfg["movement_speed"], 11)
        self._check(f, "room.smooth_turn", "Use smooth turning (otherwise snap turn)", cfg["smooth_turn"], 12)
        self._entry(f, "room.turn_speed", "Smooth-turn speed (rad/s)", cfg["turn_speed"], 13)
        self._entry(f, "room.snap_turn_degrees", "Snap turn angle (degrees)", cfg["snap_turn_degrees"], 14)
        self._check(f, "room.comfort_vignette", "Comfort vignette during movement", cfg["comfort_vignette"], 15)
        self._entry(f, "room.floor_offset_y", "Floor vertical offset (m)", cfg["floor_offset_y"], 16)
        ttk.Label(f, text="Controller shortcuts: left thumbstick click cycles Monitor → TV → Both; right thumbstick click cycles the scenery; left X toggles the seated camera height. Furniture is collidable at a simple box level, but seats do not use full physics.", foreground="#ffd28a", wraplength=760).grid(row=17, column=0, columnspan=3, sticky="w", pady=(18, 0))
        ttk.Label(f, text="This is the Quest standalone room client. It is not the missing Windows OpenXR compositor, and multiple virtual monitors currently mirror the same PC video source.", foreground="#666666", wraplength=760).grid(row=18, column=0, columnspan=3, sticky="w", pady=(10, 0))

    def _add_fake_vr_tab(self, notebook: ttk.Notebook) -> None:
        f = self._tab(notebook, "Fake VR / Game Bridge")
        cfg = self.config_data["fake_vr"]
        self._check(f, "fake_vr.enabled", "Enable experimental head-look bridge", cfg["enabled"], 0)
        self._entry(f, "fake_vr.allowed_processes", "Allowed game process names", cfg["allowed_processes"], 1)
        self._check(f, "fake_vr.yaw_enabled", "Head yaw controls horizontal camera", cfg["yaw_enabled"], 2)
        self._check(f, "fake_vr.pitch_enabled", "Head pitch controls vertical camera", cfg["pitch_enabled"], 3)
        self._check(f, "fake_vr.invert_pitch", "Invert vertical direction", cfg["invert_pitch"], 4)
        self._entry(f, "fake_vr.sensitivity_px_per_radian", "Horizontal sensitivity (px/rad)", cfg["sensitivity_px_per_radian"], 5)
        self._entry(f, "fake_vr.pitch_sensitivity", "Vertical sensitivity multiplier", cfg["pitch_sensitivity"], 6)
        self._entry(f, "fake_vr.deadzone_radians", "Angle deadzone (rad)", cfg["deadzone_radians"], 7)
        self._entry(f, "fake_vr.max_delta_pixels", "Max mouse delta per update (px)", cfg["max_delta_pixels"], 8)
        ttk.Label(f, text="Use: save settings → start QuestBridgeHost → focus the allowlisted game window → press F8 to enable head-look, F8 again to pause, and F9 to recenter. ESC always disables it. This uses standard SendInput mouse movement only; a game must accept mouse-look input for it to work.", foreground="#72531b", wraplength=760).grid(row=9, column=0, columnspan=3, sticky="w", pady=(18, 0))
        ttk.Label(f, text="For Roblox DOORS, this is only a head-look camera bridge. It cannot make the experience's first-person camera, body movement, interactions, or stereo rendering native VR. Real Roblox VR requires an OpenXR runtime accepted by Roblox.", foreground="#666666", wraplength=760).grid(row=10, column=0, columnspan=3, sticky="w", pady=(10, 0))

    def _add_play_modes_tab(self, notebook: ttk.Notebook) -> None:
        f = self._tab(notebook, "Play Modes")
        ttk.Label(f, text="Choose how you want to use QuestBridge XR", font=("Segoe UI", 15, "bold")).grid(
            row=0, column=0, columnspan=3, sticky="w", pady=(0, 12))
        ttk.Label(
            f,
            text=("COZY ROOM / FAKE VR\n"
                  "QuestBridge renders its own walkable room locally in the headset. Your PC desktop is streamed as a 2D image to the desk monitor, TV, or both. "
                  "The opt-in head-look bridge translates head yaw/pitch into standard Windows mouse movement for a focused, allowlisted game. "
                  "This is a comfort/compatibility mode, not stereo 6DoF game rendering; gameplay movement and interactions remain the game's normal inputs."),
            wraplength=740, justify="left").grid(row=1, column=0, columnspan=3, sticky="w", pady=(0, 12))
        ttk.Button(f, text="Open Fake VR settings", command=lambda: self._select_tab_by_title(notebook, "Fake VR / Game Bridge")).grid(
            row=2, column=0, sticky="w", pady=(0, 24))
        ttk.Separator(f).grid(row=3, column=0, columnspan=3, sticky="ew", pady=(0, 18))
        ttk.Label(f, text="REAL PCVR (META QUEST LINK)", font=("Segoe UI", 12, "bold")).grid(
            row=4, column=0, columnspan=3, sticky="w", pady=(0, 8))
        ttk.Label(
            f,
            text=("For actual stereo PCVR on Quest without SteamVR, use Meta Quest Link and Meta's PC OpenXR runtime. "
                  "This is a separate real-PCVR route; it does not run inside the QuestBridge cozy room and does not use a QuestBridge-owned Windows runtime. "
                  "Install/open Meta Quest Link, connect the headset by USB or supported Wi-Fi Link, and enable VR inside the game if the experience supports it. "
                  "Not every Roblox experience supports native VR, and a mouse-look bridge cannot add native VR support to an experience."),
            wraplength=740, justify="left").grid(row=5, column=0, columnspan=3, sticky="w", pady=(0, 12))
        ttk.Button(f, text="Launch Meta Quest Link", command=self._launch_meta_link).grid(row=6, column=0, sticky="w", pady=4)
        ttk.Button(f, text="Open Roblox DOORS", command=self._open_doors_page).grid(row=6, column=1, sticky="w", padx=(10, 0), pady=4)
        ttk.Button(f, text="Install & Launch Quest APK", command=self._install_quest_app).grid(row=6, column=2, sticky="w", padx=(10, 0), pady=4)
        ttk.Button(f, text="Open Meta PCVR setup", command=lambda: webbrowser.open("https://www.meta.com/quest/setup/")).grid(
            row=7, column=0, sticky="w", pady=4)
        ttk.Button(f, text="Inspect installed OpenXR runtimes", command=self._inspect_openxr_runtimes).grid(
            row=7, column=1, sticky="w", padx=(10, 0), pady=4)
        ttk.Button(f, text="Launch an EXE with Meta runtime…", command=lambda: self._launch_openxr_app("meta")).grid(
            row=7, column=2, sticky="w", padx=(10, 0), pady=4)
        ttk.Button(f, text="Launch an EXE with active runtime…", command=lambda: self._launch_openxr_app("active")).grid(
            row=8, column=0, sticky="w", pady=4)
        ttk.Button(f, text="Launch an EXE with QuestBridge runtime (experimental)…", command=lambda: self._launch_openxr_app("qbx")).grid(
            row=8, column=1, columnspan=2, sticky="w", padx=(10, 0), pady=4)
        ttk.Label(
            f,
            text=("Runtime helper: launches a chosen app with XR_RUNTIME_JSON set only in that app's environment. It does not change the Windows registry or stop a running runtime. "
                  "Meta mode selects Meta's installed runtime. QuestBridge mode selects the experimental D3D11 runtime shipped in the portable package; it is not conformance-tested and may not run arbitrary apps. Both overrides apply only to the child process."),
            foreground="#a3b1ce", wraplength=740, justify="left").grid(row=9, column=0, columnspan=3, sticky="w", pady=(12, 0))
        ttk.Label(
            f,
            text=("Safety note: the head-look bridge starts OFF by default. It only sends ordinary mouse deltas while F8 is enabled and an allowlisted game is the foreground window. "
                  "F8 pauses/resumes, F9 recenters, and ESC pauses. It does not inject code into Roblox or change game files."),
            foreground="#72531b", wraplength=740, justify="left").grid(row=10, column=0, columnspan=3, sticky="w", pady=(18, 0))

    @staticmethod
    def _select_tab_by_title(notebook: ttk.Notebook, title: str) -> None:
        for tab_id in notebook.tabs():
            if notebook.tab(tab_id, "text") == title:
                notebook.select(tab_id)
                return

    def _inspect_openxr_runtimes(self) -> None:
        """Show runtime manifests discovered without modifying Windows runtime settings."""
        try:
            runtimes = discover_runtimes()
            if not runtimes:
                details = ("No runtime manifest was found in XR_RUNTIME_JSON, the Windows active-runtime registry values, "
                           "or standard Meta/SteamVR install locations. Install Meta Quest Link for the practical no-SteamVR route.")
            else:
                lines = []
                for item in runtimes:
                    status = f"ERROR: {item.error}" if item.error else f"Library: {item.library_path}"
                    lines.append(f"{item.name}\n  Source: {item.source}\n  Manifest: {item.manifest}\n  {status}")
                details = "\n\n".join(lines)
            details += "\n\nQuestBridge did not change the active-runtime registry setting."
            messagebox.showinfo("OpenXR runtime discovery", details, parent=self)
        except Exception as exc:
            messagebox.showerror("OpenXR runtime discovery failed", str(exc), parent=self)

    def _launch_openxr_app(self, runtime_mode: str) -> None:
        """Launch a selected Windows EXE with a per-process OpenXR runtime override."""
        app_path = filedialog.askopenfilename(
            parent=self,
            title="Choose a Windows OpenXR application",
            filetypes=(("Windows applications", "*.exe"), ("All files", "*.*")),
        )
        if not app_path:
            return
        try:
            runtimes = discover_runtimes()
            manifest = resolve_runtime_manifest(runtime_mode, runtimes)
            if not messagebox.askyesno(
                "Launch OpenXR application",
                f"Launch this application with the {runtime_mode} OpenXR runtime?\n\n"
                f"Application: {app_path}\nManifest: {manifest}\n\n"
                "This sets XR_RUNTIME_JSON for the child process only. It does not change the system-wide active runtime.",
                parent=self,
            ):
                return
            child = launch_with_runtime(app_path, [], manifest)
            self.status_var.set(f"Launched {pathlib.Path(app_path).name} with {runtime_mode} runtime (PID {child.pid}).")
        except (OSError, ValueError, subprocess.SubprocessError) as exc:
            messagebox.showerror("Could not launch OpenXR application", str(exc), parent=self)

    def _launch_meta_link(self) -> None:
        candidates = []
        for env_name in ("ProgramFiles", "ProgramFiles(x86)"):
            base = os.environ.get(env_name)
            if base:
                candidates.extend([
                    pathlib.Path(base) / "Oculus" / "Support" / "oculus-client" / "OculusClient.exe",
                    pathlib.Path(base) / "Meta" / "Horizon" / "MetaQuestLink.exe",
                ])
        executable = next((candidate for candidate in candidates if candidate.is_file()), None)
        if executable is None:
            webbrowser.open("https://www.meta.com/quest/setup/")
            messagebox.showinfo(
                "Meta Quest Link not found",
                "I couldn't find the standard Meta/Oculus PC app in its usual install paths, so I opened Meta's setup page. "
                "Install or launch Meta Quest Link, then use its Link connection and make Meta's PC OpenXR runtime active for real PCVR.",
                parent=self,
            )
            return
        try:
            subprocess.Popen([str(executable)], cwd=str(executable.parent))
            self.status_var.set("Launched Meta Quest Link. Connect the headset there for the separate real-PCVR route.")
        except OSError as exc:
            messagebox.showerror("Could not launch Meta Quest Link", str(exc), parent=self)

    def _install_quest_app(self) -> None:
        """Install the packaged debug APK on exactly one connected Quest via ADB."""
        adb = shutil.which("adb")
        if not adb:
            messagebox.showinfo(
                "Android Debug Bridge not found",
                "Install Android platform-tools, enable Developer Mode and USB debugging on the Quest, then put adb on PATH. "
                "The Quest APK is included in the combined CI portable ZIP under QuestClient/.",
                parent=self,
            )
            return
        candidates = [
            ROOT / "QuestClient" / "QuestBridgeXR-Quest-debug.apk",
            ROOT / "quest-client" / "app" / "build" / "outputs" / "apk" / "debug" / "app-debug.apk",
        ]
        apk = next((item for item in candidates if item.is_file()), None)
        if apk is None:
            messagebox.showinfo(
                "Quest APK not found",
                "This folder does not contain the Quest APK yet. Use the combined portable ZIP from the GitHub Actions build, "
                "or build quest-client/app with Android Studio/Gradle.",
                parent=self,
            )
            return
        try:
            result = subprocess.run([adb, "devices"], capture_output=True, text=True, timeout=15, check=True)
            online = [line.split()[0] for line in result.stdout.splitlines()[1:]
                      if len(line.split()) >= 2 and line.split()[1] == "device"]
            if len(online) != 1:
                raise RuntimeError(f"Expected exactly one authorized Android/Quest device, found {len(online)}.\n{result.stdout}")
            install = subprocess.run([adb, "-s", online[0], "install", "-r", str(apk)],
                                     capture_output=True, text=True, timeout=120)
            if install.returncode != 0:
                raise RuntimeError(install.stderr.strip() or install.stdout.strip() or "adb install failed")
            launch = subprocess.run([adb, "-s", online[0], "shell", "monkey", "-p", "com.questbridge.xr", "1"],
                                     capture_output=True, text=True, timeout=20)
            if launch.returncode != 0:
                raise RuntimeError("APK installed, but automatic launch failed: " + (launch.stderr.strip() or launch.stdout.strip()))
            self.status_var.set(f"Installed and launched QuestBridge XR on device {online[0]}.")
            messagebox.showinfo("QuestBridge XR installed",
                                "Installed and asked Android to launch QuestBridge XR.\n\n"
                                "Set the PC's LAN IP in Studio and export/install config.ini if this is the first run.",
                                parent=self)
        except (OSError, subprocess.SubprocessError, RuntimeError) as exc:
            messagebox.showerror("Quest APK install failed", str(exc), parent=self)

    @staticmethod
    def _open_doors_page() -> None:
        webbrowser.open("https://www.roblox.com/games/6516141723/DOORS")

    def _load_form(self, cfg: dict[str, Any]) -> None:
        mapping = {
            "network.pc_ip": cfg["network"]["pc_ip"],
            "network.quest_ip": cfg["network"]["quest_ip"],
            "network.host_port": cfg["network"]["host_port"],
            "network.quest_local_port": cfg["network"]["quest_local_port"],
            "network.telemetry_port": cfg["network"]["telemetry_port"],
            "stream.source": cfg["stream"]["source"],
            "stream.layout": cfg["stream"]["layout"],
            "stream.width": cfg["stream"]["width"],
            "stream.height": cfg["stream"]["height"],
            "stream.fps": cfg["stream"]["fps"],
            "stream.bitrate_mbps": f"{cfg['stream']['bitrate_bps'] / 1_000_000:.2f}".rstrip("0").rstrip("."),
            "stream.preset": cfg["stream"]["preset"],
            "stream.tune": cfg["stream"]["tune"],
            "stream.h264_profile": cfg["stream"]["h264_profile"],
            "stream.rate_control": cfg["stream"]["rate_control"],
            "stream.maxrate_factor": cfg["stream"]["maxrate_factor"],
            "stream.vbv_buffer_ms": cfg["stream"]["vbv_buffer_ms"],
            "stream.threads": cfg["stream"]["threads"],
            "stream.udp_send_buffer_kib": cfg["stream"]["udp_send_buffer_kib"],
            "stream.gop_seconds": cfg["stream"]["gop_seconds"],
            "stream.config_repeat_seconds": cfg["stream"]["config_repeat_seconds"],
            "stream.draw_mouse": cfg["stream"]["draw_mouse"],
            "tracking.enabled": cfg["tracking"]["enabled"],
            "tracking.controllers_enabled": cfg["tracking"]["controllers_enabled"],
            "tracking.rate_hz": cfg["tracking"]["rate_hz"],
            "tracking.position_scale": cfg["tracking"]["position_scale"],
            "tracking.thumbstick_deadzone": cfg["tracking"]["thumbstick_deadzone"],
            "tracking.invert_thumbstick_x": cfg["tracking"]["invert_thumbstick_x"],
            "tracking.invert_thumbstick_y": cfg["tracking"]["invert_thumbstick_y"],
            "tracking.controller_pose": cfg["tracking"]["controller_pose"],
            "tracking.reference_space": cfg["tracking"]["reference_space"],
            "tracking.swap_face_buttons": cfg["tracking"]["swap_face_buttons"],
            "tracking.swap_hands": cfg["tracking"]["swap_hands"],
            "tracking.trigger_deadzone": cfg["tracking"]["trigger_deadzone"],
            "tracking.trigger_curve": cfg["tracking"]["trigger_curve"],
            "tracking.offset_x": cfg["tracking"]["offset_x"],
            "tracking.offset_y": cfg["tracking"]["offset_y"],
            "tracking.offset_z": cfg["tracking"]["offset_z"],
            "display.brightness": cfg["display"]["brightness"],
            "display.contrast": cfg["display"]["contrast"],
            "display.saturation": cfg["display"]["saturation"],
            "display.gamma": cfg["display"]["gamma"],
            "display.aspect_mode": cfg["display"]["aspect_mode"],
            "display.flip_horizontal": cfg["display"]["flip_horizontal"],
            "display.flip_vertical": cfg["display"]["flip_vertical"],
            "display.swap_eyes": cfg["display"]["swap_eyes"],
            "transport.max_frame_mib": cfg["transport"]["max_frame_mib"],
            "transport.incomplete_frame_timeout_ms": cfg["transport"]["incomplete_frame_timeout_ms"],
            "transport.quest_receive_buffer_kib": cfg["transport"]["quest_receive_buffer_kib"],
            "room.environment": ROOM_ENVIRONMENT_LABELS.get(cfg["room"]["environment"], cfg["room"]["environment"]),
            "room.room_size": cfg["room"]["room_size"],
            "room.screen_destination": cfg["room"]["screen_destination"],
            "room.monitor_count": cfg["room"]["monitor_count"],
            "room.show_tv": cfg["room"]["show_tv"],
            "room.show_couch": cfg["room"]["show_couch"],
            "room.show_chair": cfg["room"]["show_chair"],
            "room.room_controls_enabled": cfg["room"]["room_controls_enabled"],
            "room.sitting_toggle_enabled": cfg["room"]["sitting_toggle_enabled"],
            "room.sitting_head_drop_m": cfg["room"]["sitting_head_drop_m"],
            "room.movement_enabled": cfg["room"]["movement_enabled"],
            "room.movement_speed": cfg["room"]["movement_speed"],
            "room.smooth_turn": cfg["room"]["smooth_turn"],
            "room.turn_speed": cfg["room"]["turn_speed"],
            "room.snap_turn_degrees": cfg["room"]["snap_turn_degrees"],
            "room.comfort_vignette": cfg["room"]["comfort_vignette"],
            "room.floor_offset_y": cfg["room"]["floor_offset_y"],
            "fake_vr.enabled": cfg["fake_vr"]["enabled"],
            "fake_vr.allowed_processes": cfg["fake_vr"]["allowed_processes"],
            "fake_vr.yaw_enabled": cfg["fake_vr"]["yaw_enabled"],
            "fake_vr.pitch_enabled": cfg["fake_vr"]["pitch_enabled"],
            "fake_vr.invert_pitch": cfg["fake_vr"]["invert_pitch"],
            "fake_vr.sensitivity_px_per_radian": cfg["fake_vr"]["sensitivity_px_per_radian"],
            "fake_vr.pitch_sensitivity": cfg["fake_vr"]["pitch_sensitivity"],
            "fake_vr.deadzone_radians": cfg["fake_vr"]["deadzone_radians"],
            "fake_vr.max_delta_pixels": cfg["fake_vr"]["max_delta_pixels"],
        }
        for key, value in mapping.items():
            if key in self.vars:
                self.vars[key].set(value)
        self.profile_var.set(cfg.get("profile", "Balanced"))
        self.status_var.set(f"Loaded profile: {cfg.get('profile', 'Balanced')}")

    def _read_form(self) -> dict[str, Any]:
        cfg = copy.deepcopy(self.config_data)
        def val(key: str) -> str:
            return str(self.vars[key].get()).strip()
        cfg["profile"] = self.profile_var.get().strip() or "Custom"
        cfg["network"]["pc_ip"] = val("network.pc_ip")
        cfg["network"]["quest_ip"] = val("network.quest_ip")
        cfg["network"]["host_port"] = int(val("network.host_port"))
        cfg["network"]["quest_local_port"] = int(val("network.quest_local_port"))
        cfg["network"]["telemetry_port"] = int(val("network.telemetry_port"))
        cfg["stream"]["source"] = val("stream.source")
        cfg["stream"]["layout"] = val("stream.layout")
        cfg["stream"]["width"] = int(val("stream.width"))
        cfg["stream"]["height"] = int(val("stream.height"))
        cfg["stream"]["fps"] = int(val("stream.fps"))
        cfg["stream"]["bitrate_bps"] = int(round(float(val("stream.bitrate_mbps")) * 1_000_000))
        cfg["stream"]["preset"] = val("stream.preset")
        cfg["stream"]["tune"] = val("stream.tune")
        cfg["stream"]["h264_profile"] = val("stream.h264_profile")
        cfg["stream"]["rate_control"] = val("stream.rate_control")
        cfg["stream"]["maxrate_factor"] = float(val("stream.maxrate_factor"))
        cfg["stream"]["vbv_buffer_ms"] = int(val("stream.vbv_buffer_ms"))
        cfg["stream"]["threads"] = int(val("stream.threads"))
        cfg["stream"]["udp_send_buffer_kib"] = int(val("stream.udp_send_buffer_kib"))
        cfg["stream"]["gop_seconds"] = float(val("stream.gop_seconds"))
        cfg["stream"]["config_repeat_seconds"] = float(val("stream.config_repeat_seconds"))
        cfg["stream"]["draw_mouse"] = bool(self.vars["stream.draw_mouse"].get())
        cfg["tracking"]["enabled"] = bool(self.vars["tracking.enabled"].get())
        cfg["tracking"]["controllers_enabled"] = bool(self.vars["tracking.controllers_enabled"].get())
        cfg["tracking"]["rate_hz"] = int(val("tracking.rate_hz"))
        cfg["tracking"]["position_scale"] = float(val("tracking.position_scale"))
        cfg["tracking"]["thumbstick_deadzone"] = float(val("tracking.thumbstick_deadzone"))
        cfg["tracking"]["invert_thumbstick_x"] = bool(self.vars["tracking.invert_thumbstick_x"].get())
        cfg["tracking"]["invert_thumbstick_y"] = bool(self.vars["tracking.invert_thumbstick_y"].get())
        cfg["tracking"]["controller_pose"] = val("tracking.controller_pose")
        cfg["tracking"]["reference_space"] = val("tracking.reference_space")
        cfg["tracking"]["swap_face_buttons"] = bool(self.vars["tracking.swap_face_buttons"].get())
        cfg["tracking"]["swap_hands"] = bool(self.vars["tracking.swap_hands"].get())
        cfg["tracking"]["trigger_deadzone"] = float(val("tracking.trigger_deadzone"))
        cfg["tracking"]["trigger_curve"] = float(val("tracking.trigger_curve"))
        cfg["tracking"]["offset_x"] = float(val("tracking.offset_x"))
        cfg["tracking"]["offset_y"] = float(val("tracking.offset_y"))
        cfg["tracking"]["offset_z"] = float(val("tracking.offset_z"))
        cfg["display"]["brightness"] = float(val("display.brightness"))
        cfg["display"]["contrast"] = float(val("display.contrast"))
        cfg["display"]["saturation"] = float(val("display.saturation"))
        cfg["display"]["gamma"] = float(val("display.gamma"))
        cfg["display"]["aspect_mode"] = val("display.aspect_mode")
        cfg["display"]["flip_horizontal"] = bool(self.vars["display.flip_horizontal"].get())
        cfg["display"]["flip_vertical"] = bool(self.vars["display.flip_vertical"].get())
        cfg["display"]["swap_eyes"] = bool(self.vars["display.swap_eyes"].get())
        cfg["transport"]["max_frame_mib"] = int(val("transport.max_frame_mib"))
        cfg["transport"]["incomplete_frame_timeout_ms"] = int(val("transport.incomplete_frame_timeout_ms"))
        cfg["transport"]["quest_receive_buffer_kib"] = int(val("transport.quest_receive_buffer_kib"))
        cfg["room"]["environment"] = ROOM_ENVIRONMENT_BY_LABEL.get(val("room.environment"), val("room.environment"))
        cfg["room"]["room_size"] = val("room.room_size")
        cfg["room"]["screen_destination"] = val("room.screen_destination")
        cfg["room"]["monitor_count"] = int(val("room.monitor_count"))
        cfg["room"]["show_tv"] = bool(self.vars["room.show_tv"].get())
        cfg["room"]["show_couch"] = bool(self.vars["room.show_couch"].get())
        cfg["room"]["show_chair"] = bool(self.vars["room.show_chair"].get())
        cfg["room"]["room_controls_enabled"] = bool(self.vars["room.room_controls_enabled"].get())
        cfg["room"]["sitting_toggle_enabled"] = bool(self.vars["room.sitting_toggle_enabled"].get())
        cfg["room"]["sitting_head_drop_m"] = float(val("room.sitting_head_drop_m"))
        cfg["room"]["movement_enabled"] = bool(self.vars["room.movement_enabled"].get())
        cfg["room"]["movement_speed"] = float(val("room.movement_speed"))
        cfg["room"]["smooth_turn"] = bool(self.vars["room.smooth_turn"].get())
        cfg["room"]["turn_speed"] = float(val("room.turn_speed"))
        cfg["room"]["snap_turn_degrees"] = float(val("room.snap_turn_degrees"))
        cfg["room"]["comfort_vignette"] = bool(self.vars["room.comfort_vignette"].get())
        cfg["room"]["floor_offset_y"] = float(val("room.floor_offset_y"))
        cfg["fake_vr"]["enabled"] = bool(self.vars["fake_vr.enabled"].get())
        cfg["fake_vr"]["allowed_processes"] = val("fake_vr.allowed_processes")
        cfg["fake_vr"]["yaw_enabled"] = bool(self.vars["fake_vr.yaw_enabled"].get())
        cfg["fake_vr"]["pitch_enabled"] = bool(self.vars["fake_vr.pitch_enabled"].get())
        cfg["fake_vr"]["invert_pitch"] = bool(self.vars["fake_vr.invert_pitch"].get())
        cfg["fake_vr"]["sensitivity_px_per_radian"] = float(val("fake_vr.sensitivity_px_per_radian"))
        cfg["fake_vr"]["pitch_sensitivity"] = float(val("fake_vr.pitch_sensitivity"))
        cfg["fake_vr"]["deadzone_radians"] = float(val("fake_vr.deadzone_radians"))
        cfg["fake_vr"]["max_delta_pixels"] = int(val("fake_vr.max_delta_pixels"))
        validate_config(cfg)
        return cfg

    def _save_settings(self) -> bool:
        try:
            self.config_data = self._read_form()
            save_config(self.config_data)
            save_fake_vr_ini(self.config_data, ROOT / "config" / "fake_vr.ini")
            self.status_var.set(f"Saved settings to {DEFAULT_CONFIG_PATH.relative_to(ROOT)} and config/fake_vr.ini")
            return True
        except (ValueError, OSError) as exc:
            messagebox.showerror("Settings not saved", str(exc), parent=self)
            self.status_var.set(f"Settings error: {exc}")
            return False

    def _restore_defaults(self) -> None:
        if not messagebox.askyesno(
            "Restore defaults",
            "Replace the active settings with the built-in Balanced defaults? Saved named profiles will not be changed.",
            parent=self,
        ):
            return
        self.config_data = copy.deepcopy(DEFAULT_CONFIG)
        self._load_form(self.config_data)
        try:
            save_config(self.config_data)
            save_fake_vr_ini(self.config_data, ROOT / "config" / "fake_vr.ini")
            self.status_var.set("Restored and saved defaults, including the virtual room and game bridge config.")
        except (ValueError, OSError) as exc:
            messagebox.showerror("Could not restore defaults", str(exc), parent=self)

    def _on_profile_selected(self, _event: object = None) -> None:
        name = self.profile_var.get()
        try:
            self.config_data = apply_profile(name)
            self._load_form(self.config_data)
        except (ValueError, OSError) as exc:
            messagebox.showerror("Profile error", str(exc), parent=self)

    def _save_profile_as(self) -> None:
        if not self._save_settings():
            return
        name = simpledialog.askstring("Save profile", "Name this profile:", initialvalue="My Profile", parent=self)
        if not name or not name.strip():
            return
        name = name.strip()
        self.config_data["profile"] = name
        try:
            path = PROFILE_DIR / profile_filename(name)
            save_config(self.config_data)
            save_config(self.config_data, path)
            self.profile_box.configure(values=profile_names())
            self.profile_var.set(name)
            self.status_var.set(f"Saved custom profile: {path.relative_to(ROOT)}")
        except (ValueError, OSError) as exc:
            messagebox.showerror("Profile not saved", str(exc), parent=self)

    def _import_config(self) -> None:
        path = filedialog.askopenfilename(title="Import QuestBridge settings", filetypes=(("JSON config", "*.json"), ("All files", "*.*")), parent=self)
        if not path:
            return
        try:
            self.config_data = load_config(path)
            self._load_form(self.config_data)
            self.status_var.set(f"Imported {path}; click Save Settings to make it active.")
        except (ValueError, OSError) as exc:
            messagebox.showerror("Could not import config", str(exc), parent=self)

    def _export_quest_ini(self) -> None:
        try:
            cfg = self._read_form()
        except (ValueError, OSError) as exc:
            messagebox.showerror("Invalid settings", str(exc), parent=self)
            return
        path = filedialog.asksaveasfilename(title="Export Quest client config", initialfile="config.ini",
                                            defaultextension=".ini", filetypes=(("INI config", "*.ini"),), parent=self)
        if not path:
            return
        save_quest_ini(cfg, path)
        self.status_var.set(f"Exported Quest client config: {path}")

    def _install_quest_ini(self) -> None:
        if not self._save_settings():
            return
        try:
            cfg = self._read_form()
            GENERATED.mkdir(parents=True, exist_ok=True)
            local_ini = save_quest_ini(cfg, GENERATED / "quest-client-config.ini")
        except (ValueError, OSError) as exc:
            messagebox.showerror("Invalid settings", str(exc), parent=self)
            return
        adb = shutil.which("adb")
        if not adb:
            messagebox.showerror("ADB not found", "Install Android platform-tools and put adb.exe on PATH, or use Export Quest INI and push it manually.", parent=self)
            return
        try:
            devices = subprocess.run([adb, "devices"], capture_output=True, text=True, timeout=10, check=True).stdout.splitlines()
            online = [line.split()[0] for line in devices[1:] if line.strip() and len(line.split()) >= 2 and line.split()[1] == "device"]
            if len(online) != 1:
                raise RuntimeError(f"Expected exactly one online ADB device; found {len(online)}. Output:\n" + "\n".join(devices))
            target_dir = "/sdcard/Android/data/com.questbridge.xr/files"
            subprocess.run([adb, "-s", online[0], "shell", "mkdir", "-p", target_dir], capture_output=True, text=True, timeout=10, check=True)
            result = subprocess.run([adb, "-s", online[0], "push", str(local_ini), target_dir + "/config.ini"], capture_output=True, text=True, timeout=30)
            if result.returncode != 0:
                raise RuntimeError(result.stderr.strip() or result.stdout.strip() or "adb push failed")
            self.status_var.set(f"Installed config.ini to Quest device {online[0]}. Restart QuestBridge XR to apply changes.")
        except (subprocess.SubprocessError, OSError, RuntimeError) as exc:
            messagebox.showerror("ADB install failed", str(exc), parent=self)

    def _run_diagnostics(self, probe_host: bool) -> None:
        if not self._save_settings():
            return
        try:
            command = self._tool_command("doctor.py", "--config", str(DEFAULT_CONFIG_PATH))
        except OSError as exc:
            messagebox.showerror("Portable diagnostic tool missing", str(exc), parent=self)
            return
        if probe_host:
            command.extend(["--probe-host", "--timeout", "2.0"])
        try:
            result = subprocess.run(command, cwd=str(ROOT), capture_output=True, text=True, timeout=30, check=False)
            report = result.stdout.strip()
            if result.stderr.strip():
                report += "\n\nDiagnostics stderr:\n" + result.stderr.strip()
            if not report:
                report = "Diagnostics exited without output."
            self.status_var.set("Diagnostics completed; review the report for missing prerequisites or connection issues.")
            window = tk.Toplevel(self)
            window.title("QuestBridge XR Diagnostics" if not probe_host else "QuestBridge XR Host Connection Test")
            window.geometry("860x560")
            window.minsize(700, 400)
            window.transient(self)
            container = ttk.Frame(window, padding=12)
            container.pack(fill="both", expand=True)
            ttk.Label(container, text="Read-only setup report. INFO/WARN lines can be expected before the target builds are installed.",
                      wraplength=820).pack(anchor="w", pady=(0, 8))
            text = ScrolledText(container, wrap="word", font=("Consolas", 10), height=22)
            text.pack(fill="both", expand=True)
            text.insert("1.0", report)
            text.configure(state="disabled")
            buttons = ttk.Frame(container)
            buttons.pack(fill="x", pady=(8, 0))
            def copy_report() -> None:
                window.clipboard_clear()
                window.clipboard_append(report)
                self.status_var.set("Diagnostics report copied to clipboard.")
            ttk.Button(buttons, text="Copy Report", command=copy_report).pack(side="left")
            ttk.Button(buttons, text="Close", command=window.destroy).pack(side="right")
        except (OSError, subprocess.SubprocessError) as exc:
            messagebox.showerror("Diagnostics failed", str(exc), parent=self)

    def _tool_command(self, script_name: str, *args: str) -> list[str]:
        """Return a source-Python command or the matching portable helper EXE."""
        if getattr(sys, "frozen", False):
            executable = TOOLS / (pathlib.Path(script_name).stem + ".exe")
            if not executable.is_file():
                raise FileNotFoundError(
                    f"Portable helper is missing: {executable}. Rebuild the portable package."
                )
            return [str(executable), *args]
        return [sys.executable, str(TOOLS / script_name), *args]

    def _start_host(self) -> None:
        if not self._save_settings():
            return
        if self.host_process and self.host_process.poll() is None:
            self.status_var.set("Windows host is already running.")
            return
        candidates = [ROOT / "host" / "QuestBridgeHost.exe",
                      ROOT / "build" / "host" / "Release" / "QuestBridgeHost.exe",
                      ROOT / "build" / "host" / "Debug" / "QuestBridgeHost.exe"]
        exe = next((candidate for candidate in candidates if candidate.exists()), None)
        if exe is None:
            messagebox.showinfo("Build the host first", "Windows host binary not found. On Windows, open Developer PowerShell for Visual Studio and run:\n\n.\\scripts\\build-windows.ps1", parent=self)
            return
        try:
            self.host_process = subprocess.Popen([
                str(exe), str(self.config_data["network"]["host_port"]),
                str(self.config_data["network"]["telemetry_port"]),
            ], cwd=str(ROOT))
            telemetry = self.config_data["network"]["telemetry_port"]
            detail = f"; localhost telemetry on UDP {telemetry}" if telemetry else "; localhost telemetry disabled"
            self.status_var.set(f"Started Windows host on UDP {self.config_data['network']['host_port']}{detail}.")
        except OSError as exc:
            messagebox.showerror("Could not start host", str(exc), parent=self)

    def _start_stream(self) -> None:
        if not self._save_settings():
            return
        if self.stream_process and self.stream_process.poll() is None:
            self.status_var.set("Video streamer is already running.")
            return
        try:
            command = self._tool_command("stream_test_video.py", "--config", str(DEFAULT_CONFIG_PATH))
            self.stream_process = subprocess.Popen(command, cwd=str(ROOT))
            self.status_var.set("Started video streamer. Select source=test for a generated pattern, desktop for a 2D preview, or mirror to stream the experimental runtime mirror.")
        except OSError as exc:
            messagebox.showerror("Could not start stream", str(exc), parent=self)

    def _start_telemetry_monitor(self) -> None:
        if not self._save_settings():
            return
        if self.config_data["network"]["telemetry_port"] == 0:
            messagebox.showinfo("Telemetry disabled", "Set a nonzero Local telemetry feed UDP port on the Connection tab first.", parent=self)
            return
        if self.telemetry_process and self.telemetry_process.poll() is None:
            self.status_var.set("Telemetry monitor is already running.")
            return
        try:
            command = self._tool_command("monitor_telemetry.py", "--config", str(DEFAULT_CONFIG_PATH))
            self.telemetry_process = subprocess.Popen(command, cwd=str(ROOT))
            self.status_var.set("Started localhost telemetry monitor. It prints HMD/controller packets received from the Windows host.")
        except OSError as exc:
            messagebox.showerror("Could not start telemetry monitor", str(exc), parent=self)

    def _stop_processes(self) -> None:
        stopped = []
        for name, process in (("streamer", self.stream_process), ("host", self.host_process), ("telemetry monitor", self.telemetry_process)):
            if process and process.poll() is None:
                process.terminate()
                try:
                    process.wait(timeout=3)
                except subprocess.TimeoutExpired:
                    process.kill()
                stopped.append(name)
        self.status_var.set("Stopped: " + ", ".join(stopped) if stopped else "No active child processes.")

    def _open_config_folder(self) -> None:
        try:
            if os.name == "nt":
                os.startfile(str(ROOT / "config"))  # type: ignore[attr-defined]
            elif sys.platform == "darwin":
                subprocess.Popen(["open", str(ROOT / "config")])
            else:
                subprocess.Popen(["xdg-open", str(ROOT / "config")])
        except OSError as exc:
            messagebox.showerror("Could not open folder", str(exc), parent=self)

    def _on_close(self) -> None:
        # Don't silently kill host/streamer: ask before closing if either remains active.
        active = [name for name, p in (("host", self.host_process), ("streamer", self.stream_process), ("telemetry monitor", self.telemetry_process)) if p and p.poll() is None]
        if active and messagebox.askyesno("Development processes are running", "Stop " + " and ".join(active) + " before closing?", parent=self):
            self._stop_processes()
        self.destroy()


def main() -> int:
    try:
        app = QuestBridgeStudio()
    except (ValueError, OSError) as exc:
        print(f"QuestBridge Studio startup failed: {exc}", file=sys.stderr)
        return 2
    app.mainloop()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
