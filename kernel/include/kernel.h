#ifndef KERNEL_H_
#define KERNEL_H_

#include <stdlib.h>
#include <stdio.h>
#include <../../utils/include/hello.h>
#include <../../utils/include/socket.h>
#include <../../utils/include/protocolo.h>
#include <commons/config.h>
#include <commons/log.h>

int fd_escucha_kernel;
int fd_conexion_client_cpu;
int fd_conexion_client_memoria;
int fd_conexion_server_io;

char* mensaje;
char* IP_PROPIO;
char* PUERTO_PROPIO;
char* IP_CPU;
char* PUERTO_CPU;
char* IP_MEMORIA;
char* PUERTO_MEMORIA;


int* handshakeDeMemoria;
(*handshakeDeMemoria) = 1;

int* handshakeDeCpu;
(*handshakeDeCpu) = 5;

t_log* logger_kernel;
t_config* config_kernel;

bool iniciar_conexiones();
void terminar_programa();
void leer_configuraciones();

#endif