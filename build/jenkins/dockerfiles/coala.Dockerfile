FROM python:3.7-alpine

RUN adduser -u 1006 -D builder

RUN apk add subversion cppcheck npm

RUN pip3 install coala-bears svn
RUN npm install -g eslint@5.16.0 jshint eslint-plugin-brace-rules

USER builder
