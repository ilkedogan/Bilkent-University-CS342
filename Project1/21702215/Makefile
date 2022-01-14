all: statclient_th statserver_th statclient statserver
statclient_th: statclient_th.c
	gcc -Wall -g -o statclient_th statclient_th.c -lrt
statserver_th: statclient_th.c
	gcc -Wall -g -o statserver_th statserver_th.c -lrt -lpthread
statclient: statclient.c
	gcc -Wall -g -o statclient statclient.c -lrt
statserver: statclient.c
	gcc -Wall -g -o statserver statserver.c -lrt
clean: 
	rm -fr *~ statclient statserver statserver_th statclient_th
	
	
