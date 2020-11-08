FROM debian:stable

WORKDIR /pyglpk

RUN set -e; \
    apt-get update -q=1; \
    apt-get install -q=1 -y \
        curl \
        lcov \
        libglpk-dev \
        gcc \
        git \
        make \
        python2 \
        python2-dev \
        python3 \
        python3-dev \
        python3-pip \
        rsync; \
        rm -rf /var/lib/apt/lists/*;

RUN python3 -m pip install \
    pytest \
    setuptools_scm \
    sphinx \
    sphinx-autobuild \
    tox
