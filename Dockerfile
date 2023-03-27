FROM gcc:4.9
COPY ./ /usr/src/server
WORKDIR /usr/src/server
RUN gcc -std=c99  *.c -o server
CMD ["./server"]
