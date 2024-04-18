#ifndef MAIN_H_
#define MAIN_H

#include <stdlib.h>
#include <stdio.h>
#include <../../utils/include/hello.h>
#include <../../utils/include/socket.h>
#include <../../utils/include/protocolo.h>
#include <commons/config.h>
#include <commons/log.h>

int fd_conexion_server_kernel;
int fd_conexion_client_memoria;
int fd_escucha_cpu;

char* ip_propio;
char* puerto_propio; 
char* ip_memoria;
char* puerto_memoria;


t_log* logger_cpu;
t_config* config_cpu;

bool iniciar_conexiones();
void terminar_programa();
void leer__configuraciones();
#endif
