FROM build-base

# This silences a transifex-client warning
RUN apt-get install -qqy git

RUN pip3 install transifex-client lxml babel

USER builder
COPY --chown=builder transifexrc /home/builder/.transifexrc
