FROM build-base:latest

ARG DEBIAN_FRONTEND=noninteractive
ARG DEBCONF_NOWARNINGS="yes"
RUN apt-get install -qqy gcc-7 g++-7 llvm-7 libclang-7-dev --no-install-recommends

USER builder

ENV LIBCC gcc-7
ENV LIBCXX g++-7
ENV CC gcc-7
ENV CXX g++-7
