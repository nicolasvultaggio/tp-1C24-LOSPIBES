#ifndef ENTRADASALIDA_ENTRADASALIDA_H
#define ENTRADASALIDA_ENTRADASALIDA_H

#include <stdlib.h>
#include <stdio.h>
#include <../../utils/include/hello.h>
#include <../../utils/include/socket.h>
#include <../../utils/include/protocolo.h>
#include <commons/config.h>
#include <commons/log.h>

int fd_conexion_kernel;//socket para enviar a kernel
int fd_conexion_memoria;//socket para enviar a memoria

char* ip_kernel;
char* puerto_kernel;
char* ip_memoria;
char* puerto_memoria;
char* mensaje_ok_memoria;
char* mensaje_ok_kernel;

t_log* logger_io;
t_config* config_io;

void leer_configuraciones();
void terminar_programa();
bool iniciar_conexiones();

#endif