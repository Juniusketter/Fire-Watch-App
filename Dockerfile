
FROM ubuntu:22.04
RUN apt update && apt install -y build-essential cmake libsqlite3-dev
WORKDIR /app
COPY . .
RUN cmake . && make
CMD ["./firewatch"]
