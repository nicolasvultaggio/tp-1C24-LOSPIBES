#ifndef PROTOCOLO_H_
#define PROTOCOLO_H_
#include<stdio.h>
#include<stdlib.h>
#include<sys/socket.h>
#include<commons/log.h>
#include<string.h>
#include<commons/config.h>

typedef enum{
	handshakeCPU,
	handshakeKERNEL,
	handshakeIO,
	handshakeMEMORIA
}handshake;

typedef enum{
	MENSAJE,
	PAQUETE
}op_code;

typedef struct
{
	int size;
	void* stream;
} t_buffer;

typedef struct
{
	op_code codigo_operacion;
	t_buffer* buffer;
} t_paquete;

void crear_buffer(t_paquete* paquete);
void* serializar_paquete(t_paquete* paquete, int bytes);
void enviar_mensaje(char* mensaje, int socket_cliente);
t_paquete* crear_paquete(void);
void agregar_a_paquete(t_paquete* paquete, void* valor, int tamanio);
void enviar_paquete(t_paquete* paquete, int socket_cliente);
void eliminar_paquete(t_paquete* paquete);
t_list* recibir_paquete(int socket_cliente);
void* recibir_buffer(int* size, int socket_cliente);
int recibir_operacion(int socket_cliente);
void recibir_mensaje(t_log* loggerServidor, int socket_cliente);
void enviar_operacion(int socket_conexion, int* valor);
void enviar_mensaje_de_exito(int socket, char* mensaje);
void enviar_handshake(int unSocket, handshake numero);
#endif