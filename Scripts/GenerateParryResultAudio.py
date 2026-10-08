"""Build short stage impacts and copy the original critical strike for perfects."""
from pathlib import Path
import argparse
import shutil
import subprocess
import wave
import numpy as np


def generate(ffmpeg, sources):
    target = Path(__file__).resolve().parents[1] / 'Saved/AudioImports/ParryResult'
    target.mkdir(parents=True, exist_ok=True)
    samples = {}
    paths = {
        'hit': sources / 'Base_Optional/hit.wav',
        'parry': sources / 'Base_Optional/DSGNMisc_MELEE-Sword Parry_HY_PC-004.wav',
        'critical': sources / 'Combo5/DSGNMisc_SKILL IMPACT-Critical Strike_HY_PC-003.wav',
    }
    for key, path in paths.items():
        result = subprocess.run([ffmpeg, '-v', 'error', '-i', str(path), '-ar', '48000',
                                 '-ac', '2', '-f', 'f32le', '-'], check=True, capture_output=True)
        audio = np.frombuffer(result.stdout, dtype='<f4').reshape(-1, 2).astype(float)
        samples[key] = audio / max(0.001, np.max(np.abs(audio)))
    # No powerup layer is included in any ordinary impact.
    recipes = [({'hit': 0.8, 'parry': 0.15}, 0.20),
               ({'hit': 0.8, 'parry': 0.35}, 0.25),
               ({'hit': 0.7, 'critical': 0.50}, 0.32),
               ({'hit': 0.6, 'critical': 0.80}, 0.40)]
    for index, (layers, duration) in enumerate(recipes, 1):
        count = round(48000 * duration)
        mixed = np.zeros((count, 2))
        for key, gain in layers.items():
            length = min(count, len(samples[key]))
            mixed[:length] += samples[key][:length] * gain
        mixed[:96] *= np.linspace(0, 1, 96)[:, None]
        mixed[-1920:] *= np.linspace(1, 0, 1920)[:, None]
        mixed *= 0.8 / max(0.001, np.max(np.abs(mixed)))
        pcm = np.rint(mixed * 32767).astype('<i2')
        with wave.open(str(target / ('SFX_ParryStageImpact_0' + str(index) + '.wav')), 'wb') as output:
            output.setparams((2, 2, 48000, 0, 'NONE', 'not compressed'))
            output.writeframes(pcm.tobytes())
    shutil.copyfile(paths['critical'], target / 'SFX_PerfectParryCriticalStrike.wav')
    print('Generated:', target)


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--ffmpeg', required=True)
    parser.add_argument('--sources', type=Path, required=True)
    args = parser.parse_args()
    generate(args.ffmpeg, args.sources)
