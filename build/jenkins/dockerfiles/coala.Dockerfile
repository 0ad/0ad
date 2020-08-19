FROM python:3.8

RUN useradd --uid 1006 builder

RUN apt-get -yy update && apt-get -yy install \
  cppcheck \
  git \
  npm \
  subversion

RUN npm install -g npm@latest
RUN npm install -g eslint@5.16.0 jshint eslint-plugin-brace-rules

RUN pip3 install svn

RUN git clone -b 0ad-fixes https://github.com/0ad/coala.git
RUN cd coala && pip3 install .

RUN git clone -b 0ad-fixes https://github.com/0ad/coala-bears.git
RUN cd coala-bears && pip3 install .

USER builder
