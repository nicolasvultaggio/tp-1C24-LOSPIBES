#ifndef MAIN_H_
#define MAIN_H

#include <stdlib.h>
#include <stdio.h>
#include <../../utils/include/hello.h>
#include <../../utils/include/socket.h>
#include <../../utils/include/protocolo.h>
#include <commons/config.h>
#include <commons/log.h>
int server_socket;
int socket_cliente_a_CPU;
int socket_cliente_delKERNEL;

char* mensaje1;
char* ip__propio;
char* puerto__propio; 
char* ip__memoria;
char* puerto__memoria;

int socket_cliente;//filedescriptorCPU

int* handshakeDeCPU;
(*handshakeDeCPU) = 0;


t_log* logger_cpu;
t_config* config_cpu;

bool iniciar_conexiones();
void terminar_programa();
void leer__configuraciones();
#endif