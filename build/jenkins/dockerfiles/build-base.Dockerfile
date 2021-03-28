FROM debian:buster

RUN useradd -ms /bin/bash --uid 1006 builder

RUN apt-get -qq update

# 0 A.D. dependencies.
RUN apt-get install -qqy \
      build-essential \
      cmake \
      curl \
      libboost-dev \
      libboost-filesystem-dev \
      libclang-7-dev \
      libcurl4-gnutls-dev \
      libenet-dev \
      libfmt-dev \
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
      libsodium-dev \
      libsdl2-dev \
      libvorbis-dev \
      libwxgtk3.0-dev \
      libxcursor-dev \
      libxml2-dev \
      libxml-simple-perl \
      llvm-7 \
      zlib1g-dev \
 && apt-get clean

# Other utilities
RUN apt-get install -qqy \
      python3-dev \
      python3-pip \
      rsync \
      subversion \
      vim \
      mkdocs

# Install rust and Cargo via rustup
USER builder
RUN curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh -s -- -y
ENV PATH="${PATH}:/home/builder/.cargo/bin"
USER root

ENV SHELL /bin/bash
