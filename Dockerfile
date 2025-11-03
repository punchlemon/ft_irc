FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive

# Install build tools
RUN apt-get update \
    && apt-get install -y --no-install-recommends \
       build-essential \
       g++ \
       make \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /usr/src/app

# Copy project sources
COPY . /usr/src/app

# Build using the existing Makefile
RUN make

# Expose IRC default port used by this project
EXPOSE 6667

# Default command — can be overridden by docker run or docker-compose
CMD ["/usr/src/app/ircserv", "6667", "password"]
