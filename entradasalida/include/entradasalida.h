#include <stdlib.h>
#include <stdio.h>
#include <../../utils/include/hello.h>
#include <../../utils/include/socket.h>
#include <../../utils/include/protocolo.h>
#include <commons/config.h>
#include <commons/log.h>
#ifndef ENTRADASALIDA_ENTRADASALIDA_H
#define ENTRADASALIDA_ENTRADASALIDA_H

int socket_cliente_a_kernel;//socket para enviar a kernel
int socket_cliente_a_memoria;//socket para enviar a memoria

char* ip_kernel;
char* puerto_kernel;
char* ip_memoria;
char* puerto_memoria;

t_log* logger;
t_config* config;

void leer_las_configs();
bool conectar_a_kernel();
bool conectar_a_memoria();
void terminar_programa();