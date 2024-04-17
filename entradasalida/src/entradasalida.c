#include <stdlib.h>
#include <stdio.h>
#include <../../utils/include/hello.h>
#include <../include/entradasalida.h>

int main(int argc, char* argv[]) {
    decir_hola("la interfaz de I/Os");

    logger = log_create("entradasalida_logs.log","entradasalida",1,LOG_LEVEL_INFO); // crea el puntero al log
    config = config_create("./entradasalida.config"); // crea el puntero al archivo de config
    
    leer_las_configs();//eso, guarda toda la info necesaria del archivo de las configuraciones
    

    socket_cliente_a_memoria = crear_conexion(ip_memoria,puerto_memoria);

    if(socket_cliente_a_memoria == -1){
        log_error(logger,"La conexion entre la interfaz y la memoria no estaria funcionando.");
        terminar_programa();
        exit(1);
    }

    enviar_operacion(socket_cliente_a_memoria, &handshake_de_memoria);


    char mensaje_ok_memoria[1024] = {0};

    recv(socket_cliente_a_memoria, mensaje_ok_memoria, 1024, 0);

    printf("Respuesta del servidor: %s\n", mensaje_ok_memoria);

    close(socket_cliente_a_memoria);


    
    socket_cliente_a_kernel = crear_conexion(ip_kernel,puerto_kernel);
 
    if(socket_cliente_a_kernel == -1){
        log_error(logger,"La conexion hentre la interfaz y el kernel no estaria funcionando.");
        terminar_programa();
        exit(2);
    }

    enviar_operacion(socket_cliente_a_kernel,&handshake_de_kernel);

    char mensaje_ok_kernel[1024] = {0};
    recv(socket_cliente_a_kernel, mensaje_ok_kernel, 1024, 0);
    printf("Respuesta del servidor: %s\n", mensaje_ok_kernel);

    close(socket_cliente_a_kernel);


    terminar_programa();

    return 0;
}

void leer_las_configs(){
    ip_memoria = config_get_string_value(config,"IP_MEMORIA");
    puerto_memoria = config_get_string_value(config,"PUERTO_MEMORIA");
    ip_kernel = config_get_string_value(config,"IP_KERNEL");
    puerto_kernel = config_get_string_value(config,"PUERTO_KERNEL");
    return;
}

int conectar_modulo(int un_socket, char *un_ip, char* un_puerto){
    un_socket = crear_conexion(un_ip,un_puerto);
    return un_socket != -1;
}

void terminar_programa()
{
	if(logger != NULL){
		log_destroy(logger);
	}
	if(config != NULL){
		config_destroy(config);
	}
    liberar_conexion(socket_cliente_a_kernel);
    liberar_conexion(socket_cliente_a_memoria);
}