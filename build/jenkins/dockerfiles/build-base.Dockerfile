FROM debian:buster

RUN useradd -ms /bin/bash --uid 1006 builder
# 0 A.D. dependencies.
ARG DEBIAN_FRONTEND=noninteractive
ARG DEBCONF_NOWARNINGS="yes"
RUN apt-get -qqy update && apt-get install -qqy \
      cmake \
      curl \
      libboost-dev \
      libboost-filesystem-dev \
      libcurl4-gnutls-dev \
      libenet-dev \
      libfmt-dev \
      libfreetype6-dev \
      libgloox-dev \
      libgnutls28-dev \
      libgtk-3-dev \
      libicu-dev \
      libidn11-dev \
      libjson-perl \
      libminiupnpc-dev \
      libogg-dev \
      libopenal-dev \
      libpng-dev \
      libsdl2-dev \
      libsodium-dev \
      libvorbis-dev \
      libwxgtk3.0-dev \
      libxcursor-dev \
      libxml-simple-perl \
      libxml2-dev \
      m4 \
      python3-dev \
      python3-pip \
      zlib1g-dev \
 && apt-get clean

# Install rust and Cargo via rustup
USER builder
RUN curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh -s -- -y
ENV PATH="${PATH}:/home/builder/.cargo/bin"
USER root

ENV SHELL /bin/bash

