
# sound-analyser-for-LEDs

## Table of Contents
 - [Introduction](#introduction) 
 - [Features](#features) 
 - [Installation](#installation)
 - [Usage](#usage)
 - [Contributing](#contributing)
 - [License](#license)
 - [Contact](#contact)
 
 
## Introduction
This project is for displaying the sound output of your device on an external LED-strip, controlled by e.g. an ESP32 (the [corresponding github-repository](https://github.com/AlmTheHedgehog/ESP32-WS2811-Bluetooth-Controller) for this).

## Features
- Connect an external device e.g. ESP32 via bluetooth, LAN or WIFI
- It analyzes the sound output of your computer and sends the data to the external device

## Installation

This project depends on different external packets. The installation-guide is tested on ubuntu 22.04.

### Installation of portaudio and standard tools
```sh
    sudo apt-get update
    sudo apt-get install -y gcc make portaudio19-dev libbluetooth-dev bluez valgrind git
```
### Cloning the github-repository
```sh
    git clone --depth 1 https://github.com/AlmTheHedgehog/sound-analyser-for-LEDs.git
    cd sound-analyser-for-LEDs
```

### Installation of FFTW3

Download the newest version from the [official website](http://fftw.org/download.html) and extract the files into include/fftw3. 
Then run the following commands
```sh
	cd sound-analyser-for-LEDs && mkdir include
	cd include
	./config 
	make
	make install
```
  
### Expected folder structure

    sound-analyzer 
    ├── build 
    ├── include 
    │   └── fftw3 
    ├── src 
    ├── Makefile


## Usage
### Makefile options

The command `make all` builds the executable into the build/-directory.

The command `make clean` deletes the build/-directory.

The command `make check` builds the application and runs valgrind.

The command `make run` builds the application and runs it.

## Contributing
1. Fork this repository
2. Clone the forked repository
3. Create a new branch `git checkout -b feature-branch`
4. Commit `git commit -m "Added feature-name"`
5. Push to the branch `git push origin feature-branch`
6. Open a pull request

## License
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)


## Contact
### Owner
[AImTheHedgehog](https://github.com/AlmTheHedgehog) 

### Contributers
[JuliusSDev](https://github.com/JuliusSDev)

