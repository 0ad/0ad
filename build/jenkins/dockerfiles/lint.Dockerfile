FROM debian:bookworm-slim
ARG DEBIAN_FRONTEND=noninteractive
ARG DEBCONF_NOWARNINGS="yes"
RUN useradd -ms /bin/bash --uid 1006 builder
RUN apt-get -qq update && apt-get install -qqy --no-install-recommends \
	python3-dev \
	cppcheck \
	git \
	subversion \
	libphutil \
	php-cli \
	php-curl \
	curl \
&& apt-get clean
ENV SHELL /bin/bash
WORKDIR "/home/builder"
RUN curl -fsSLk https://deb.nodesource.com/setup_20.x -o nodesource_setup.sh
RUN chmod +x nodesource_setup.sh && ./nodesource_setup.sh && rm nodesource_setup.sh
RUN apt install nodejs -qqy
RUN npm install -g eslint@8.57.0 eslint-plugin-brace-rules
USER builder 
RUN git clone https://github.com/phacility/arcanist.git ~/arcanist
ENV PATH="${PATH}:~/arcanist/bin/"
