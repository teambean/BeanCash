**Unix/Linux Bean Cash Daemon Build Instructions**

See readme-qt.rst for instructions on building Beancash-qt, the
graphical user interface.


UNIX BUILD NOTES
================

To Build
--------

::

  cd src/
  make -f makefile.unix            # Headless Beancash daemon

See readme-qt.rst for instructions on building Beancash QT,
the graphical Vault.

Dependencies
------------

+------------+------------------+-------------------------------+
| Library    | Purpose          | Description                   |
+============+==================+===============================+
| libssl     | SSL Support      | Secure communications         |
+------------+------------------+-------------------------------+
| libdb      | Berkeley DB      | Blockchain & wallet storage   |
+------------+------------------+-------------------------------+
| libboost   | Boost            | C++ Library                   |
+------------+------------------+-------------------------------+
| libprotobuf| Communication    | Serialized Communication      |
+------------+------------------+-------------------------------+


*Note that libexecinfo should be installed, if you building under *BSD systems. 
This library provides backtrace facility.*

libqrencode may be used for QRCode image generation. It can be downloaded
from http://fukuchi.org/works/qrencode/index.html.en, or installed via
your package manager. Set USE_QRCODE to control this:

::

 USE_QRCODE=0   (the default) No QRCode support - libqrcode not required
 USE_QRCODE=1   QRCode support enabled

Licenses of statically linked libraries:
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

 Berkeley DB:
    New BSD license with additional requirement that linked software must be free open source
    
 Boost:
    MIT-like license
    
 Protobuf:
    BSD

Versions used in this release:
 + GCC           4.9.0
 + OpenSSL       1.0.2t
 + Berkeley DB   5.3.28NC
 + Boost         1.59.0
 + Protobuf      2.5.0


Dependency Build Instructions: Ubuntu & Debian
----------------------------------------------
::

  sudo apt-get install build-essential
  sudo apt-get install libssl-dev
  sudo apt-get install libdb++-dev
  sudo apt-get install libboost-all-dev
  sudo apt-get install libprotobuf-dev
  sudo apt-get install protobuf-compiler



*If using Boost 1.37, append -mt to the boost libraries in the makefile.*


Dependency Build Instructions: Gentoo
-------------------------------------
::

  emerge -av1 --noreplace boost openssl sys-libs/db

Take the following steps to build:
::

 cd ${BeanCash_DIR}/src
 make -f makefile.unix
 strip beancashd


Notes
~~~~~
*The release is built with GCC and then "strip Beancashd" to strip the debug
symbols, which reduces the executable size by about 90%.*



Berkeley DB
~~~~~~~~~~~

You need Berkeley DB. If you have to build Berkeley DB yourself:
::

  ../dist/configure --enable-cxx
  make


Boost
~~~~~

If you need to build Boost yourself:
::

  sudo su
  ./bootstrap.sh
  ./bjam install


Security
--------
To help make your Beancash installation more secure by making certain attacks impossible to exploit even if a vulnerability is found, you can take the following measures:

Position Independent Executable
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    Build position independent code to take advantage of Address Space Layout Randomization offered by some kernels. An attacker who is able to cause execution of code at an arbitrary memory location is thwarted if he doesn't know where anything useful is located.
    
    The stack and heap are randomly located by default but this allows the code section to be randomly located as well.

    On an Amd64 processor where a library was not compiled with -fPIC, this will cause an error such as: 
    
      *"relocation R_X86_64_32 against `......' can not be used when making a shared object;"*

    To build with PIE, use:
    ::
    
      make -f makefile.unix ... -e PIE=1

    To test that you have built PIE executable, install scanelf, part of paxutils, and use:
    ::
    
      scanelf -e ./beancashd

    The output should contain:
    
     *TYPE*
     
     *ET_DYN*

Non-executable Stack
~~~~~~~~~~~~~~~~~~~~
    If the stack is executable then trivial stack based buffer overflow exploits are possible if vulnerable buffers are found. By default, Beancash should be built with a non-executable stack but if one of the libraries it uses asks for an executable stack or someone makes a mistake and uses a compiler extension which requires an executable stack, it will silently build an executable without the non-executable stack protection.

    To verify that the stack is non-executable after compiling use:
    ::
    
      scanelf -e ./beancashd

    The output should contain:
    
      *STK/REL/PTL*
      
      *RW- R-- RW*-

    *The STK RW- means that the stack is readable and writeable but not executable.*
 
------------------------------------------------------------------------------------
 
Copyright (c) 2009-2011 Satoshi Nakomoto
Copyright (c) 2011-2013 Bitcoin Developers
Copyright (c) 2015-2015 Nokat balthazar@yandex.ru
Copyright (c) 2015-2017 Bean Core www.bitbean.org
Copyright (c) 2015-2022 Bean Core www.beancash.org

Distributed under the MIT/X11 software license, see the accompanying file license.txt or http://www.opensource.org/licenses/mit-license.php. 

This product includes software developed by the OpenSSL Project for use in the OpenSSL Toolkit (http://www.openssl.org/).  This product includes cryptographic software written by Eric Young (eay@cryptsoft.com).

