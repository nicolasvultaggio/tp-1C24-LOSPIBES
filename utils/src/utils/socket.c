#include <../../utils/include/socket.h>

//Cliente
int crear_conexion(char *ip,char *puerto, t_log * unLogger, char * conectado)
{
	struct addrinfo hints;
	struct addrinfo *server_info;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	getaddrinfo(ip, puerto, &hints, &server_info);

	
	int socket_cliente = socket(server_info->ai_family, server_info->ai_socktype, server_info-> ai_protocol);

	// Ahora que tenemos el socket, vamos a conectarlo
	if(connect(socket_cliente, server_info->ai_addr, server_info->ai_addrlen) == (-1)){
		log_info(unLogger,"Fallo la conexion con %s", conectado);
	}else{
		log_info(unLogger,"Conectado a %s ", conectado);

	}
	freeaddrinfo(server_info);

	return socket_cliente;
}

//Servidor
int iniciar_servidor(char *ip,char *puerto, t_log * unLogger, char * escucha)
{
	struct addrinfo hints, *servinfo;

    
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	getaddrinfo(ip ,puerto, &hints, &servinfo);

	// Creamos el socket de escucha del servidor
	int socket_servidor = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);

	// Asociamos el socket a un puerto
	bind(socket_servidor,servinfo->ai_addr,servinfo->ai_addrlen);

	// Escuchamos las conexiones entrantes
	listen(socket_servidor, SOMAXCONN);

	log_info(unLogger,"Iniciado el puerto de escucha %s ", escucha);
	
	freeaddrinfo(servinfo);

	return socket_servidor;
}

int esperar_cliente(int socket_servidor, t_log * un_log, char * cliente)
{
	// Aceptamos un nuevo cliente
	int socket_cliente = accept(socket_servidor,NULL,NULL);
	if(socket_cliente == (-1)){
		log_info(un_log,"Ocurrio un error en el accept");
	}else{
		log_info(un_log, "Se conectó el cliente %s ", cliente);

	}
	return socket_cliente;
}

void liberar_conexion(int socket_cliente)
{
	close(socket_cliente);
}

