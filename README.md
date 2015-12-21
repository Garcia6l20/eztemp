# EZTemp

Twig like templating library and tools.

## Requirements

This library is on top of the *boost* library.

On debian-based distro:

```bash
sudo apt-get install \
    libboost-program-options-dev \
    libboost-python-dev \
    libboost-regex-dev \
    libboost-date-time-dev
```

## Installation

```bash
git clone https://github.com/Garcia6l20/eztemp
cd eztemp
mkdir build
cd build
cmake ..
make

# run some tests
make check

# install
sudo make install
```

