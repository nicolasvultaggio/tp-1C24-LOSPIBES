#ifndef ENTRADASALIDA_ENTRADASALIDA_H
#define ENTRADASALIDA_ENTRADASALIDA_H

#include <stdlib.h>
#include <stdio.h>
#include <../../utils/include/hello.h>
#include <../../utils/include/socket.h>
#include <../../utils/include/protocolo.h>
#include <commons/config.h>
#include <commons/log.h>

int socket_cliente_a_kernel;//socket para enviar a kernel
int socket_cliente_a_memoria;//socket para enviar a memoria

int* handshake_de_memoria;
(*handshake_de_memoria) = 2;

int* handshake_de_kernel;
(*handshake_de_kernel) = 1;

char* ip_kernel;
char* puerto_kernel;
char* ip_memoria;
char* puerto_memoria;
char* mensaje_ok_memoria;
char* mensaje_ok_kernel;

t_log* logger;
t_config* config;

int conectar_modulo(int un_socket, char *un_ip, char* un_puerto);
void leer_las_configs();
void terminar_programa();

#endif