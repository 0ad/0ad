FROM gcc:8

RUN useradd -ms /bin/bash --uid 1006 builder

RUN apt-get -qq update && apt-get -qq upgrade
RUN apt-get install -qqy cmake git gettext libxml2-dev python python-pip subversion

RUN pip2 install transifex-client lxml

RUN git clone https://anongit.kde.org/pology.git /root/pology
RUN mkdir /root/pology/build
WORKDIR /root/pology/build
RUN cmake ..
RUN make && make install

USER builder
COPY --chown=builder transifexrc /home/builder/.transifexrc
