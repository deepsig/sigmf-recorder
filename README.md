
# sigmf-recorder

a simple CLI tool for RF recording using [libsigmf](https://github.com/deepsig/libsigmf).
provided under the Apache License 2.0 and the copyright notice can be found in LICENSE.

## Compiling
```
git clone https://github.com/deepsig/sigmf-recorder.git
cd sigmf-recorder
git submodule update --init --recursive
mkdir build
cd build
cmake ..
make
```

## Usage

```
$ ./sigmf_recoder
record_sigmf program options::
  -o [ --output ] arg                output filename
  -f [ --freq ] arg                  center frequency (MHz) [required]
  -r [ --rate ] arg (=40)            sample rate (MHz)
  -w [ --bw ] arg (=40)              sample bandwidth (MHz)
  -g [ --gain ] arg (=45)            receive gain (dB)
  -n [ --samples ] arg (=0)          Number of samples to capture
  -s [ --seconds ] arg (=0)          Number of seconds to capture
  -b [ --bufsize ] arg (=2000)       Number of samples per buffer
  -d [ --device ] arg                USRP Device Args
  -v [ --subdev ] arg (=A:A)         USRP Subdevice
  -a [ --antenna ] arg (=RX2)        USRP Antenna Port
  -c [ --reference ] arg (=internal) Clock Reference
  -t [ --datatype ] arg (=16sc)      Data type
  -e [ --description ] arg           Description
  -N [ --shortname ] arg (=RF)       Short name in auto filename
  -j [ --showjson ]                  Only show JSON Example and Exit
  -h [ --help ]                      produce help message
```

## Contributing

please send github pull requests
