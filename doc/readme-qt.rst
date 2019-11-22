Beancash-qt: Qt5 GUI for Beancash
=================================

Build instructions
===================

Debian (tested under Ubuntu 16.04 LTS - 25/03/2018)
---------------------------------------------------

- make sure to have the required packages:

::

        sudo apt-get update
        sudo apt-get upgrade
        sudo apt-get install qt5-default qt5-qmake qtbase5-dev-tools qttools5-dev-tools
        sudo apt-get install build-essential
	sudo apt-get install libdb5.3++-dev libssl-dev libboost-dev libboost-all-dev
	sudo apt-get install libprotobuf-dev protobuf-compiler

- now download and compile the three libraries: (update this when libraries changes)

.. _`OpenSSL 1.0.2t`: https://www.openssl.org/source/openssl-1.0.2t.tar.gz
.. _`Berkley DB 5.3.28.NC` : http://download.oracle.com/otn/berkeley-db/db-5.3.28.NC.zip
.. _`Boost 1.59.0` : http://sourceforge.net/projects/boost/files/boost/1.59.0/boost_1_59_0.tar.gz

    - boost : `Boost 1.59.0`_
    - db : `Berkley DB 5.3.28.NC`_
    - openssl : `OpenSSL 1.0.2t`_

- Prepare a directory path/to/libs (we'll refer to it as /libs) where you would like to store those libraries.

- Unzip all three archives inside /libs resulting in:

::

        libs/boost_1_59_0
        libs/db-5.3.28.NC
        libs/openssl-1.0.2t

- Now compile all the libraries.

    boost 1.59.0
    ::

        cd /libs/boost_1_59_0
        ./bootstrap.sh
        mkdir build-boost
        ./b2 --build-dir=/build-boost toolset=gcc stage

    berkley db
    ::

        cd /libs/db-5.3.28.NC
        cd build-unix
        ../dist/configure --enable-cxx
        make

    openssl1.0.2t
    ::

        cd /libs/openssl-1.0.2t
        ./config
        make

- In order to instruct the compiler to find the libraries modify some global variables insides Beancash-qt.pro

::

       BOOST_INCLUDE_PATH=/full/path/to/libs/boost_1_59_0
       BOOST_LIB_PATH=/full/path/to/libs/stage/lib

       OPENSSL_INCLUDE_PATH=/full/path/to/libs/openssl-1.0.2t/include
       OPENSSL_LIB_PATH=/full/path/to/libs/openssl-1.0.2t

       BDB_INCLUDE_PATH=/full/path/to/libs/db-5.3.28.NC/build_unix
       BDB_LIB_PATH=/full/path/to/libs/db-5.3.28.NC/build_unix

- Now we can compile the application.

::

    cd /path/beancash/
    qmake RELEASE=1 (other flags: USE_O3=1 to use compilation optimization, USE_DBUS=1 which build with notification support (enabled by default))
    make

- After compilation An executable named `Beancash-qt` will be built.


Windows
--------

Windows build instructions:

- Download the `QT Windows SDK`_ and install it. You don't need the Symbian stuff, just the desktop Qt.

- Compile openssl, boost and dbcxx.

- Copy the contents of the folder "deps" to "X:\\QtSDK\\mingw", replace X:\\ with the location where you installed the Qt SDK. Make sure that the contents of "deps\\include" end up in the current "include" directory.

- Open the .pro file in QT creator and build as normal (ctrl-B)

.. _`QT Windows SDK`: http://qt.nokia.com/downloads/sdk-windows-cpp


Mac OS X
--------

- Download and install the `Qt Mac OS X SDK`_. It is recommended to also install Apple's Xcode with UNIX tools.

- Download and install `MacPorts`_.

- Execute the following commands in a terminal to get the dependencies:

::

	sudo port selfupdate
	sudo port install boost db53

- Open the .pro file in Qt Creator and build as normal (cmd-B)

.. _`Qt Mac OS X SDK`: http://qt.nokia.com/downloads/sdk-mac-os-cpp
.. _`MacPorts`: http://www.macports.org/install.php


Build configuration options
============================

Notification support for recent (k)ubuntu versions
---------------------------------------------------

To see desktop notifications on (k)ubuntu versions starting from 10.04, enable usage of the
FreeDesktop notification interface through DBUS using the following qmake option:

::

    qmake "USE_DBUS=1"

Generation of QR codes
-----------------------

libqrencode may be used to generate QRCode images for payment requests. 
It can be downloaded from http://fukuchi.org/works/qrencode/index.html.en, or installed via your package manager. Pass the USE_QRCODE 
flag to qmake to control this:

+--------------+--------------------------------------------------------------------------+
| USE_QRCODE=0 | (the default) No QRCode support - libarcode not required                 |
+--------------+--------------------------------------------------------------------------+
| USE_QRCODE=1 | QRCode support enabled                                                   |
+--------------+--------------------------------------------------------------------------+


Berkely DB version warning
==========================

A warning for people using the *static binary* version of Beancash on a Linux/UNIX-ish system (tl;dr: **Berkely DB databases are not forward compatible**).

The static binary version of Beancash is linked against libdb 5.3.28.NC (see also `this Debian issue`_).

Now the nasty thing is that databases from 5.X are not compatible with 4.X.

If the globally installed development package of Berkely DB installed on your system is 5.X, any source you
build yourself will be linked against that. The first time you run with a 5.X version the database will be upgraded,
and 4.X cannot open the new format. This means that you cannot go back to the old statically linked version without
significant hassle!

.. _`this Debian issue`: http://bugs.debian.org/cgi-bin/bugreport.cgi?bug=621425
