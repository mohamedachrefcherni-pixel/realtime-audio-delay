# Real-Time Audio Delay (C++ / PortAudio / ASIO)

A small Windows program that plays microphone input back through headphones after an adjustable delay (0 to 3 seconds), using a circular buffer and low-latency ASIO audio.

**Author:** Mohamed Achref Cherni

---

## 1. What the program does

The program takes the sound from a microphone and plays it back in the headphones after a delay chosen by the user (for example 500 ms, 1000 ms or 2000 ms).

- Language: C++ (Windows)
- Audio library: PortAudio, with the **ASIO** driver model (low latency)
- Method: a circular buffer (delay line). Each new sample is written into the buffer, and the sample from `delay` samples earlier is read and sent to the output.
- Input: 1 channel (mono). Output: 2 channels (the same signal on left and right).
- Maximum artificial delay: **3 seconds**.

> **Important:** the total delay heard by the user is the **artificial delay + the hardware delay** of the system (ASIO buffers, converters, headset). The program only controls the first part. See section 5.

---

## 2. Requirements

### 2.1 Computer
- Windows PC (the program was developed and tested on Windows).
- Any modern CPU is enough. The processing is very light.

### 2.2 Audio hardware
- A **wired headset** with microphone (tested headphones: **AKG K52**, chosen because its analog latency is negligible), or
- An audio interface with its own ASIO driver, with a microphone and headphones connected to it.

### 2.3 Software
- An **ASIO driver** installed on the PC:
  - **ASIO4ALL v2** (free, works with most sound cards and headsets), or
  - the manufacturer's ASIO driver of the audio interface (usually better and lower latency).
- The program built with **PortAudio compiled with ASIO support** (this needs the Steinberg ASIO SDK at build time). If PortAudio is built without ASIO, no ASIO device will appear in the list.
- Files needed in the project: `portaudio.h`, `pa_asio.h`, and the PortAudio library linked to the executable.

### 2.4 Build from source

You need CMake 4.0 or newer, a C++20 compiler (MSVC or MinGW), and the Steinberg ASIO SDK. The SDK is available from Steinberg and has its own license terms, so it is **not included** in this repository. PortAudio is included as a Git submodule.

```bash
git clone --recurse-submodules https://github.com/mohamedachrefcherni-pixel/realtime-audio-delay.git
cd realtime-audio-delay
cmake -B build -DASIO_SDK_DIR="C:/path/to/ASIO-SDK"
cmake --build build --config Release
```

The executable `audio_delay.exe` is created in the `build` folder (in `build/Release` with Visual Studio).

In CLion, open the folder and add `-DASIO_SDK_DIR=C:/path/to/ASIO-SDK` under **Settings → Build, Execution, Deployment → CMake → CMake options**, then reload the CMake project.

If CMake stops with "Set ASIO_SDK_DIR", the path to the ASIO SDK was not given or is wrong.

---

## 3. Setup before running (do this once per headset / per PC)

1. Plug the headset (or audio interface) in **before** starting the program.
2. Close every other program that uses the sound card (browser, Teams, Zoom, music player, DAW). An ASIO driver is usually used by **one program at a time**.
3. Open the ASIO driver control panel (for ASIO4ALL: the tray icon, bottom right of the screen).
4. In the panel:
   - Turn **ON only** the headset (its microphone and its output).
   - Turn **OFF** all other devices (laptop microphone, speakers, webcam mic, HDMI audio...).
   - Set the buffer size (see section 5). Start with a medium value, then reduce it.
5. Lower the Windows volume and the headset volume to a **low level** before the first test. There is no gain control or limiter in the program.
6. Put the headphones on **and keep the microphone away from the earcups** (see section 7, feedback).

> **Why only one device ON?**
> The program uses input channel 0 and output channels 0 and 1. In ASIO4ALL, these are the *first active* channels. If another device is active before the headset, the program may take the wrong microphone or send sound to the wrong output.

---

## 4. Running the program (step by step)

1. Start the program. It prints the list of audio devices:

   ```
   Available devices:
   0: Microsoft Sound Mapper - Input (MME)  in=2 out=0
   ...
   5: ASIO4ALL v2 (ASIO)  in=2 out=2
   ```

   *(The numbers above are only an example. They are different on each PC and they can change when a device is plugged or unplugged. **Always read the list again.**)*

2. **Enter the input device index.** Choose a line that ends with **(ASIO)**.
3. **Enter the output device index.** In ASIO, one driver is one device with both input and output, so type **the same number** as for the input.
4. **Enter the artificial delay in ms** (for example `500`). Allowed range: 0 to 3000 ms.
5. The ASIO control panel opens. You can check the devices and the buffer size here. If you change something, **stop the program (Enter) and restart it** so the new settings are used.
6. The stream starts and the program prints:
   `Streaming with ~X ms artificial delay (plus small hardware latency)... press Enter to stop.`
7. Speak into the microphone. You hear your voice delayed.
8. Press **Enter** to stop the program.

To change the delay, stop the program and start it again. The delay cannot be changed while the stream is running.

---

## 5. Delay: what you really get

```
Total delay = artificial delay (set in the program) + hardware delay
```

The hardware delay comes from the ASIO input buffer, the ASIO output buffer, the converters (ADC/DAC) and the headset. It depends on the **driver, the buffer size and the equipment**. It is not the same on two different setups.

### 5.1 Buffer size
- A **small buffer** gives low hardware delay, but may cause clicks and dropouts.
- A **large buffer** is more stable, but adds delay.
- Use the smallest buffer that gives clean sound.
- The program asks for 64 frames per buffer. If the driver cannot do 64, the opening of the stream may fail or the driver may use its own size. In this case, set the size in the driver panel.

### 5.2 Sample rate
- The program is fixed at **44100 Hz**.
- The driver and the device must accept 44100 Hz. If your device only works at 48000 Hz, change `double sampleRate = 44100;` to `48000` in the code and rebuild.
- The delay in seconds stays correct because it is computed from the sample rate.

### 5.3 How to measure the real delay (recommended after every change of equipment)
1. Run the program with an artificial delay of **0 ms**.
2. Record a test signal (click or noise burst) at the input and the signal heard at the output (loopback: output wired back to an input, or the headphone signal recorded).
3. Compute the delay between the two signals by **cross-correlation** (Knapp and Carter, 1976). Example in MATLAB:

   ```matlab
   % x(:,1) = reference signal, x(:,2) = signal after the system
   [c, lags] = xcorr(x(:,2), x(:,1));
   [~, i] = max(abs(c));
   hardware_delay_ms = 1000 * lags(i) / fs;
   ```

4. The result is the **hardware delay** of this setup.
5. To get a target total delay, set: `artificial delay = target − hardware delay`.

---

## 6. If you change the equipment

**After any change: restart the program, read the device list again, and measure the delay again (section 5.3).**

| What changes | What to do | What to check |
|---|---|---|
| **Same type of wired headset** (analog, 3.5 mm) | Plug it in, restart the program. | Mic and headphone levels. Re-measure the delay. |
| **Different wired headset** (other model) | Same as above. | Microphone sensitivity (volume of the voice). Headphone volume. Closed-back headphones are needed (section 7). |
| **USB headset** | It appears as a new device in the ASIO panel. Turn it ON, turn the others OFF. | It may have **1 output only** or **1 input only**, or a different sample rate. USB adds its own hardware delay. Re-measure. |
| **Bluetooth headset** | **Not recommended.** | Bluetooth adds a large and variable delay (often 100 ms or more) and ASIO4ALL cannot control it. The delay cannot be trusted. |
| **Audio interface with its own ASIO driver** | Install the manufacturer's driver. Select it in the device list instead of ASIO4ALL. | Choose the interface in its own control panel. The device name and index are different. Sample rate and buffer size are set in its panel. |
| **Different computer** | Install the ASIO driver. Build or copy the program with the same PortAudio (with ASIO). | The device list and the indexes will be different. Re-measure the delay. |
| **Different microphone position** | Keep the mic near the mouth and away from the earcups. | Voice level and feedback. |

### Notes about headsets
- **Latency:** a wired analog headset adds almost no delay. Digital or wireless links (USB, Bluetooth) add delay.
- **Isolation:** the user also hears their own voice directly through the air and through bone conduction. A closed-back headphone with good isolation reduces the direct sound, so the delayed sound is easier to hear. Open-back headphones leak more.
- **Volume:** a more sensitive headphone or microphone gives a louder result. Always start with a low volume.

---

## 7. Troubleshooting

| Problem or message | Probable cause | Solution |
|---|---|---|
| No ASIO device in the list | ASIO driver not installed, or PortAudio built without ASIO. | Install ASIO4ALL or the interface driver. Use a PortAudio build with ASIO. |
| `Pa_OpenStream failed: Invalid sample rate` | The device does not accept 44100 Hz. | Change `sampleRate` in the code (for example 48000) or set the rate in the driver panel. |
| `Pa_OpenStream failed: Invalid number of channels` | The device has no mono input or no stereo output (many headsets have 1 mic and 2 outputs, some have 1 output only). | Check the `in=` and `out=` values in the list. Make sure the right device is ON in the ASIO panel. If needed, change `channelCount` in the code. |
| `Pa_OpenStream failed: Bad I/O device combination` | Input and output indexes are different, or they are from different driver types. | Use the **same ASIO device index** for input and output. |
| `Pa_OpenStream failed: Device unavailable` or `Unanticipated host error` | Another program is using the ASIO driver, or the device was unplugged. | Close other audio programs. Re-plug the headset. Restart the program. |
| `Pa_OpenStream failed: Buffer too small / too big` | The buffer size of 64 is not accepted by the driver. | Change the buffer size in the driver panel. |
| No sound at all | Wrong device chosen, wrong channel (another device is first in the ASIO panel), volume at zero, or the headset is muted. | Turn OFF the other devices in the panel. Check the volume. Restart the program. |
| Clicks, crackles, dropouts | The ASIO buffer is too small for this PC. | Increase the buffer size in the driver panel. Close heavy programs. |
| A whistle or loud feedback | The microphone picks up the headphone sound. | Lower the volume. Move the mic away from the earcups. Use closed-back headphones. **Remove the headphones if it starts.** |
| The delay seems longer than the value entered | Hardware delay is added. | Measure it (section 5.3) and reduce the artificial delay. |
| The delay is shorter than the value entered (above 3 s) | The program limits the delay to 3 seconds without any message. | Use 3000 ms or less. |
| The input device index is not the same as last time | Indexes change when devices are plugged or unplugged. | Always read the list before entering the number. |

---

## 8. Known limitations

- Windows and ASIO only. Input and output must be on the same ASIO device.
- The delay is set **before** the stream starts. It cannot be changed while running.
- Maximum delay: 3 seconds. A larger value is silently reduced to 3 s, and the message printed at the start still shows the value that was typed.
- Input is mono and output is stereo (same signal on both ears).
- Sample rate is fixed in the code (44100 Hz).
- No volume control, no limiter, no feedback protection.
- Device selection is done by typing the index, and the program does not check that the device is an ASIO one.
- The program does not report dropouts during the stream.

---

## License

MIT. See the `LICENSE` file.
