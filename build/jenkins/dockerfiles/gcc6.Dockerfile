FROM gcc:6

RUN useradd -ms /bin/bash --uid 1006 builder

RUN apt-get -qq update && apt-get -qq upgrade
RUN apt-get install -qqy cmake libcurl4-gnutls-dev libenet-dev         \
    libgnutls28-dev libgtk-3-dev libicu-dev libidn11-dev libjson-perl  \
    libminiupnpc-dev libnspr4-dev libpython-dev libogg-dev             \
    libopenal-dev libpng-dev libsdl2-dev libvorbis-dev libxcursor-dev  \
    libxml2-dev libxml-simple-perl subversion zlib1g-dev

ADD libsodium-1.0.17.tar.gz /root/libsodium/
RUN cd /root/libsodium/libsodium-1.0.17 \
 && ./configure \
 && make && make check \
 && make install

ADD boost_1_67_0.tar.bz2 /root/boost/
RUN cd /root/boost/boost_1_67_0 \
 && ./bootstrap.sh --with-libraries=filesystem \
 && ./b2 install

ADD gloox-1.0.22.tar.bz2 /root/gloox/
RUN cd /root/gloox/gloox-1.0.22 \
 && ./configure \
 && make && make check \
 && make install

ADD wxWidgets-3.0.4.tar.bz2 /root/wxWidgets/
RUN cd /root/wxWidgets/wxWidgets-3.0.4 \
 && ./configure \
 && make \
 && make install

RUN ldconfig

USER builder
ENV SHELL /bin/bash

ENV LIBCC gcc
ENV LIBCXX g++
ENV PSCC gcc
ENV PSCXX g++
