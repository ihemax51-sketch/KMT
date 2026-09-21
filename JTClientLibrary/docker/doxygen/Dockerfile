FROM alpine:3.19.1
RUN apk update && apk add doxygen git make
RUN git clone https://github.com/jothepro/doxygen-awesome-css.git && cd doxygen-awesome-css && make install
