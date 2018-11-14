FROM alpine:edge
ENV GLPK_VERSION 4.63

WORKDIR /pyglpk

RUN echo "http://dl-cdn.alpinelinux.org/alpine/edge/testing" >> /etc/apk/repositories \
    && apk update \
    && apk add \
        curl \
        lcov \
        gcc \
        git \
        gmp \
        gmp-dev \
        make \
        musl-dev \
        py2-pip \
        python \
        python3 \
        python-dev \
        python3-dev \
        rsync

RUN mkdir -p /usr/local/src \
    && curl -SL http://ftp.gnu.org/gnu/glpk/glpk-${GLPK_VERSION}.tar.gz \
    | tar xzC /usr/local/src \
    && cd /usr/local/src/glpk-${GLPK_VERSION} \
    && ./configure --prefix=/usr/local --with-gmp\
    && make install

RUN pip install \
    pytest \
    sphinx \
    sphinx-autobuild \
    tox
