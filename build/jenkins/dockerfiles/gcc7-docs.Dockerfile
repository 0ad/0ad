FROM 0ad-gcc7:latest

USER root

ARG DEBIAN_FRONTEND=noninteractive
ARG DEBCONF_NOWARNINGS="yes"
RUN apt-get install -qqy graphviz doxygen xsltproc lcov --no-install-recommends
RUN update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-7 7 --slave /usr/bin/g++ g++ /usr/bin/g++-7 --slave /usr/bin/gcov gcov /usr/bin/gcov-7

USER builder
