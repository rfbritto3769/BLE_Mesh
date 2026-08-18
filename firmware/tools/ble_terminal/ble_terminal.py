#!/usr/bin/env python3
"""BLE terminal GUI -- scan, connect, and exchange commands with a BLE device.

Built for the WBZ351 (or any BLE peripheral exposing a UART-style service).
Uses `bleak` for cross-platform BLE (Windows/macOS/Linux) and tkinter for the
GUI (bundled with standard Python -- no extra install).

Run:
    pip install bleak
    python ble_terminal.py

The app auto-detects the two most common UART-over-BLE profiles:
  * Microchip Transparent UART Service (TRS)
  * Nordic UART Service (NUS)
If your firmware uses a different service, pick the notify (RX) and write (TX)
characteristics manually from the drop-downs after connecting.
"""

from __future__ import annotations

import asyncio
import math
import os
import queue
import threading
import time
import tkinter as tk
from tkinter import ttk, scrolledtext, filedialog, simpledialog, colorchooser

try:
    from bleak import BleakClient, BleakScanner
except ImportError:  # pragma: no cover - friendly message when dep missing
    raise SystemExit(
        "The 'bleak' package is required.\n\n    pip install bleak\n"
    )


# --- Known UART-over-BLE profiles ----------------------------------------
# Each entry: label -> (service, notify/RX char [device->app], write/TX char).
KNOWN_PROFILES = {
    "Microchip Transparent UART": (
        "49535343-fe7d-4ae5-8fa9-9fafd205e455",
        "49535343-1e4d-4bd9-ba61-23c647249616",  # notify (device -> app)
        "49535343-8841-43f4-a8d4-ecbe34729bb3",  # write  (app -> device)
    ),
    "Nordic UART Service": (
        "6e400001-b5a3-f393-e0a9-e50e24dcca9e",
        "6e400003-b5a3-f393-e0a9-e50e24dcca9e",  # notify (device -> app)
        "6e400002-b5a3-f393-e0a9-e50e24dcca9e",  # write  (app -> device)
    ),
}

LINE_ENDINGS = {
    "None": b"",
    "LF (\\n)": b"\n",
    "CR (\\r)": b"\r",
    "CRLF (\\r\\n)": b"\r\n",
}


# --- Network topology rules (derived from the provisioning table) --------
# Node ID -> cluster and role:  cluster = (id-1)//10, gateway when id%10 == 1.
def node_cluster(nid: int) -> int:
    return (nid - 1) // 10


def node_is_gateway(nid: int) -> bool:
    return nid % 10 == 1


def cluster_gateway_id(cluster: int) -> int:
    return cluster * 10 + 1


def is_unicast_dst(dst: int) -> bool:
    """True for a specific-node destination (not off/all/cluster broadcast)."""
    return dst not in (0x00, 0xFF) and not (0xC0 <= dst <= 0xCF)

# One-click command buttons. Format, one per line:  Label | HEX BYTES
# A line with no "|" (or empty command) becomes a section header.
# Editable in-app via the "Edit..." button; persisted to macros.txt.
DEFAULT_MACROS = """\
-- Provisioning (DST=0x00 -> directly connected board) --
Assign Node 1  (Gateway cluster 0) | 00 00 00 05 05 01
Assign Node 2  (Local cluster 0)   | 00 00 00 05 05 02
Assign Node 5  (Local cluster 0)   | 00 00 00 05 05 05
Assign Node 11 (Gateway cluster 1) | 00 00 00 05 05 0B
Assign Node 20 (Local cluster 1)   | 00 00 00 05 05 14
Assign Node 99 (Local cluster 9)   | 00 00 00 05 05 63
Unprovision (erase ID, revert)     | 00 00 00 05 06

-- LED control --
Node 2  -> 100% Red    | 02 00 00 05 01 64 00 00
Node 15 -> 50% Green   | 0F 00 00 05 01 00 32 00
ALL -> 100% Red        | FF 00 00 05 01 64 00 00
ALL -> White 50%       | FF 00 00 05 01 32 32 32
ALL -> OFF             | FF 00 00 05 01 00 00 00
Cluster 0 -> 100% Red  | C0 00 00 05 01 64 00 00
Cluster 1 -> 50% Green | C1 00 00 05 01 00 32 00
Cluster 9 -> OFF       | C9 00 00 05 01 00 00 00
Identify Node 25       | 19 00 00 05 04
Broadcast identify     | FF 00 00 05 04
"""


# =========================================================================
# BLE worker: owns an asyncio loop on a background thread. The GUI submits
# coroutines to it and receives results/events through a thread-safe queue.
# =========================================================================
class BleWorker:
    def __init__(self, event_q: "queue.Queue"):
        self.event_q = event_q
        self.loop = asyncio.new_event_loop()
        self.client: BleakClient | None = None
        self.chars_by_handle: dict[int, object] = {}
        self._thread = threading.Thread(target=self._run, daemon=True)
        self._thread.start()

    def _run(self):
        asyncio.set_event_loop(self.loop)
        self.loop.run_forever()

    def submit(self, coro):
        """Schedule a coroutine on the worker loop from the GUI thread."""
        return asyncio.run_coroutine_threadsafe(coro, self.loop)

    def post(self, kind: str, **data):
        self.event_q.put((kind, data))

    def stop(self):
        self.loop.call_soon_threadsafe(self.loop.stop)

    # --- coroutines ------------------------------------------------------
    async def scan(self, timeout: float = 5.0):
        self.post("status", text=f"Scanning ({timeout:.0f}s)...")
        try:
            devices = await BleakScanner.discover(timeout=timeout)
            items = [(d.name or "(no name)", d.address) for d in devices]
            items.sort(key=lambda t: (t[0] == "(no name)", t[0].lower()))
            self.post("scan_result", devices=items)
            self.post("status", text=f"Found {len(items)} device(s)")
        except Exception as exc:  # noqa: BLE001
            self.post("error", text=f"Scan failed: {exc}")
            self.post("status", text="Scan failed")

    async def connect(self, address: str):
        self.post("status", text=f"Connecting to {address}...")
        try:
            self.client = BleakClient(
                address, disconnected_callback=self._on_disconnect
            )
            await self.client.connect()

            self.chars_by_handle.clear()
            chars = []
            for service in self.client.services:
                for c in service.characteristics:
                    self.chars_by_handle[c.handle] = c
                    chars.append(
                        {
                            "handle": c.handle,
                            "uuid": str(c.uuid),
                            "properties": list(c.properties),
                            "service": str(service.uuid),
                        }
                    )
            self.post("connected", address=address, chars=chars)
            self.post("status", text=f"Connected to {address}")
        except Exception as exc:  # noqa: BLE001
            self.post("error", text=f"Connect failed: {exc}")
            self.post("disconnected", clean=False)

    async def start_notify(self, handle: int):
        char = self.chars_by_handle.get(handle)
        if char is None:
            return
        try:
            await self.client.start_notify(char, self._notify_cb)
            self.post("info", text=f"Subscribed to notify {char.uuid}")
        except Exception as exc:  # noqa: BLE001
            self.post("error", text=f"start_notify failed: {exc}")

    async def stop_notify(self, handle: int):
        char = self.chars_by_handle.get(handle)
        if char is None or self.client is None:
            return
        try:
            await self.client.stop_notify(char)
        except Exception:  # noqa: BLE001
            pass

    def _notify_cb(self, _sender, data: bytearray):
        self.post("rx", data=bytes(data))

    async def write(self, handle: int, data: bytes, response: bool):
        if self.client is None or not self.client.is_connected:
            self.post("error", text="Not connected")
            return
        char = self.chars_by_handle.get(handle)
        if char is None:
            self.post("error", text="No write characteristic selected")
            return
        try:
            await self.client.write_gatt_char(char, data, response=response)
            self.post("tx", data=data)
        except Exception as exc:  # noqa: BLE001
            self.post("error", text=f"Write failed: {exc}")

    async def disconnect(self):
        if self.client is not None:
            try:
                await self.client.disconnect()
            except Exception:  # noqa: BLE001
                pass
        self.post("disconnected", clean=True)

    def _on_disconnect(self, _client):
        self.post("disconnected", clean=True)


# =========================================================================
# GUI
# =========================================================================
class BleTerminalApp:
    def __init__(self, root: tk.Tk):
        self.root = root
        self.event_q: "queue.Queue" = queue.Queue()
        self.worker = BleWorker(self.event_q)

        self.device_map: dict[str, str] = {}       # label -> address
        self.notify_map: dict[str, int] = {}        # label -> handle
        self.write_map: dict[str, int] = {}         # label -> handle
        self.write_props: dict[int, list] = {}      # handle -> properties
        self.connected = False

        self.history: list[str] = []                # sent-command recall
        self.history_idx = 0
        self.logfile = None                         # live session log handle

        self.net_nodes: dict[int, dict] = {}        # node id -> {"color": (r,g,b)}
        self.net_connected_id = None                # node id the BLE board is provisioned as
        self.net_window = None                      # NetworkWindow when open
        self.macros_path = os.path.join(
            os.path.dirname(os.path.abspath(__file__)), "macros.txt"
        )
        if not os.path.exists(self.macros_path):
            try:
                self._save_macros_text(DEFAULT_MACROS)
            except OSError:
                pass

        root.title("BLE Terminal")
        root.geometry("820x600")
        root.minsize(680, 460)

        self._build_connection_bar()
        self._build_char_bar()
        self._build_terminal()
        self._build_input_bar()
        self._build_statusbar()

        root.protocol("WM_DELETE_WINDOW", self._on_close)
        self.root.after(40, self._poll_events)

    # --- widgets ---------------------------------------------------------
    def _build_connection_bar(self):
        f = ttk.Frame(self.root, padding=(8, 6))
        f.pack(fill=tk.X)

        self.scan_btn = ttk.Button(f, text="Scan", command=self._on_scan)
        self.scan_btn.pack(side=tk.LEFT)

        self.device_cb = ttk.Combobox(f, state="readonly", width=44)
        self.device_cb.pack(side=tk.LEFT, padx=6, fill=tk.X, expand=True)

        self.connect_btn = ttk.Button(
            f, text="Connect", command=self._on_connect_toggle, state=tk.DISABLED
        )
        self.connect_btn.pack(side=tk.LEFT)

        ttk.Button(f, text="Network map", command=self._open_network_map).pack(
            side=tk.LEFT, padx=(6, 0)
        )

    def _build_char_bar(self):
        f = ttk.Frame(self.root, padding=(8, 0))
        f.pack(fill=tk.X)

        ttk.Label(f, text="Notify (RX):").pack(side=tk.LEFT)
        self.notify_cb = ttk.Combobox(f, state="disabled", width=30)
        self.notify_cb.pack(side=tk.LEFT, padx=(4, 10))
        self.notify_cb.bind("<<ComboboxSelected>>", self._on_notify_change)

        ttk.Label(f, text="Write (TX):").pack(side=tk.LEFT)
        self.write_cb = ttk.Combobox(f, state="disabled", width=30)
        self.write_cb.pack(side=tk.LEFT, padx=4)

    def _build_terminal(self):
        outer = ttk.Frame(self.root, padding=(8, 6))
        outer.pack(fill=tk.BOTH, expand=True)

        paned = ttk.PanedWindow(outer, orient=tk.HORIZONTAL)
        paned.pack(fill=tk.BOTH, expand=True)

        term_frame = ttk.Frame(paned)
        self.term = scrolledtext.ScrolledText(
            term_frame, wrap=tk.WORD, state=tk.DISABLED,
            font=("Consolas", 10), background="#101418", foreground="#d6d6d6",
        )
        self.term.pack(fill=tk.BOTH, expand=True)
        self.term.tag_config("rx", foreground="#8fd6a0")
        self.term.tag_config("tx", foreground="#78aaff")
        self.term.tag_config("info", foreground="#888888")
        self.term.tag_config("error", foreground="#ff7b72")
        self.term.tag_config("ts", foreground="#5a6270")

        macro_frame = ttk.Frame(paned, width=250)
        self._build_led_gauge(macro_frame)
        self._build_color_picker(macro_frame)
        self._build_macros(macro_frame)

        paned.add(term_frame, weight=4)
        paned.add(macro_frame, weight=1)

    def _build_macros(self, parent):
        header = ttk.Frame(parent)
        header.pack(fill=tk.X, pady=(0, 4))
        ttk.Label(header, text="Quick commands", font=("", 9, "bold")).pack(side=tk.LEFT)
        ttk.Button(header, text="Edit...", width=7, command=self._on_edit_macros).pack(side=tk.RIGHT)

        canvas = tk.Canvas(parent, highlightthickness=0, width=240)
        vsb = ttk.Scrollbar(parent, orient=tk.VERTICAL, command=canvas.yview)
        canvas.configure(yscrollcommand=vsb.set)
        vsb.pack(side=tk.RIGHT, fill=tk.Y)
        canvas.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)

        self.macro_canvas = canvas
        self.macro_inner = ttk.Frame(canvas)
        self._macro_window = canvas.create_window((0, 0), window=self.macro_inner, anchor="nw")
        self.macro_inner.bind(
            "<Configure>", lambda _e: canvas.configure(scrollregion=canvas.bbox("all"))
        )
        canvas.bind(
            "<Configure>", lambda e: canvas.itemconfigure(self._macro_window, width=e.width)
        )
        canvas.bind("<Enter>", lambda _e: canvas.bind_all("<MouseWheel>", self._macro_scroll))
        canvas.bind("<Leave>", lambda _e: canvas.unbind_all("<MouseWheel>"))

        self._rebuild_macro_buttons()

    def _macro_scroll(self, event):
        self.macro_canvas.yview_scroll(int(-event.delta / 120), "units")

    def _rebuild_macro_buttons(self):
        for w in self.macro_inner.winfo_children():
            w.destroy()
        for entry in self._parse_macros(self._load_macros_text()):
            if entry["type"] == "header":
                ttk.Label(
                    self.macro_inner, text=entry["text"], font=("", 8, "bold"),
                    foreground="#666666", wraplength=220, justify=tk.LEFT,
                ).pack(fill=tk.X, pady=(7, 1), padx=2)
            elif self._parse_hex(entry["cmd"]) is None:
                ttk.Button(
                    self.macro_inner, text=f"{entry['label']}  (bad hex)", state=tk.DISABLED
                ).pack(fill=tk.X, pady=1, padx=2)
            else:
                ttk.Button(
                    self.macro_inner, text=entry["label"],
                    command=lambda c=entry["cmd"]: self._send_macro(c),
                ).pack(fill=tk.X, pady=1, padx=2)

    @staticmethod
    def _parse_macros(text: str):
        entries = []
        for raw in text.splitlines():
            line = raw.strip()
            if not line or line.startswith("#"):
                continue
            if "|" in line:
                label, cmd = line.split("|", 1)
                label, cmd = label.strip(), cmd.strip()
                if cmd:
                    entries.append({"type": "button", "label": label, "cmd": cmd})
                else:
                    entries.append({"type": "header", "text": label})
            else:
                entries.append({"type": "header", "text": line})
        return entries

    def _load_macros_text(self) -> str:
        try:
            if os.path.exists(self.macros_path):
                with open(self.macros_path, encoding="utf-8") as fh:
                    return fh.read()
        except OSError:
            pass
        return DEFAULT_MACROS

    def _save_macros_text(self, content: str):
        with open(self.macros_path, "w", encoding="utf-8") as fh:
            fh.write(content)

    def _on_edit_macros(self):
        win = tk.Toplevel(self.root)
        win.title("Edit quick commands")
        win.geometry("560x520")
        win.transient(self.root)

        ttk.Label(
            win, padding=(8, 6),
            text=("One command per line:   Label | HEX BYTES\n"
                  "A line with no '|' becomes a section header. "
                  "Hex may use spaces, e.g.  00 00 00 05 05 01"),
            justify=tk.LEFT,
        ).pack(anchor=tk.W)

        txt = scrolledtext.ScrolledText(win, wrap=tk.NONE, font=("Consolas", 10))
        txt.pack(fill=tk.BOTH, expand=True, padx=8)
        txt.insert("1.0", self._load_macros_text())

        bar = ttk.Frame(win, padding=8)
        bar.pack(fill=tk.X)

        def restore():
            txt.delete("1.0", tk.END)
            txt.insert("1.0", DEFAULT_MACROS)

        def save():
            content = txt.get("1.0", tk.END)
            try:
                self._save_macros_text(content)
            except OSError as exc:
                self._append(f"Could not save macros: {exc}\n", "error")
                return
            self._rebuild_macro_buttons()
            self._set_status("Quick commands saved")
            win.destroy()

        ttk.Button(bar, text="Restore defaults", command=restore).pack(side=tk.LEFT)
        ttk.Button(bar, text="Cancel", command=win.destroy).pack(side=tk.RIGHT)
        ttk.Button(bar, text="Save", command=save).pack(side=tk.RIGHT, padx=(0, 6))

        win.grab_set()

    def _send_macro(self, cmd: str):
        data = self._parse_hex(cmd)
        if data is None:
            self._append(f"Macro has invalid hex: {cmd}\n", "error")
            return
        self._send_bytes(data)

    # --- LED state gauge -------------------------------------------------
    def _build_led_gauge(self, parent):
        self._led_rgb = (0, 0, 0)          # last commanded R,G,B in percent
        box = ttk.LabelFrame(parent, text="LED state", padding=6)
        box.pack(fill=tk.X, pady=(0, 6))
        self.led_target = ttk.Label(box, text="Target: --", font=("", 8))
        self.led_target.pack(anchor=tk.W)
        self.led_canvas = tk.Canvas(
            box, height=96, highlightthickness=0, background="#101418"
        )
        self.led_canvas.pack(fill=tk.X)
        self.led_canvas.bind("<Configure>", lambda _e: self._draw_led_gauge())

    def _draw_led_gauge(self):
        c = self.led_canvas
        c.delete("all")
        w = max(c.winfo_width(), 200)
        pad = 6
        r, g, b = (min(100, max(0, v)) for v in self._led_rgb)
        swatch = f"#{round(r * 2.55):02x}{round(g * 2.55):02x}{round(b * 2.55):02x}"

        c.create_rectangle(pad, pad, pad + 56, pad + 82, fill=swatch, outline="#3a3a3a")
        if (r, g, b) == (0, 0, 0):
            c.create_text(pad + 28, pad + 41, text="OFF", fill="#777777", font=("", 8, "bold"))

        bx = pad + 66
        bw = max(40, w - bx - pad - 34)
        for i, (name, val, color) in enumerate(
            (("R", r, "#ff5555"), ("G", g, "#55dd66"), ("B", b, "#5599ff"))
        ):
            y = pad + 8 + i * 26
            c.create_text(bx - 12, y + 6, text=name, fill="#aaaaaa", font=("Consolas", 9))
            c.create_rectangle(bx, y, bx + bw, y + 12, fill="#20262c", outline="")
            fw = round(bw * val / 100)
            if fw > 0:
                c.create_rectangle(bx, y, bx + fw, y + 12, fill=color, outline="")
            c.create_text(bx + bw + 18, y + 6, text=f"{val}%", fill="#cccccc", font=("Consolas", 9))

    def _maybe_update_led(self, data: bytes):
        # LED "set dimmer" frame: [DST] 00 00 05 01 R G B  (R/G/B are 0..100 %).
        if len(data) == 8 and data[3] == 0x05 and data[4] == 0x01:
            self._led_rgb = (data[5], data[6], data[7])
            self.led_target.config(text=f"Target: {self._describe_dst(data[0])}")
            self._draw_led_gauge()

    @staticmethod
    def _describe_dst(dst: int) -> str:
        if dst == 0x00:
            return "connected board"
        if dst == 0xFF:
            return "ALL nodes"
        if 0xC0 <= dst <= 0xCF:
            return f"Cluster {dst - 0xC0}"
        return f"Node {dst}"

    # --- Color picker ----------------------------------------------------
    def _build_color_picker(self, parent):
        self._pick_after = None
        self.pick_vars = {}
        self.pick_labels = {}

        box = ttk.LabelFrame(parent, text="Color picker", padding=6)
        box.pack(fill=tk.X, pady=(0, 6))

        tf = ttk.Frame(box)
        tf.pack(fill=tk.X)
        ttk.Label(tf, text="To:").pack(side=tk.LEFT)
        self.pick_target = ttk.Combobox(
            tf, state="readonly", width=10, values=["Connected", "ALL", "Cluster", "Node"]
        )
        self.pick_target.set("ALL")
        self.pick_target.pack(side=tk.LEFT, padx=4)
        self.pick_target.bind("<<ComboboxSelected>>", lambda _e: self._pick_target_changed())
        self.pick_num = ttk.Spinbox(tf, from_=0, to=254, width=5)
        self.pick_num.set(0)
        self.pick_num.pack(side=tk.LEFT)

        for name in ("R", "G", "B"):
            row = ttk.Frame(box)
            row.pack(fill=tk.X, pady=1)
            ttk.Label(row, text=name, width=2).pack(side=tk.LEFT)
            var = tk.DoubleVar(value=0)
            self.pick_vars[name] = var
            ttk.Scale(row, from_=0, to=100, variable=var,
                      command=lambda _v: self._pick_changed()).pack(
                side=tk.LEFT, fill=tk.X, expand=True, padx=4)
            lbl = ttk.Label(row, width=4, text="0%")
            lbl.pack(side=tk.RIGHT)
            self.pick_labels[name] = lbl

        br = ttk.Frame(box)
        br.pack(fill=tk.X, pady=(4, 0))
        self.pick_live = tk.BooleanVar(value=False)
        ttk.Checkbutton(br, text="Live", variable=self.pick_live).pack(side=tk.LEFT)
        ttk.Button(br, text="Send color", command=self._pick_send).pack(side=tk.RIGHT)

        self._pick_target_changed()

    def _pick_target_changed(self):
        t = self.pick_target.get()
        if t == "Cluster":
            self.pick_num.config(state="normal", from_=0, to=15)
        elif t == "Node":
            self.pick_num.config(state="normal", from_=1, to=254)
        else:
            self.pick_num.config(state="disabled")

    def _pick_changed(self):
        for name, var in self.pick_vars.items():
            self.pick_labels[name].config(text=f"{int(round(var.get()))}%")
        if self.pick_live.get():
            if self._pick_after is not None:
                self.root.after_cancel(self._pick_after)
            self._pick_after = self.root.after(130, self._pick_send)

    def _pick_dst(self):
        t = self.pick_target.get()
        if t == "Connected":
            return 0x00
        if t == "ALL":
            return 0xFF
        try:
            n = int(self.pick_num.get())
        except (ValueError, tk.TclError):
            return None
        if t == "Cluster":
            return 0xC0 + n if 0 <= n <= 15 else None
        if t == "Node":
            return n if 1 <= n <= 254 else None
        return 0xFF

    def _pick_send(self):
        self._pick_after = None
        dst = self._pick_dst()
        if dst is None:
            self._set_status("Color picker: invalid target")
            return
        self.send_led(dst, self.pick_vars["R"].get(),
                      self.pick_vars["G"].get(), self.pick_vars["B"].get())

    # --- Network map -----------------------------------------------------
    def _open_network_map(self):
        if self.net_window is not None:
            self.net_window.win.lift()
            self.net_window.win.focus_force()
            return
        self.net_window = NetworkWindow(self)

    def _net_observe(self, data: bytes):
        """Update the network model from a sent frame; redraw if the map is open."""
        if len(data) < 5 or data[3] != 0x05:
            return
        dst, cmd = data[0], data[4]
        changed = False

        if cmd == 0x05 and len(data) >= 6:            # provision: 00 00 00 05 05 <id>
            nid = data[5]
            if 1 <= nid <= 254:
                self.net_nodes.setdefault(nid, {})
                self.net_connected_id = nid
                changed = True
        elif cmd == 0x06:                             # unprovision
            self.net_connected_id = None
            changed = True
        elif cmd == 0x01 and len(data) == 8:          # LED set: DST .. R G B
            changed = self._net_apply_color(dst, (data[5], data[6], data[7]))
        elif cmd == 0x04 and is_unicast_dst(dst):     # identify a specific node
            self.net_nodes.setdefault(dst, {})
            changed = True

        if changed and self.net_window is not None:
            self.net_window.draw()

    def _net_apply_color(self, dst: int, rgb: tuple) -> bool:
        if dst == 0xFF:
            for info in self.net_nodes.values():
                info["color"] = rgb
            return bool(self.net_nodes)
        if 0xC0 <= dst <= 0xCF:
            cl = dst - 0xC0
            touched = False
            for nid, info in self.net_nodes.items():
                if node_cluster(nid) == cl:
                    info["color"] = rgb
                    touched = True
            return touched
        if dst == 0x00:
            if self.net_connected_id is not None:
                self.net_nodes.setdefault(self.net_connected_id, {})["color"] = rgb
                return True
            return False
        if is_unicast_dst(dst):
            self.net_nodes.setdefault(dst, {})["color"] = rgb
            return True
        return False

    def _build_input_bar(self):
        f = ttk.Frame(self.root, padding=(8, 0))
        f.pack(fill=tk.X)

        self.entry = ttk.Entry(f)
        self.entry.pack(side=tk.LEFT, fill=tk.X, expand=True)
        self.entry.bind("<Return>", lambda _e: self._on_send())
        self.entry.bind("<Up>", self._history_prev)
        self.entry.bind("<Down>", self._history_next)

        self.ending_cb = ttk.Combobox(
            f, state="readonly", width=12, values=list(LINE_ENDINGS.keys())
        )
        self.ending_cb.set("LF (\\n)")
        self.ending_cb.pack(side=tk.LEFT, padx=6)

        self.send_btn = ttk.Button(
            f, text="Send", command=self._on_send, state=tk.DISABLED
        )
        self.send_btn.pack(side=tk.LEFT)

    def _build_statusbar(self):
        f = ttk.Frame(self.root, padding=(8, 4))
        f.pack(fill=tk.X)

        self.rx_format = tk.StringVar(value="ASCII")
        self.tx_format = tk.StringVar(value="ASCII")
        self.timestamps = tk.BooleanVar(value=True)
        self.autoscroll = tk.BooleanVar(value=True)

        ttk.Label(f, text="Receive:").pack(side=tk.LEFT)
        ttk.Combobox(f, textvariable=self.rx_format, state="readonly", width=6,
                     values=["ASCII", "Hex"]).pack(side=tk.LEFT, padx=(2, 10))
        ttk.Label(f, text="Send:").pack(side=tk.LEFT)
        ttk.Combobox(f, textvariable=self.tx_format, state="readonly", width=6,
                     values=["ASCII", "Hex"]).pack(side=tk.LEFT, padx=(2, 10))
        self.tx_format.trace_add("write", self._on_tx_format_change)

        ttk.Checkbutton(f, text="Timestamps", variable=self.timestamps).pack(side=tk.LEFT)
        ttk.Checkbutton(f, text="Autoscroll", variable=self.autoscroll).pack(side=tk.LEFT, padx=(6, 0))
        ttk.Button(f, text="Clear", command=self._clear_terminal).pack(side=tk.LEFT, padx=(6, 0))
        ttk.Button(f, text="Save log...", command=self._save_log).pack(side=tk.LEFT, padx=(6, 0))
        self.log_enabled = tk.BooleanVar(value=False)
        ttk.Checkbutton(f, text="Log to file", variable=self.log_enabled,
                        command=self._toggle_logfile).pack(side=tk.LEFT, padx=(6, 0))

        self.status = ttk.Label(f, text="Idle", anchor=tk.E)
        self.status.pack(side=tk.RIGHT, fill=tk.X, expand=True)

    # --- button handlers -------------------------------------------------
    def _on_scan(self):
        self.scan_btn.config(state=tk.DISABLED)
        self.device_cb.set("")
        self.device_map.clear()
        self.worker.submit(self.worker.scan(5.0))

    def _on_connect_toggle(self):
        if self.connected:
            self.worker.submit(self.worker.disconnect())
            self._set_status("Disconnecting...")
            return
        label = self.device_cb.get()
        address = self.device_map.get(label)
        if not address:
            self._set_status("Select a device first")
            return
        self.connect_btn.config(state=tk.DISABLED)
        self.worker.submit(self.worker.connect(address))

    def _on_tx_format_change(self, *_args):
        # Line endings are appended only to ASCII text; irrelevant for raw hex.
        if self.tx_format.get() == "Hex":
            self.ending_cb.config(state=tk.DISABLED)
            self.entry.config(foreground="#a06000")
        else:
            self.ending_cb.config(state="readonly")
            self.entry.config(foreground="")

    def _history_prev(self, _evt=None):
        if not self.history:
            return "break"
        self.history_idx = max(0, self.history_idx - 1)
        self._set_entry(self.history[self.history_idx])
        return "break"

    def _history_next(self, _evt=None):
        if not self.history:
            return "break"
        self.history_idx = min(len(self.history), self.history_idx + 1)
        if self.history_idx == len(self.history):
            self._set_entry("")
        else:
            self._set_entry(self.history[self.history_idx])
        return "break"

    def _set_entry(self, text: str):
        self.entry.delete(0, tk.END)
        self.entry.insert(0, text)
        self.entry.icursor(tk.END)

    def _save_log(self):
        path = filedialog.asksaveasfilename(
            title="Save terminal log",
            defaultextension=".txt",
            filetypes=[("Text files", "*.txt"), ("All files", "*.*")],
        )
        if not path:
            return
        try:
            with open(path, "w", encoding="utf-8") as fh:
                fh.write(self.term.get("1.0", tk.END))
            self._set_status(f"Saved log to {path}")
        except OSError as exc:
            self._append(f"Could not save log: {exc}\n", "error")

    def _toggle_logfile(self):
        if self.log_enabled.get():
            path = filedialog.asksaveasfilename(
                title="Log session to file",
                defaultextension=".log",
                filetypes=[("Log files", "*.log"), ("Text files", "*.txt"), ("All files", "*.*")],
            )
            if not path:
                self.log_enabled.set(False)
                return
            try:
                self.logfile = open(path, "a", encoding="utf-8")
                self.logfile.write(time.strftime("\n==== session started %Y-%m-%d %H:%M:%S ====\n"))
                self.logfile.flush()
                self._set_status(f"Logging to {path}")
            except OSError as exc:
                self.log_enabled.set(False)
                self._append(f"Could not open log file: {exc}\n", "error")
        else:
            self._close_logfile()
            self._set_status("Logging stopped")

    def _close_logfile(self):
        if self.logfile is not None:
            try:
                self.logfile.close()
            except OSError:
                pass
            self.logfile = None

    def _on_notify_change(self, _evt=None):
        label = self.notify_cb.get()
        handle = self.notify_map.get(label)
        if handle is not None:
            self.worker.submit(self.worker.start_notify(handle))

    def _on_send(self):
        if not self.connected:
            return
        text = self.entry.get()
        if text == "":
            return
        if self.tx_format.get() == "Hex":
            data = self._parse_hex(text)
            if data is None:
                self._append("Invalid hex input (expected pairs like: 01 A2 FF)\n", "error")
                return
        else:
            data = text.encode("utf-8") + LINE_ENDINGS[self.ending_cb.get()]

        if not self._send_bytes(data):
            return
        if not self.history or self.history[-1] != text:
            self.history.append(text)
        self.history_idx = len(self.history)
        self.entry.delete(0, tk.END)

    def _send_bytes(self, data: bytes) -> bool:
        """Write raw bytes to the selected TX characteristic. Returns success."""
        if not self.connected:
            self._set_status("Not connected")
            return False
        write_handle = self.write_map.get(self.write_cb.get())
        if write_handle is None:
            self._set_status("No write characteristic selected")
            return False
        props = self.write_props.get(write_handle, [])
        response = "write" in props  # else write-without-response
        self.worker.submit(self.worker.write(write_handle, data, response))
        return True

    def send_led(self, dst: int, r, g, b):
        """Build and send an LED dimmer frame: DST 00 00 05 01 R G B."""
        def clamp(v):
            return max(0, min(100, int(round(v))))
        self._send_bytes(bytes([dst & 0xFF, 0x00, 0x00, 0x05, 0x01,
                                clamp(r), clamp(g), clamp(b)]))

    def send_identify(self, nid: int):
        self._send_bytes(bytes([nid & 0xFF, 0x00, 0x00, 0x05, 0x04]))

    # --- event pump ------------------------------------------------------
    def _poll_events(self):
        try:
            while True:
                kind, data = self.event_q.get_nowait()
                self._handle_event(kind, data)
        except queue.Empty:
            pass
        self.root.after(40, self._poll_events)

    def _handle_event(self, kind: str, data: dict):
        if kind == "status":
            self._set_status(data["text"])
        elif kind == "info":
            self._append(data["text"] + "\n", "info")
        elif kind == "error":
            self._append(data["text"] + "\n", "error")
            self.scan_btn.config(state=tk.NORMAL)
        elif kind == "scan_result":
            self._populate_devices(data["devices"])
            self.scan_btn.config(state=tk.NORMAL)
        elif kind == "connected":
            self._on_connected(data["address"], data["chars"])
        elif kind == "disconnected":
            self._on_disconnected()
        elif kind == "rx":
            self._show_bytes(data["data"], "rx", hexed=self.rx_format.get() == "Hex")
        elif kind == "tx":
            self._show_bytes(data["data"], "tx", hexed=self.tx_format.get() == "Hex", prefix="» ")
            self._maybe_update_led(data["data"])
            self._net_observe(data["data"])

    # --- state transitions ----------------------------------------------
    def _populate_devices(self, devices):
        self.device_map = {f"{name}  [{addr}]": addr for name, addr in devices}
        labels = list(self.device_map.keys())
        self.device_cb.config(values=labels)
        if labels:
            self.device_cb.current(0)
            self.connect_btn.config(state=tk.NORMAL)

    def _on_connected(self, address, chars):
        self.connected = True
        self.connect_btn.config(text="Disconnect", state=tk.NORMAL)
        self.send_btn.config(state=tk.NORMAL)
        self.notify_cb.config(state="readonly")
        self.write_cb.config(state="readonly")
        self._append(f"Connected to {address}\n", "info")

        # Build notify / write characteristic drop-downs.
        self.notify_map.clear()
        self.write_map.clear()
        self.write_props.clear()
        notify_labels, write_labels = [], []
        for c in chars:
            props = c["properties"]
            label = f"{self._short_uuid(c['uuid'])} (h{c['handle']})"
            if "notify" in props or "indicate" in props:
                self.notify_map[label] = c["handle"]
                notify_labels.append(label)
            if "write" in props or "write-without-response" in props:
                self.write_map[label] = c["handle"]
                self.write_props[c["handle"]] = props
                write_labels.append(label)

        self.notify_cb.config(values=notify_labels)
        self.write_cb.config(values=write_labels)

        # Auto-select a known UART profile if present.
        n_handle, w_handle = self._match_known_profile(chars)
        self._preselect(self.notify_cb, self.notify_map, n_handle, notify_labels)
        self._preselect(self.write_cb, self.write_map, w_handle, write_labels)

        if self.notify_cb.get():
            self._on_notify_change()
        else:
            self._append(
                "No notify characteristic found -- pick one manually.\n", "info"
            )

    def _on_disconnected(self):
        was = self.connected
        self.connected = False
        self.connect_btn.config(text="Connect", state=tk.NORMAL)
        self.send_btn.config(state=tk.DISABLED)
        self.notify_cb.config(state="disabled")
        self.write_cb.config(state="disabled")
        if was:
            self._append("Disconnected\n", "info")
        self._set_status("Disconnected")

    # --- helpers ---------------------------------------------------------
    def _match_known_profile(self, chars):
        service_uuids = {c["service"].lower() for c in chars}
        char_uuids = {c["uuid"].lower(): c["handle"] for c in chars}
        for _label, (svc, notify_uuid, write_uuid) in KNOWN_PROFILES.items():
            if svc in service_uuids:
                return char_uuids.get(notify_uuid), char_uuids.get(write_uuid)
        # Fallback: first notify-capable and first write-capable characteristic.
        n = next((c["handle"] for c in chars
                  if "notify" in c["properties"] or "indicate" in c["properties"]), None)
        w = next((c["handle"] for c in chars
                  if "write" in c["properties"] or "write-without-response" in c["properties"]), None)
        return n, w

    @staticmethod
    def _preselect(combo, label_map, handle, labels):
        if handle is None:
            if labels:
                combo.current(0)
            return
        for label, h in label_map.items():
            if h == handle:
                combo.set(label)
                return
        if labels:
            combo.current(0)

    @staticmethod
    def _short_uuid(uuid: str) -> str:
        # Collapse the standard 128-bit base so custom UUIDs stay readable.
        u = uuid.lower()
        if u.endswith("-0000-1000-8000-00805f9b34fb") and u.startswith("0000"):
            return f"0x{u[4:8]}"
        return uuid

    @staticmethod
    def _parse_hex(text: str):
        cleaned = text.replace("0x", "").replace(",", " ").replace("-", " ")
        cleaned = "".join(cleaned.split())
        if len(cleaned) % 2 != 0:
            return None
        try:
            return bytes.fromhex(cleaned)
        except ValueError:
            return None

    def _show_bytes(self, data: bytes, tag: str, hexed: bool, prefix: str = ""):
        if hexed:
            body = " ".join(f"{b:02X}" for b in data)
        else:
            body = data.decode("utf-8", errors="replace")
        self._append(prefix + body + ("\n" if hexed or not body.endswith("\n") else ""), tag)

    def _append(self, text: str, tag: str):
        stamp = time.strftime("[%H:%M:%S] ") if (self.timestamps.get() and tag in ("rx", "tx")) else ""
        self.term.config(state=tk.NORMAL)
        if stamp:
            self.term.insert(tk.END, stamp, "ts")
        self.term.insert(tk.END, text, tag)
        if self.autoscroll.get():
            self.term.see(tk.END)
        self.term.config(state=tk.DISABLED)

        if self.logfile is not None:
            try:
                self.logfile.write(stamp + text)
                self.logfile.flush()
            except OSError:
                pass

    def _clear_terminal(self):
        self.term.config(state=tk.NORMAL)
        self.term.delete("1.0", tk.END)
        self.term.config(state=tk.DISABLED)

    def _set_status(self, text: str):
        self.status.config(text=text)

    def _on_close(self):
        try:
            if self.connected:
                self.worker.submit(self.worker.disconnect())
                time.sleep(0.2)
        finally:
            self._close_logfile()
            self.worker.stop()
            self.root.destroy()


class NetworkWindow:
    """A live radial map: mesh hub -> cluster gateways -> local nodes.

    The model lives on the app (`app.net_nodes`, `app.net_connected_id`); this
    window just renders it and can add/clear nodes manually.
    """

    def __init__(self, app: "BleTerminalApp"):
        self.app = app
        self.win = tk.Toplevel(app.root)
        self.win.title("Network map")
        self.win.geometry("640x560")
        self.win.protocol("WM_DELETE_WINDOW", self.close)

        bar = ttk.Frame(self.win, padding=(8, 6))
        bar.pack(fill=tk.X)
        ttk.Button(bar, text="Add node...", command=self._add_node).pack(side=tk.LEFT)
        ttk.Button(bar, text="Clear", command=self._clear).pack(side=tk.LEFT, padx=(6, 0))
        ttk.Label(
            bar, foreground="#777",
            text="   ★ gateway   ·  yellow ring = BLE-connected board",
        ).pack(side=tk.LEFT)

        self.canvas = tk.Canvas(self.win, background="#0d1116", highlightthickness=0)
        self.canvas.pack(fill=tk.BOTH, expand=True)
        self.canvas.bind("<Configure>", lambda _e: self.draw())
        # Tag bindings apply to node items created on every redraw.
        self.canvas.tag_bind("node", "<Button-1>", self._on_node_click)
        self.canvas.tag_bind("node", "<Button-3>", self._on_node_click)
        self.draw()

    def close(self):
        self.app.net_window = None
        self.win.destroy()

    def _node_at(self, _event):
        item = self.canvas.find_withtag("current")
        if not item:
            return None
        for tag in self.canvas.gettags(item[0]):
            if tag.startswith("id:"):
                return int(tag[3:])
        return None

    def _on_node_click(self, event):
        nid = self._node_at(event)
        if nid is None:
            return
        role = "gateway" if node_is_gateway(nid) else "local"
        menu = tk.Menu(self.win, tearoff=0)
        menu.add_command(label=f"Node {nid} — cluster {node_cluster(nid)}, {role}", state="disabled")
        menu.add_separator()
        menu.add_command(label="Identify", command=lambda: self.app.send_identify(nid))
        menu.add_command(label="Set color...", command=lambda: self._set_node_color(nid))
        menu.add_command(label="Turn off", command=lambda: self.app.send_led(nid, 0, 0, 0))
        menu.add_separator()
        menu.add_command(label="Remove from map", command=lambda: self._remove_node(nid))
        menu.tk_popup(event.x_root, event.y_root)

    def _set_node_color(self, nid):
        rgb, _hex = colorchooser.askcolor(parent=self.win, title=f"Color for node {nid}")
        if rgb:
            r, g, b = (round(v / 255 * 100) for v in rgb)
            self.app.send_led(nid, r, g, b)

    def _remove_node(self, nid):
        self.app.net_nodes.pop(nid, None)
        if self.app.net_connected_id == nid:
            self.app.net_connected_id = None
        self.draw()

    def _add_node(self):
        nid = simpledialog.askinteger(
            "Add node", "Node ID (1-254):", parent=self.win, minvalue=1, maxvalue=254
        )
        if nid is not None:
            self.app.net_nodes.setdefault(nid, {})
            self.draw()

    def _clear(self):
        self.app.net_nodes.clear()
        self.app.net_connected_id = None
        self.draw()

    def draw(self):
        c = self.canvas
        c.delete("all")
        w = max(c.winfo_width(), 320)
        h = max(c.winfo_height(), 320)
        cx, cy = w / 2, h / 2
        nodes = self.app.net_nodes
        conn = self.app.net_connected_id

        if not nodes:
            c.create_text(
                cx, cy, fill="#8a929c", justify=tk.CENTER, font=("", 11),
                text="No nodes yet.\nProvision a board or send an LED/identify "
                     "command,\nor use “Add node...”.",
            )
            return

        clusters: dict[int, set] = {}
        for nid in nodes:
            clusters.setdefault(node_cluster(nid), set()).add(nid)

        radius = max(90, min(w, h) / 2 - 96)
        order = sorted(clusters)
        count = len(order)

        # Backbone spokes + cluster subtrees.
        for i, cl in enumerate(order):
            ang = -math.pi / 2 + 2 * math.pi * i / count
            gx = cx + radius * math.cos(ang)
            gy = cy + radius * math.sin(ang)
            c.create_line(cx, cy, gx, gy, fill="#39414b", width=2)

            locals_ = sorted(n for n in clusters[cl] if not node_is_gateway(n))
            for j, lid in enumerate(locals_):
                la = 2 * math.pi * j / max(1, len(locals_))
                lx = gx + 52 * math.cos(la)
                ly = gy + 52 * math.sin(la)
                c.create_line(gx, gy, lx, ly, fill="#2a2f37")
                self._draw_node(lx, ly, lid, gateway=False, conn=conn)

            self._draw_node(gx, gy, cluster_gateway_id(cl), gateway=True, conn=conn)

        # Central mesh hub.
        c.create_oval(cx - 20, cy - 20, cx + 20, cy + 20,
                      fill="#2b3440", outline="#5a6270", width=2)
        c.create_text(cx, cy, text="MESH", fill="#cfd8e3", font=("", 8, "bold"))

    def _draw_node(self, x, y, nid, gateway, conn):
        c = self.canvas
        rgb = self.app.net_nodes.get(nid, {}).get("color")
        if rgb:
            r, g, b = (min(100, max(0, v)) for v in rgb)
            fill = f"#{round(r * 2.55):02x}{round(g * 2.55):02x}{round(b * 2.55):02x}"
            dark_text = (r + g + b) > 160
        else:
            fill = "#3a3f46"
            dark_text = False
        rad = 17 if gateway else 12
        if nid == conn:
            outline, width = "#e8c14a", 3
        elif gateway:
            outline, width = "#8791a0", 2
        else:
            outline, width = "#4a515a", 1
        tags = ("node", f"id:{nid}")
        c.create_oval(x - rad, y - rad, x + rad, y + rad,
                      fill=fill, outline=outline, width=width, tags=tags)
        label = f"{nid}★" if gateway else str(nid)
        c.create_text(x, y, text=label, font=("", 8, "bold"),
                      fill="#0b0d10" if dark_text else "#e6e6e6", tags=tags)
        if gateway:
            c.create_text(x, y + rad + 8, text=f"cluster {node_cluster(nid)}",
                          fill="#9aa3ad", font=("", 7))


def main():
    root = tk.Tk()
    # Use a native-ish theme where available.
    try:
        ttk.Style().theme_use("vista")
    except tk.TclError:
        pass
    BleTerminalApp(root)
    root.mainloop()


if __name__ == "__main__":
    main()
