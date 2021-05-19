if [ `uname -a | grep -c armv7l` -ge 1 ]
then
    gcc -Wall -Wextra -g -lm -lmraa lab4b.c -o lab4b
else
    gcc -DDUMMY -lm lab4b.c -o lab4b
fi
