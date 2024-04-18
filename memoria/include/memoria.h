#ifndef MEMORIA_H_
#define MEMORIA_H_
#include <../../utils/include/hello.h>
#include <../../utils/include/socket.h>
#include <../../utils/include/protocolo.h>

int fd_escucha_memoria;
int fd_conexion_server;
int handshake;
//No va a quedar para siempre
int handshakeDeCPU = 0;
int handshakeDeKERNEL = 1;
int handshakeDeIO = 2;

char* puerto_propio;

t_log* logger_memoria;
t_config* config_memoria;

void terminar_programa();
void leer_configuraciones();

#endif