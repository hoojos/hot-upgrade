FROM gcc:4.9
COPY ./ /usr/src/server
WORKDIR /usr/src/server
RUN gcc -std=c99 -o server main.c
CMD ["tail -f /dev/null"]
