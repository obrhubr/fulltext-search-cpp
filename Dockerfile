FROM ubuntu:20.04

ENV DEBIAN_FRONTEND noninteractive

WORKDIR /usr/src/app
COPY . .

RUN apt-get update && apt-get install -y \
    g++ \
    make \
    libsqlite3-dev \
    librestbed-dev \
    nlohmann-json3-dev

RUN ["make"]

CMD ["./fulltext"]

EXPOSE 1984