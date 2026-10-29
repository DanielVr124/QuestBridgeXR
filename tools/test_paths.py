from __future__ import annotations
import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch
import qbx_paths

class ProjectPathTests(unittest.TestCase):
    def test_source_script_resolves_repository_root(self):
        expected = Path(qbx_paths.__file__).resolve().parents[1]
        self.assertEqual(qbx_paths.project_root(qbx_paths.__file__), expected)

    def test_frozen_root_executable_resolves_its_parent(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp) / 'portable'
            root.mkdir()
            exe = root / 'QuestBridgeStudio.exe'
            exe.touch()
            with patch.object(qbx_paths.sys, 'frozen', True, create=True), \
                 patch.object(qbx_paths.sys, 'executable', str(exe)):
                self.assertEqual(qbx_paths.project_root('ignored.py'), root)

    def test_frozen_tool_executable_resolves_parent_of_tools_folder(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp) / 'portable'
            tools = root / 'tools'
            tools.mkdir(parents=True)
            exe = tools / 'doctor.exe'
            exe.touch()
            with patch.object(qbx_paths.sys, 'frozen', True, create=True), \
                 patch.object(qbx_paths.sys, 'executable', str(exe)):
                self.assertEqual(qbx_paths.project_root('ignored.py'), root)

if __name__ == '__main__':
    unittest.main()
