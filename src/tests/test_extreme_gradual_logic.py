import importlib.util
import io
import math
import re
import sys
import types
import unittest
from contextlib import redirect_stdout
from pathlib import Path


TESTS_DIR = Path(__file__).resolve().parent


class _SoundDeviceStub(types.ModuleType):
    def __init__(self):
        super().__init__("sounddevice")
        self.default = types.SimpleNamespace(device=("mock-input", "mock-output"))

    def query_devices(self):
        return []

    def play(self, *_args, **_kwargs):
        return None

    def wait(self):
        return None


def load_script_module(module_name: str, file_name: str):
    script_path = TESTS_DIR / file_name
    spec = importlib.util.spec_from_file_location(module_name, script_path)
    module = importlib.util.module_from_spec(spec)

    previous_numpy = sys.modules.get("numpy")
    previous_sounddevice = sys.modules.get("sounddevice")
    sys.modules["numpy"] = types.ModuleType("numpy")
    sys.modules["sounddevice"] = _SoundDeviceStub()

    try:
        assert spec.loader is not None
        spec.loader.exec_module(module)
    finally:
        if previous_numpy is None:
            sys.modules.pop("numpy", None)
        else:
            sys.modules["numpy"] = previous_numpy

        if previous_sounddevice is None:
            sys.modules.pop("sounddevice", None)
        else:
            sys.modules["sounddevice"] = previous_sounddevice

    return module


def run_guidance(module, function_name: str, *args):
    events = []
    output_buffer = io.StringIO()

    original_play_beep = module.play_beep
    original_play_in_tune_jingle = module.play_in_tune_jingle
    original_sleep = module.time.sleep

    def fake_beep(frequency, duration_ms, amplitude=0.9, preserve_frequency=False):
        events.append(("beep", float(frequency), int(duration_ms), float(amplitude)))

    def fake_jingle():
        events.append(("jingle",))

    def fake_sleep(seconds):
        events.append(("sleep", float(seconds)))

    module.play_beep = fake_beep
    module.play_in_tune_jingle = fake_jingle
    module.time.sleep = fake_sleep

    try:
        with redirect_stdout(output_buffer):
            getattr(module, function_name)(*args)
    finally:
        module.play_beep = original_play_beep
        module.play_in_tune_jingle = original_play_in_tune_jingle
        module.time.sleep = original_sleep

    return events, output_buffer.getvalue()


def extract_guidance_beeps(events):
    return [event for event in events if event[0] == "beep"][2:]


def extract_reported_values(output: str, label: str):
    pattern = rf"{label}: (\d+)ms"
    return [int(value) for value in re.findall(pattern, output)]


class ExtremeAndGradualGuidanceTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.extreme = load_script_module("extreme_under_test", "test_extreme.py")
        cls.gradual = load_script_module("gradual_under_test", "test_gradual.py")

    def test_parse_note_matches_reference_frequency(self):
        self.assertAlmostEqual(self.extreme.parse_note("A4"), 440.0, places=6)
        self.assertAlmostEqual(self.gradual.parse_note("E2"), 82.4068892282, places=6)
        self.assertAlmostEqual(self.gradual.parse_note("C4"), 261.6255653006, places=6)

    def test_extreme_flat_case_uses_tune_up_beeps_and_slows_near_target(self):
        events, output = run_guidance(self.extreme, "extreme_guiding_beeps", "E4", 0.954, 4)
        guidance_beeps = extract_guidance_beeps(events)
        gaps = extract_reported_values(output, "Gap")
        durations = extract_reported_values(output, "Duration")

        self.assertTrue(guidance_beeps)
        self.assertTrue(all(math.isclose(beep[1], 1100.0, rel_tol=0.0, abs_tol=1e-9) for beep in guidance_beeps))
        self.assertIn("TUNE UP", output)
        self.assertEqual(events[-2], ("jingle",))
        self.assertEqual(sorted(gaps), gaps)
        self.assertEqual(sorted(durations), durations)

    def test_extreme_sharp_case_uses_tune_down_beeps(self):
        events, output = run_guidance(self.extreme, "extreme_guiding_beeps", "A2", 1.035, 4)
        guidance_beeps = extract_guidance_beeps(events)

        self.assertTrue(guidance_beeps)
        self.assertTrue(all(math.isclose(beep[1], 600.0, rel_tol=0.0, abs_tol=1e-9) for beep in guidance_beeps))
        self.assertIn("TUNE DOWN", output)
        self.assertEqual(events[-2], ("jingle",))

    def test_gradual_flat_case_glides_down_from_two_octaves_above(self):
        target = self.gradual.parse_note("E4")
        events, output = run_guidance(self.gradual, "gradual_guiding_beeps", "E4", 0.954, 4)
        guidance_beeps = extract_guidance_beeps(events)
        glide_frequencies = [beep[1] for beep in guidance_beeps]

        self.assertTrue(guidance_beeps)
        self.assertAlmostEqual(glide_frequencies[0], target * 4.0, places=6)
        self.assertEqual(glide_frequencies, sorted(glide_frequencies, reverse=True))
        self.assertIn("TUNE UP", output)
        self.assertEqual(events[-2], ("jingle",))

    def test_gradual_sharp_case_uses_higher_glide_band(self):
        target = self.gradual.parse_note("A2")
        events, output = run_guidance(self.gradual, "gradual_guiding_beeps", "A2", 1.035, 4)
        guidance_beeps = extract_guidance_beeps(events)
        glide_frequencies = [beep[1] for beep in guidance_beeps]

        self.assertTrue(guidance_beeps)
        self.assertAlmostEqual(glide_frequencies[0], target * 8.0, places=6)
        self.assertEqual(glide_frequencies, sorted(glide_frequencies, reverse=True))
        self.assertIn("TUNE DOWN", output)
        self.assertEqual(events[-2], ("jingle",))


if __name__ == "__main__":
    unittest.main()
