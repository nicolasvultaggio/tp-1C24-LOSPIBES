#ifndef KERNEL_H_
#define KERNEL_H_

#include <stdlib.h>
#include <stdio.h>
#include <../../utils/include/hello.h>
#include <../../utils/include/socket.h>
#include <../../utils/include/protocolo.h>
#include <commons/config.h>
#include <commons/log.h>

int server_socket;
int socket_cliente_a_CPU;
int socket_cliente_a_MEMORIA;
int socket_cliente_a_CPUU;

char* mensaje;
char* IP_PROPIO;
char* PUERTO_PROPIO;
char* IP_CPU;
char* PUERTO_CPU;
char* IP_MEMORIA;
char* PUERTO_MEMORIA;
char* PUERTO_ESCUCHA_CPU;


int* handshakeDeMemoria;
(*handshakeDeMemoria) = 1;
 int* handshakeDeCpu;
 (*handshakeDeCpu)=5;

t_log* logger;
t_config* config;

bool iniciar_conexiones();
void terminar_programa();
void leer_configuraciones();

#endif