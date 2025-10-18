FROM ubuntu:22.04
RUN apt-get update && apt-get install -y build-essential cmake git
WORKDIR /app
COPY . /app
RUN make all || true
CMD ["/bin/bash"]
