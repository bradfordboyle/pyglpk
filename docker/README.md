# Docker Images

These docker images can be used for local development and testing.

## Building

```sh
pushd docker
docker build -t pyglpk-dev .
popd
```

## Running

```sh
docker run --rm -it --mount type=bind,source="${PWD}",destination=/pyglpk pyglpk-dev tox -e py27,py37
```
