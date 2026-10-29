#!/usr/bin/env python3
"""Tk widget smoke tests; run under xvfb-run on Linux or normally on Windows."""
from __future__ import annotations
import pathlib
import sys
import tkinter as tk
import unittest
ROOT = pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
from questbridge_studio import QuestBridgeStudio
from qbx_config import ROOM_ENVIRONMENTS

class StudioUiTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        try:
            cls.app = QuestBridgeStudio()
        except tk.TclError as exc:
            raise unittest.SkipTest(f"Tk display unavailable: {exc}")

    @classmethod
    def tearDownClass(cls):
        app = getattr(cls, "app", None)
        if app is not None:
            try:
                app.destroy()
            except tk.TclError:
                pass

    def test_play_modes_and_virtual_room_tabs_exist(self):
        notebooks = []
        def visit(widget):
            if widget.winfo_class() == "TNotebook":
                notebooks.append(widget)
            for child in widget.winfo_children():
                visit(child)
        visit(self.app)
        self.assertEqual(len(notebooks), 1)
        labels = [notebooks[0].tab(tab_id, "text") for tab_id in notebooks[0].tabs()]
        self.assertIn("Virtual Room", labels)
        self.assertIn("Fake VR / Game Bridge", labels)
        self.assertIn("Play Modes", labels)

    def test_virtual_room_exposes_new_scenery_choices(self):
        combo_values = []
        def visit(widget):
            if widget.winfo_class() == "TCombobox":
                combo_values.append(tuple(self.app.tk.splitlist(widget.cget("values"))))
            for child in widget.winfo_children():
                visit(child)
        visit(self.app)
        self.assertTrue(any(set(ROOM_ENVIRONMENTS).issubset(set(values)) for values in combo_values))

    def test_each_config_setting_has_a_widget(self):
        self.assertEqual(len(self.app.vars), 75)

if __name__ == "__main__":
    unittest.main(verbosity=2)
