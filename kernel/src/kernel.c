#include <../../kernel/include/kernel.h>

int main(int argc, char* argv[]) {
    
    decir_hola("Kernel la puta madre");

    logger = log_create("kernel_logs.log","kernel",1,LOG_LEVEL_INFO);
    config = config_create("./kernel.config");
    //conexion KERNEL A MEMORIA//
    leer_configuraciones();
    socket_cliente_a_MEMORIA = crear_conexion(IP_MEMORIA,PUERTO_MEMORIA);
    //conexion KERNEL A CPU//
    socket_cliente_a_CPUU = crear_conexion(IP_MEMORIA,PUERTO_ESCUCHA_CPU);

    if(!socket_cliente_a_MEMORIA){
        log_error(logger,"Alguna conexion esta tirando error");
        terminar_programa();
        exit(2);
    }

    //KERNEL A MEMORIA//
    enviar_operacion(socket_cliente_a_MEMORIA, &handshakeDeMemoria);

    char mensaje[1024] = {0};
    recv(socket_cliente_a_MEMORIA, mensaje, 1024, 0);
    printf("Respuesta del servidor: %s\n", mensaje);    

    close(socket_cliente_a_MEMORIA);
     //KERNEL A CPU//
    enviar_operacion(socket_cliente_a_CPUU, &handshakeDeCpu);

    char mensaje3[1024] = {0};
    recv(socket_cliente_a_CPUU, mensaje3, 1024, 0);
    printf("Respuesta del servidor: %s\n", mensaje3);    

    close(socket_cliente_a_CPUU);



    server_socket = iniciar_servidor(NULL , PUERTO_PROPIO);
    int cliente_socket;
    
    while ((cliente_socket = esperar_cliente(server_socket)) != (-1))
    {
        log_info(logger,"Se conecto la interfaz de I/O");
        int cod_op = recibir_operacion(cliente_socket);
        if(cod_op == 1){
            enviar_mensaje_de_exito(cliente_socket, "Mensaje desde Kernel a Interfaz IO");   
            log_info(logger,"Ya te mande el valor y el OP esta bien, Interfaz IO");
            break;
        }else{
            log_info(logger, "Codigo de operacion no reconocido en el server");
            break;
        }
    }
    
    close(server_socket);
    terminar_programa();
    return 0;
}

void leer_configuraciones(){
    IP_PROPIO = config_get_string_value(config,"IP_ESCUCHA");
    PUERTO_PROPIO = config_get_string_value(config,"PUERTO_ESCUCHA");
    IP_MEMORIA = config_get_string_value(config,"IP_MEMORIA");
    PUERTO_MEMORIA = config_get_string_value(config,"PUERTO_MEMORIA");
    IP_CPU = config_get_string_value(config,"IP_CPU");
    PUERTO_ESCUCHA_CPU = config_get_string_value(config,"PUERTO_ESCUCHA_CPU");

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