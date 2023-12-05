FROM ubuntu:20.04

# Install system packages
RUN DEBIAN_FRONTEND="noninteractive" apt-get update && apt-get -y install tzdata
RUN apt-get update \
    && apt-get install -y \
        openssh-server  \
        sudo \
        ssh \
    \
        python3-pip \
        python3-dev \
        python3-venv \
        libevent-dev \
    \
        vim \
    \
        gcc \
        g++ \
        gdb \
        clang \
        cmake \
        make \
        build-essential \
        autoconf \
        automake \
        valgrind \
        software-properties-common \
        libomp-dev \
    \
       ninja-build \
       neovim \
       swig \
   \
       locales-all \
       dos2unix \
   \
       doxygen \
       fish \
        rsync \
        tar \
        tree \
        wget \
    && apt-get clean

RUN pip3 install --upgrade pip numpy matplotlib scipy pandas seaborn
WORKDIR /app