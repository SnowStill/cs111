# NAME: Qinglin Zhang
# EMAIL: qqinglin0327@gmail.com
# ID:205356739


default:
	gcc -Wall -Wextra -DDUMMY -lm lab4b.c -o lab4b
	chmod 777 compile.sh
	./compile.sh
clean:
	rm -f lab4b-205356739.tar.gz  lab4b
check: default
	chmod 777 test.sh
	./test.sh
dist: check
	tar -czvf lab4b-205356739.tar.gz lab4b.c Makefile README test.sh compile.sh
