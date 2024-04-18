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
/*int socket_cliente_a_MEMORIA;*/

char* mensaje1;
char* ip__propio;
char* puerto__propio; 
char* ip__memoria;
char* puerto__memoria;

int* handshakeDeCPU;
(*handshakeDeCPU) = 0;



t_log* logger;
t_config* config1;

bool iniciar_conexiones();
void terminar_programa();
void leer__configuraciones();
#endif
