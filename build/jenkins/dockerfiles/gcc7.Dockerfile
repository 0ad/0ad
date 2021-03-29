FROM build-base:latest

RUN apt-get install -qqy gcc-7 g++-7

USER builder

ENV LIBCC gcc-7
ENV LIBCXX g++-7
ENV PSCC gcc-7
ENV PSCXX g++-7
