FROM ubuntu:22.04
RUN apt update && apt install -y build-essential cmake
WORKDIR /app
COPY . .
RUN cmake . && make
CMD ["./firewatch"]
