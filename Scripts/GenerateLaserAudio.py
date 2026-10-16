"""Generate original deterministic 48 kHz mono laser charge/fire source WAVs."""
from pathlib import Path
import wave
import numpy as np

root = Path(__file__).resolve().parents[1] / 'SourceArt/Audio/SFX/Laser'
root.mkdir(parents=True, exist_ok=True)
rate = 48000
rng = np.random.default_rng(261015)

def save(name, samples, peak):
    samples = samples / max(float(np.max(np.abs(samples))), 1e-9) * peak
    with wave.open(str(root / name), 'wb') as output:
        output.setnchannels(1)
        output.setsampwidth(2)
        output.setframerate(rate)
        output.writeframes((samples * 32767).astype('<i2').tobytes())

t = np.arange(rate * 2) / rate
phase = 2 * np.pi * (160 * t + 180 * t**2)
envelope = np.minimum(t / 0.04, 1) * (0.15 + 0.85 * (t / 2)**1.5)
envelope *= np.minimum((2 - t) / 0.015, 1)
charge = (np.sin(phase) + 0.22 * np.sin(phase * 2.01)) * envelope
charge *= 0.85 + 0.15 * np.sin(2 * np.pi * (8 * t + 6 * t**2))
save('LaserCharge.wav', charge, 0.42)

t = np.arange(int(rate * 0.42)) / rate
phase = 2 * np.pi * (105 * t + 650 * 0.025 * (1 - np.exp(-t / 0.025)))
noise = rng.normal(0, 1, len(t))
noise = np.convolve(noise, np.ones(9) / 9, mode='same')
fire = (np.sin(phase) * 0.75 + np.sin(phase * 2.7) * 0.18 + noise * 0.7)
fire *= np.minimum(t / 0.003, 1) * np.exp(-t / 0.085) * np.minimum((0.42 - t) / 0.025, 1)
save('LaserFire.wav', fire, 0.63)
print('LASER_AUDIO_GENERATED', root)
