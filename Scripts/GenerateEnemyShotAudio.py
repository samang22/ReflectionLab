"""Generate a quiet original retro-energy pew, 48 kHz mono PCM."""
from pathlib import Path
import numpy as np
import wave

rate = 48000
t = np.arange(int(rate * 0.14)) / rate
phase = 2 * np.pi * (240 * t + 650 * 0.025 * (1 - np.exp(-t / 0.025)))
signal = (np.sin(phase) + 0.12 * np.sin(phase * 2))
signal *= np.minimum(t / 0.004, 1) * np.exp(-t / 0.035) * np.minimum((0.14 - t) / 0.02, 1)
signal *= 0.45 / np.max(np.abs(signal))
path = Path(__file__).resolve().parents[1] / 'SourceArt/Audio/SFX/EnemyShot.wav'
with wave.open(str(path), 'wb') as output:
    output.setnchannels(1)
    output.setsampwidth(2)
    output.setframerate(rate)
    output.writeframes((signal * 32767).astype('<i2').tobytes())
print('ENEMY_SHOT_GENERATED', path)
