# LCG
LCG is a suite of command-line programs to perform electrophysiological experiments.

## Installation
Detailed instructions about the installation of LCG can be found
[here](http://danielelinaro.github.io/dynclamp/installation.html)

In a nutshell, if you have a system with at least Comedi, you can
install LCG by typing the following commands:
```
autoreconf -i
./configure --prefix=$HOME/local
make
make install
```
We strongly recommend to install LCG in a local directory (for example in the user's
home directory) instead of system-wide.

An additional option that you may want to pass to configure is --with-output-reset,
which instructs the program to always reset the output of the DAQ card to zero whenever
the program terminates.

Additional Python scripts can be installed by typing:
```
cd python
python setup.py build
python setyp.py install --prefix=$HOME/local
```

## Usage
LCG consists of a main executable, called lcg, which takes as arguments
commands and related options.

A list of available commands can be obtained by typing
```
lcg help
```

To display information about each command, type:
```
lcg help command
lcg command -h
```

### Version
0.9

### License
GPL

### Authors
Daniele Linaro - danielelinaro@gmail.com   
Joao Couto - jpcouto@gmail.com

