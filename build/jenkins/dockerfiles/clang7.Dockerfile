FROM build-base:latest

RUN apt-get install -qqy clang-7 lld-7

USER builder

ENV CC clang-7
ENV CXX clang++-7
ENV LDFLAGS -fuse-ld=lld-7