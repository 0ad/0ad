FROM debian:buster

ARG DEBIAN_FRONTEND=noninteractive
ARG DEBCONF_NOWARNINGS="yes"
RUN useradd -ms /bin/bash --uid 1006 builder
RUN apt-get -qq update && apt-get install -qqy --no-install-recommends \
      curl \
      python3-dev \
      python3-pip \
      git \
      subversion \
 && apt-get clean

ENV SHELL /bin/bash
RUN pip3 install setuptools wheel
RUN pip3 install transifex-client lxml babel
USER builder
COPY --chown=builder transifexrc /home/builder/.transifexrc
