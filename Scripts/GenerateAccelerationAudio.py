"""Original short rising whoosh for entering a ballistic acceleration field."""
from pathlib import Path
import numpy as np
import wave

rate = 48000
t = np.arange(int(rate * 0.20)) / rate
phase = 2 * np.pi * (300 * t + 2200 * t*t)
noise = np.random.default_rng(261016).normal(0,1,len(t))
noise = np.convolve(noise, np.ones(12)/12, mode='same')
signal = (0.7*np.sin(phase) + 0.35*noise) * np.sin(np.pi*t/0.20)**2
signal *= 0.4 / np.max(np.abs(signal))
path = Path(__file__).resolve().parents[1] / 'SourceArt/Audio/SFX/Acceleration.wav'
with wave.open(str(path), 'wb') as output:
    output.setnchannels(1)
    output.setsampwidth(2)
    output.setframerate(rate)
    output.writeframes((signal*32767).astype('<i2').tobytes())
print('ACCELERATION_AUDIO_GENERATED', path)
