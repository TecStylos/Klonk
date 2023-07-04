# Klonk

Klonk is a clock and Spotify integration for a Raspberry Pi.

## Getting Started

### Requirements
 - [Raspberry Pi](https://www.raspberrypi.org/)
 - [320x240 touchscreen](https://joy-it.net/en/products/RB-TFT3.2V2)
 - [Spotify Premium Account](https://www.spotify.com/us/premium/)
 - [Spotify Application](https://developer.spotify.com/dashboard/applications)
 - CMake
 - C++17 compiler

### Installation

#### Clone the repository onto your Raspberry Pi
```base
git clone https://github.com/TecStylos/klonk.git
```

#### Build the project
```bash
./build.sh [configuration]
```

Possible configurations are:
 - Release
 - Debug
 - RelWithDebInfo

Binaries are placed in the `bin/[configuration]` directory.

Add it to `rc.local` to start it on boot.

#### Configure spotify

Place your client-id and client-secret in the `data/client_id.txt` and `data/client_secret.txt` files respectively.

When you are done, run `./bin/[configuration]/klonk` to start the application.
The first time you run it, you will be asked to authorize the application.
The authorization process requires a browser to log in with your Spotify account. This can be done using a VNC-Server.