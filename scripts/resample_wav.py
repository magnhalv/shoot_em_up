from pydub import AudioSegment

# Load the WAV file
audio = AudioSegment.from_file("assets/sound/explosion4.wav")

# Resample to 48,000 Hz
resampled_audio = audio.set_frame_rate(48000)
#resampled_audio = audio.set_sample_width(2)

# Export the resampled audio
resampled_audio.export("output.wav", format="wav")
