# NAME: Qinglin Zhang
# EMAIL: qqinglin0327@gmail.com
# ID:205356739
CFLAGS=-Wall -Wextra

default:
	chmod 777 compile.sh
	./compile.sh
clean:
	rm -f lab4c-205356739.tar.gz  lab4c_tcp lab4c_tls
check: default
	./lab4c_tcp --id=205356739 --host=lever.cs.ucla.edu --log=logfile 19000
	./lab4c_tls --id=205356739 --host=lever.cs.ucla.edu --log=logfile 18000
dist:default
	tar -czvf lab4c-205356739.tar.gz lab4c_tls.c lab4c_tcp.c Makefile README compile.sh
