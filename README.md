# dll
Implements a delay-locked-loop as an MHA plugin as described in

Fons Adriensen: Using a DLL to filter time. 2005

http://kokkinizita.linuxaudio.org/papers/usingdll.pdf

Fons writes in the paper that this DLL algorithm is already implemented in Jack. 
For scenarios where Jack is too much overhead, and direct alsa usage is the 
better choice, I implemented my own version of that DLL algorithm from the paper.

Instead of writing the alsa interface code on my own, I implement the DLL as a
plugin for the open-source software openMHA. 

The plugin `dll` implements the DLL time filtering, the plugin `metronome` is a
simple test plugin that uses the DLL to implement a metronome.

# Compile for ARM Linux: Debian Buster

I'm going to use precompiled debian packages from the openMHA project.
openMHA has no Debian packages for Debian Buster specifically, but their
Debian package for Ubuntu Bionic will work on Debian Buster.  Add

```
deb [arch=armhf] http://apt.hoertech.de bionic universe
```
to `/etc/apt/sources.list`, run
```
wget -qO- https://apt.hoertech.de/openmha-packaging.pub | sudo apt-key add
sudo apt update
sudo apt install --no-install-recommends openmha libopenmha-dev g++-7 make git cmake 
git clone https://github.com/tobiasherzke/dll
cd dll
make; make unit-tests
```
This generates files `dll.so` and `metronome.so` and executes some
unit tests for the `dll` plugin.

# Install on ARM Linux: Debian Buster
Copy files `dll.so` and `metronome.so` to `/usr/lib/` as root.

No need to recompile on every ARM device, just transfer these two files to
every ARM device where they are needed, e.g. by scp.
Other ARM devices do not need the compiler etc, just
add the line to `etc/apt/sources` as above and install only the openmha base package:
```
wget -qO- https://apt.hoertech.de/openmha-packaging.pub | sudo apt-key add
sudo apt update
sudo apt install openmha
```

# Execute
Load the plugins into an openMHA alsa config.  E.g.
```
mha fragsize=96 srate=44100 nchannels_in=2 iolib=MHAIOalsa \
  io.in.device=hw:0 io.out.device=hw:0 mhalib=mhachain \
  mha.algos='[dll metronome]' mha.metronome.bpm=60 \
  io.format=S16_LE io.priority=55 cmd=start sleep=11
```
Adapt to your own requirements.  Remember to enable realtime priorities
for the user executing the openMHA, e.g. with 
```
cat /etc/security/limits.d/audio.conf

@audio   -  rtprio     95
@audio   -  memlock    unlimited
```
