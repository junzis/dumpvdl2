# NEWS

## Version 1.7.0 (2019-08-11):
* Complete overhaul of output generation code. Messages are no longer printed
  directly into the output file, but rather stored in memory in a structured
  manner and serialized into an output string (still in memory), which is then
  printed into an output file. This change does not bring significant benefits
  or features yet, but is a major step towards multi-output, multi-format
  architecture in the next release. A side effect of this is more concise
  output formatting (indentation). The purpose of this is to better visualise
  message hierarchy (layered structure). **Note**: libacars version 1.3.0
  or later is now required to compile and run dumpvdl2.
* Significant enhancements to CLNP, COTP and IDRP decoders. Most if not all
  protocol headers are now decoded and included in the output.
* Added a decoder for X.225 Session Protocol SPDUs.
* Added a decoder for VDL SNDCF error reports.
* XID: improved decoding of XID sequencing, Frequency support and System mask
  attributes.
* X.25: improved decoding of facility fields, call clearing causes and
  diagnostic codes.
* Minor bug fixes.

## Version 1.6.0 (2019-01-19):
* New build system based on cmake. Refer to README.md for new installation
  instructions.
* FANS/1-A CPDLC, ADS-C and ACARS decoders have been removed. These features
  have been moved to libacars library, which is now a mandatory dependency
  (compilation will fail if libacars is not installed). Refer to README.md for
  details.
* Added support for SoapySDR library (contributed by Fabrice Crohas). However
  there are still some limitations with respect to SDR types which dumpvdl2
  can work with. As of now, the device must support a sampling rate of 2100000
  samples per second. This will be addressed in a future release.
* The program should now compile and run on MacOS, however this hasn't been
  tested well. Feedback and bug reports are welcome.
* Minor bugfixes

## Version 1.5.0 (2018-11-17):
* Sync with the latest SDRPlay API v. 2.13 which is now required to compile the
  program
* Added support for SDRPlay RSPduo (with tuner selection, bias-T and notch
  filters)
* Added support for RSP1A features: Bias-T and broadcast notch filter
* Minor bugfixes

## Version 1.4.0 (2018-08-01):
* Major improvements to the DSP and decoding engine - gives much better decoding
  success rate, especially for weak transmissions from distant aircraft.
* Each VDL2 channel is now processed in a separate thread, which allows
  balancing the load onto multiple CPU cores.
* Fixed a nasty decoder bug which caused good frames to be corrupted during
  decoding stage and therefore missed.
* Added CRC checking for ACARS messages. If there is a CRC mismatch, the message
  will still be logged, but with an appropriate warning.
* New StatsD metric `avlc.msg.air2air` - counts messages from aircraft to
  aircraft (however strange it seems, such things happen in real life).
* New StatsD metric `decoder.errors.too_long` - counts VDL2 bursts dropped due
  to unreasonably large value in the length field.
* Due to changes in the decoding engine, the following StatsD metric are now
  obsolete and have been removed: `avlc.errors.no_flag_start`,
  `avlc.errors.no_flag_end`, `decoder.errors.no_preamble`.
* Added `--extended-header` command line option which causes a few decoder
  diagnostic parameters to be logged for each message.  Refer to the FAQ in
  README.md for description of the fields.
* Other minor bugfixes and improvements.
* C11-capable compiler is now required to build the program.

## Version 1.3.1 (2018-05-27):
* Added `decpdlc` utility which decodes FANS-1/A CPDLC messages supplied from
  command line or from a file. Can be used to decode CPDLC traffic received via
  other media than VDL-2 (eg. ACARS, HFDL, SATCOM). Refer to "decpdlc" section in
  README.md for details.

## Version 1.3.0 (2018-04-11):
* Added decoder for FANS-1/A ADS-C messages
* Added decoder for FANS-1/A CPDLC messages
* Human-readable formatting of CPDLC and Context Management messages (if you
  still want the old format, add `--dump-asn1` command line option)
* Added install rule to Makefile
* Added a sample systemd service file
* Bug fixes

## Version 1.2.0 (2018-01-01):
* Better input signal filtering - reduced bit error rate, 10-50% more messages
  successfully decoded.
* SDRPlay: major bugfixes in sample buffer handling code - shall now perform
  equally well as RTLSDR (or even better).
* SDRPlay: --gain knob replaced with --gr (gain reduction). This makes gain
  configuration in dumpvdl2 compatible with other SDRPlay apps. See README.md
  for details.
* SDRPlay: support for selecting device by serial number.
* Added optional build-time PLATFORM knob which sets CPU-specific compiler flags
  to optimize the code for various flavours of Raspberry Pi. Supported values:
  rpiv1, rpiv2, rpiv3.
* Other minor bug fixes

## Version 1.1.0 (2017-06-20):
* Support for SDRPlay RSP1 and 2 using native binary API
* Added dissector for ISO 8073/X.224 Connection-oriented Transport Protocol
* Added dissectors for ICAO Context Management and CPDLC
* Enhancements for ES-IS and XID dissectors
* Support for referring to dongles using their serial numbers
* Output formatting fixes
* Bug fixes

## Version 1.0.0 (2017-02-26):
* First public release

// vim: textwidth=80
