# Intro

This git repository contains a small number of openMHA plugins (in source
code) that implement and make use of mapping sound samples to wall clock
time.

Implementation of these algorithms as openMHA plugins was done because
* it is easier to write openMHA plugins than it is to write the alsa or Jack applications, and
* openMHA plugins can be easily combined with other plugins.

The plugins from this repository should only be used in openMHA
configurations which make use of a low-level sound card interface, e.g.
MHAIOalsa.

# Plugin "`dll`"
Implements a delay-locked-loop as an MHA plugin as described in

Fons Adriensen: Using a DLL to filter time. 2005

http://kokkinizita.linuxaudio.org/papers/usingdll.pdf

Fons writes in the paper that this DLL algorithm is already implemented in Jack.

For scenarios where Jack is too much overhead, and direct alsa usage is the
better choice, I implemented my own version of that DLL algorithm from the
paper.

The plugin `dll` retrieves time stamp from the computer's clock, filters
these time stamps with a DLL, and publishes the filtered time stamps as AC
variables.

# Plugin "`timestamper`"

Retrieves current time on each processing callback and publishes the time
stamp as an AC variable. Similar in behaviour as plugin `dll` but does not
perform any filtering on the time stamps before publishing.

# Plugin "`metronome`"
The plugin `metronome` is a simple test plugin that uses the filtered time
stamps from the DLL to implement a metronome. Having multiple instances of
openMHA each running the `metronome` plugin with identical configuration, and
each driven by a `dll` that filters an NTP controlled clock will lead to
synchronized metronome sounds across all instances.

# Plugins "`wav2lsl`" and "`lsl2wav`"
These plugins transmit the openMHA audio stream over the network as an LSL
stream, or vice versa receive an audio stream via LSL from the network and
replace the openMHA audio stream with data from the network stream.

These plugins need a `dll` plugin upstream that tags sound samples with
time stamps.

Resampling can be improved, currently only does nearest-neighbor lookup.

# Compile for ARM Linux: Debian Buster

I'm using precompiled debian packages from the openMHA project.
openMHA has no Debian packages for Debian Buster specifically, but their
Debian package for Ubuntu Bionic will work on Debian Buster.  Add

```
deb [arch=armhf] http://apt.hoertech.de bionic universe
```
to `/etc/apt/sources.list`, run
```
wget -qO- http://apt.hoertech.de/openmha-packaging.pub | sudo apt-key add
sudo apt update
sudo apt install --no-install-recommends openmha libopenmha-dev g++-7 make git cmake
git clone https://github.com/tobiasherzke/dll
cd dll
make; make unit-tests
```
This generates plugin files `dll.so`, `lsl2wav.so`, `metronome.so`,
`timestamper.so`,  and `wav2lsl.so`, and executes some unit tests for the
`dll` plugin.

# Install on ARM Linux: Debian Buster
Copy all generated `*.so` files to `/usr/lib/` as root.

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
