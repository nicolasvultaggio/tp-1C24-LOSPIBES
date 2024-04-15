#include <../../utils/include/protocolo.h>

//Cliente
void crear_buffer(t_paquete* paquete)
{
	paquete->buffer = malloc(sizeof(t_buffer));
	paquete->buffer->size = 0;
	paquete->buffer->stream = NULL;
}

void* serializar_paquete(t_paquete* paquete, int bytes)
{
	void * magic = malloc(bytes);
	int desplazamiento = 0;

	memcpy(magic + desplazamiento, &(paquete->codigo_operacion), sizeof(int));
	desplazamiento+= sizeof(int);
	memcpy(magic + desplazamiento, &(paquete->buffer->size), sizeof(int));
	desplazamiento+= sizeof(int);
	memcpy(magic + desplazamiento, paquete->buffer->stream, paquete->buffer->size);
	desplazamiento+= paquete->buffer->size;

	return magic;
}

void enviar_mensaje(char* mensaje, int socket_cliente)
{
	t_paquete* paquete = malloc(sizeof(t_paquete));

	paquete->codigo_operacion = MENSAJE;
	paquete->buffer = malloc(sizeof(t_buffer));
	paquete->buffer->size = strlen(mensaje) + 1;
	paquete->buffer->stream = malloc(paquete->buffer->size);
	memcpy(paquete->buffer->stream, mensaje, paquete->buffer->size);

	int bytes = paquete->buffer->size + 2*sizeof(int);

	void* a_enviar = serializar_paquete(paquete, bytes);

	send(socket_cliente, a_enviar , bytes, 0);

	free(a_enviar);
	eliminar_paquete(paquete);
	
}

t_paquete* crear_paquete(void)
{
	t_paquete* paquete = malloc(sizeof(t_paquete));
	paquete->codigo_operacion = PAQUETE;
	crear_buffer(paquete);
	return paquete;
}

void agregar_a_paquete(t_paquete* paquete, void* valor, int tamanio)
{
	paquete->buffer->stream = realloc(paquete->buffer->stream, paquete->buffer->size + tamanio + sizeof(int));

	memcpy(paquete->buffer->stream + paquete->buffer->size, &tamanio, sizeof(int));
	memcpy(paquete->buffer->stream + paquete->buffer->size + sizeof(int), valor, tamanio);

	paquete->buffer->size += tamanio + sizeof(int);
}

void enviar_paquete(t_paquete* paquete, int socket_cliente)
{
	int bytes = paquete->buffer->size + 2*sizeof(int);
	void* a_enviar = serializar_paquete(paquete, bytes);

	send(socket_cliente, a_enviar, bytes, 0);

	free(a_enviar);
}

void eliminar_paquete(t_paquete* paquete)
{
	free(paquete->buffer->stream);
	free(paquete->buffer);
	free(paquete);
}

//Servidor

t_list* recibir_paquete(int socket_cliente)
{
	int size;
	int desplazamiento = 0;
	void * buffer;
	t_list* valores = list_create();
	int tamanio;

	buffer = recibir_buffer(&size, socket_cliente);
	while(desplazamiento < size)
	{
		memcpy(&tamanio, buffer + desplazamiento, sizeof(int));
		desplazamiento+=sizeof(int);
		char* valor = malloc(tamanio);
		memcpy(valor, buffer+desplazamiento, tamanio);
		desplazamiento+=tamanio;
		list_add(valores, valor);
	}
	free(buffer);
	return valores;
}


void* recibir_buffer(int* size, int socket_cliente)
{
	void * buffer;

	recv(socket_cliente, size, sizeof(int), MSG_WAITALL);
	buffer = malloc(*size);
	recv(socket_cliente, buffer, *size, MSG_WAITALL);

	return buffer;
}

int recibir_operacion(int socket_cliente)
{
	int cod_op;
	if(recv(socket_cliente, &cod_op, sizeof(int), MSG_WAITALL) > 0)
		return cod_op;
	else
	{
		close(socket_cliente);
		return -1;
	}
}


void enviar_operacion(int socket_conexion, int* valor)
{
	send(socket_conexion, valor, sizeof(int) ,0);
	
}

void recibir_mensaje(t_log* loggerServidor ,int socket_cliente)
{
	int size;
	char* buffer = recibir_buffer(&size, socket_cliente);
	log_info(loggerServidor, "Me llego el mensaje %s", buffer);
	free(buffer);
}

/*
// se implmenta despues de hacer accept
//			socket de la conexion ya creada / lo sabe el modulo   /   lo sabe el modulo  / lo sabe el modulo
void handshakeSERVIDOR(int socketConexion , int32_t handshakeExitoso , int32_t * conexionExitosa , int32_t * noCoincideHandshake ){
	int32_t handshake;
	size_t bytes;
    if(recv(socketConexion,&handshake,sizeof(int32_t), MSG_WAITALL)==(-1)){
        printf("No se pudo recibir handshake");
    };
    if(handshake == handshakeExitoso){
        bytes = send(socketConexion, conexionExitosa, sizeof(int), 0);
    }else{
        bytes = send(socketConexion, noCoincideHandshake, sizeof(int), 0); 
    }
	return;
}
// declaro la variable bytes porque send y recv deben devolver algo

// se implementa despues de hacer connect
void handshakeCLIENTE( int socketConexion , int32_t * handshakeAEnviar, int32_t valorDeConfirmacion  ){
	size_t bytes;
	int resultado;
	bytes = send(socketConexion, handshakeAEnviar , sizeof(int),0 );
	bytes = recv(socketConexion, &resultado, sizeof(int), MSG_WAITALL);
	if( resultado != valorDeConfirmacion){
		printf("El cliente recibio la respuesta del servidor pero no fue la correcta");
		exit(-1);
	}
	return;
}
*/
