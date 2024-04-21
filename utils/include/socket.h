#ifndef SOCKETS_H_
#define SOCKETS_H_

#include<stdio.h>
#include<stdlib.h>
#include<sys/socket.h>
#include<commons/log.h>
#include<commons/collections/list.h>
#include<string.h>
#include<commons/config.h>
#include<netdb.h>
#include<pthread.h>

int iniciar_servidor(char* ip, char* puerto, t_log * unLogger,char * escucha);
int esperar_cliente(int socket_servidor, t_log * un_log, char * cliente);
int crear_conexion(char *ip, char* puerto,t_log * un_log, char * conectado);
void liberar_conexion(int socket_cliente);


#endif 
