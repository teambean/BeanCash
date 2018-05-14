###Dependencies###
`apt-get install -y pkg-config
apt-get -y install build-essential autoconf automake libtool libboost-all-dev libgmp-dev libssl-dev libcurl4-openssl-dev git 
yes '' | sudo add-apt-repository ppa:bitcoin/bitcoin`
`apt-get update`
`apt-get -y install libdb4.8-dev libdb4.8++-dev`
`apt-get -y install fail2ban`
`apt-get -y install make g++ gcc cpp ngrep iftop sysstat autotools-dev unzip`
`apt-get -y install libqt5gui5 libqt5core5a libqt5dbus5 qttools5-dev qttools5-dev-tools libprotobuf-dev protobuf-compiler libqrencode-dev libminiupnpc-dev qt5-default`

###Beancash QT Compile###
`git clone -b non-static --single-branch https://github.com/Crazyhead90/BeanCash.git Beancash`
`cd Beancash`
`qmake "USE_UPNP=1" Beancash-qt.pro`
`make`

###Beancash daemon Compile###
(Recommend Daemon)
With UPNP:
``cd src && \
    make -f makefile.unix && \
    strip Beancashd``
    
Without UPNP:
```cd src && \
    make -f makefile.unix USE_UPNP= && \
    strip Beancashd```
