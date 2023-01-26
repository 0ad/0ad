FROM build-base:latest

# Obviously clang8 is not available but 13 is ^^"

ARG DEBIAN_FRONTEND=noninteractive
ARG DEBCONF_NOWARNINGS="yes"
RUN echo "deb https://deb.debian.org/debian buster-backports main" > /etc/apt/sources.list.d/backports.list
RUN apt-get update && apt-get install -qqy llvm-8 clang-8 lld-8 libclang-8-dev --no-install-recommends

USER builder

ENV CC clang-8
ENV CXX clang++-8
ENV LDFLAGS -fuse-ld=lld-8
