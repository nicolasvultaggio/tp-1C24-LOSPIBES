#include <../../kernel/include/kernel.h>

int main(int argc, char* argv[]) {
    
    decir_hola("Kernel la puta madre");

    logger = log_create("kernel_logs.log","kernel",1,LOG_LEVEL_INFO);
    config = config_create("./kernel.config");

    leer_configuraciones();
    socket_cliente_a_MEMORIA = crear_conexion(IP_MEMORIA,PUERTO_MEMORIA);

    if(!socket_cliente_a_MEMORIA){
        log_error(logger,"Alguna conexion esta tirando error");
        terminar_programa();
        exit(2);
    }

    
    enviar_operacion(socket_cliente_a_MEMORIA, &handshakeDeMemoria);

    
    recv(socket_cliente_a_MEMORIA, mensaje, 1024, 0);
    printf("Respuesta del servidor: %s\n", mensaje);    

    close(socket_cliente_a_MEMORIA);

    

    server_socket = iniciar_servidor(NULL , PUERTO_PROPIO);
    int cliente_socket = esperar_cliente(server_socket);
    
    terminar_programa();
    return 0;
}

void leer_configuraciones(){
    IP_PROPIO = config_get_string_value(config,"IP_ESCUCHA");
    PUERTO_PROPIO = config_get_string_value(config,"PUERTO_ESCUCHA");
    IP_MEMORIA = config_get_string_value(config,"IP_MEMORIA");
    PUERTO_MEMORIA = config_get_string_value(config,"PUERTO_MEMORIA");
    IP_CPU = config_get_string_value(config,"IP_CPU");

}

bool iniciar_conexiones(){
    //socket_cliente_a_CPU = crear_conexion(IP_CPU,PUERTO_CPU);
    socket_cliente_a_MEMORIA = crear_conexion(IP_MEMORIA,PUERTO_MEMORIA); 

    return socket_cliente_a_MEMORIA != -1;//socket_cliente_a_CPU != -1 && socket_cliente_a_MEMORIA != -1; 
}

void terminar_programa(){
    config_destroy(config);
    log_destroy(logger);
    
}