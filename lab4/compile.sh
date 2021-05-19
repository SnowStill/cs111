if [ `uname -a | grep -c armv7l` -ge 1 ]
then
    gcc -Wall -Wextra -g -lm -lmraa lab4c_tcp.c -o lab4c_tcp
    gcc -lssl -lcrypto -lm -lmraa lab4c_tls.c -o lab4c_tls
else
    gcc -DDUMMY -lm lab4c_tcp.c -o lab4c_tcp
    gcc -lssl -lcrypto -lm -DDUMMY lab4c_tls.c -o lab4c_tls
fi
