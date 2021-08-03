FROM ubuntu:20.04
ENV DEBIAN_FRONTEND noninteractive

COPY makefile /makefile
RUN apt-get update && apt-get install -y \
    g++ \
    make \
    libsqlite3-dev \
    librestbed-dev \
    nlohmann-json3-dev

WORKDIR /usr/src/app
COPY . .

RUN ["make"]

CMD ["./fulltext"]

EXPOSE 1984