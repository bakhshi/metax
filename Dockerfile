FROM ubuntu:18.04

# Install necessary dependencies
RUN apt-get update &&\
    apt-get install -y --no-install-recommends \
        build-essential \
        git \
        zlib1g-dev \
        vim \
        gdb \
        ca-certificates \
        locales \
        g++ \
        make \
        pkg-config \
        libssl-dev \
        wget \
        locales-all &&\
    apt-get clean

ENV LC_ALL en_US.UTF-8
ENV LANG en_US.UTF-8
ENV LANGUAGE en_US.UTF-8

RUN wget https://pocoproject.org/releases/poco-1.8.1/poco-1.8.1-all.tar.bz2
RUN tar -xf poco-1.8.1-all.tar.bz2

WORKDIR /poco-1.8.1-all

RUN ./configure --no-tests --no-samples --omit=CppUnit,Data,MongoDB,PageCompiler,Redis,Zip
RUN make
RUN make install
RUN ldconfig

ADD . /metax

WORKDIR /metax

RUN make setup
RUN make