**Microsoft Windows (32-bit) Bean Cash Daemon Build Instructions**

See readme-qt.rst for instructions on building Beancash-qt, the
graphical user interface.

WINDOWS BUILD NOTES
===================

Compilers Supported
-------------------
TODO: What works?
Note: releases are cross-compiled using mingw running on Windows and Linux.


Dependencies
------------
Libraries you need to download separately and build:


+------------+----------------------+-----------------------------------------------------------------------------+
| Dependency | Default path         | Download link                                                               |
+============+======================+=============================================================================+
| OpenSSL    | \openssl-1.0.2t-mgw  | http://www.openssl.org/source/                                              |
+------------+----------------------+-----------------------------------------------------------------------------+
| Berkeley DB| \db-5.3.28NC-mgw     | http://www.oracle.com/technology/software/products/berkeley-db/index.html   |
+------------+----------------------+-----------------------------------------------------------------------------+
| Boost      | \boost-1.59.0.1-mgw  | http://www.boost.org/users/download/                                        |
+------------+----------------------+-----------------------------------------------------------------------------+
| Protobuf   | \protobuf-2.5.0-mgw  | https://github.com/protocolbuffers/protobuf/releases/tag/v2.5.0             |
+------------+----------------------+-----------------------------------------------------------------------------+


Their licenses:
~~~~~~~~~~~~~~~

OpenSSL:

 Old BSD license with the problematic advertising requirement.
 
Berkeley DB:
 New BSD license with additional requirement that linked software must be free open source.
 
Boost:
 MIT-like license.
 
Protobuf:
 BSD


Versions used in this release:
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

 OpenSSL      1.0.2t
 
 Berkeley DB  5.3.28NC
 
 Boost        1.59.0
 
 Protobuf     2.5.0


OpenSSL
-------
MSYS shell:

  *un-tar sources with MSYS 'tar xfz' to avoid issue with symlinks (OpenSSL ticket 2377)
  change 'MAKE' env. variable from 'C:\MinGW32\bin\mingw32-make.exe' to '/c/MinGW32/bin/mingw32-make.exe'*

::

  cd /c/openssl-1.0.2t-mgw
  ./config
  make

Berkeley DB
-----------
MSYS shell:

::

  cd /c/db-5.3.28NC-mgw/build_unix
  sh ../dist/configure --enable-mingw --enable-cxx
  make

Boost
-----
DOS prompt:

::

  cd \boost-1.58.0.1-mgw
  toolset=gcc --build-type=complete stage


Bean Cash
---------
DOS prompt:

::

  cd \BeanCash\src
  mingw32-make -f makefile.mingw
  strip Beancashd.exe
