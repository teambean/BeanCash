**Mac OS X Bean Cash Daemon Build Instructions**

See readme-qt.rst for instructions on building Beancash-qt, the
graphical user interface.



Mac OS X Build Notes
====================


Tested on 10.5 and 10.6 intel.  PPC is not supported because it's big-endian.

All of the commands should be executed via the terminal application located in:

::

/Applications/Utilities

You need to install XCode with all the options checked so that the compiler and
everything is available in */usr* not just */Developer*. 

XCode is available from https://developer.apple.com


1.  Clone the github tree to get the source code:

::

  git clone https://github.com/teambean/BeanCash

2.  Download and install MacPorts from https://www.macports.org

  2a. (for 10.7 Lion)
    *Edit /opt/local/etc/macports/macports.conf and uncomment "build_arch i386"*

3.  Install dependencies from MacPorts

::

  sudo port install boost db53 openssl

    3a. Optionally install qrencode (and set USE_QRCODE=1):

::

      sudo port install qrencode

4.  Now you should be able to build Beancashd:

::

  cd Beancash/src
  make -f makefile.osx

Run:

::

  ./beancashd -help  # for a list of command-line options.
  ./beancashd -daemon # to start the Bean Cash daemon.
  ./beancashd help # When the daemon is running, to get a list of RPC commands.


  
Copyright (c) 2009-2011 Satoshi Nakomoto
Copyright (c) 2011-2013 Bitcoin Developers
Copyright (c) 2015 Nokat
Copyright (c) 2015-2017 Bean Core www.bitbean.org
Copyright (c) 2015-2019 Bean Core www.beancash.org

Distributed under the MIT/X11 software license, see the accompanying file license.txt or http://www.opensource.org/licenses/mit-license.php. 

This product includes software developed by the OpenSSL Project for use in the OpenSSL Toolkit (http://www.openssl.org/).  This product includes cryptographic software written by Eric Young (eay@cryptsoft.com).
