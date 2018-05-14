###Dependencies###
```
sudo apt-get install -y pkg-config
```
```
sudo apt-get -y install build-essential autoconf automake libtool libboost-all-dev libgmp-dev libssl-dev libcurl4-openssl-dev git 
yes '' | sudo add-apt-repository ppa:bitcoin/bitcoin
```
```
sudo apt-get update
sudo apt-get -y install libdb4.8-dev libdb4.8++-dev
sudo apt-get -y install fail2ban
sudo apt-get -y install make g++ gcc cpp ngrep iftop sysstat autotools-dev unzip
```
```
sudo apt-get -y install libqt5gui5 libqt5core5a libqt5dbus5 qttools5-dev qttools5-dev-tools libprotobuf-dev protobuf-compiler libqrencode-dev libminiupnpc-dev qt5-default
``` 

###Beancash QT Compile###
```
git clone -b non-static --single-branch https://github.com/Crazyhead90/BeanCash.git Beancash
cd Beancash
qmake "USE_UPNP=1" Beancash-qt.pro
make
```

###Beancash daemon Compile###<br />
(Recommend Daemon)<br />
With UPNP:<br />
```
cd src && \
    make -f makefile.unix && \
    strip Beancashd
```
    
Without UPNP:
```
cd src && \
    make -f makefile.unix USE_UPNP= && \
    strip Beancashd
```

If this guide helped you compile your wallet without having the unnecessary wasted time and headaches the "official" guides would give you and you want to buy me a beer for it, it would be appreciated:<br />
Beancash: 2HaJKvzBwnB3VMjHNPUtiqjfHBXk4aiKRC
