FROM build-base:latest

RUN apt-get install -qqy gcc-7 g++-7

USER builder

ENV LIBCC gcc-7
ENV LIBCXX g++-7
ENV CC gcc-7
ENV CXX g++-7
