#include <../../kernel/include/kernel.h>

int main(int argc, char* argv[]) {
    
    decir_hola("Kernel la puta madre");

    logger_kernel = log_create("kernel_logs.log","kernel",1,LOG_LEVEL_INFO);
    config_kernel = config_create("./kernel.config");
    leer_configuraciones();

    if(!iniciar_conexiones()){
        log_error(logger_kernel,"Alguna conexion esta tirando error");
        terminar_programa();
        exit(2);
    }


    //KERNEL A MEMORIA//
    enviar_operacion(socket_cliente_a_MEMORIA, &handshakeDeMemoria);
    char mensaje_ok_memoria[1024] = {0};
    recv(socket_cliente_a_MEMORIA, mensaje_ok_memoria, 1024, 0);
    printf("Respuesta de memoria: %s\n", mensaje_ok_memoria);    

    //KERNEL A CPU//
    enviar_operacion(socket_cliente_a_CPU, &handshakeDeCpu);
    char mensaje_ok_cpu[1024] = {0};
    recv(socket_cliente_a_CPU, mensaje_ok_cpu, 1024, 0);
    printf("Respuesta de cpu: %s\n", mensaje_ok_cpu);  

     



    server_socket = iniciar_servidor(NULL , PUERTO_PROPIO);
    socket_cliente_a_interfaz = esperar_cliente(server_socket);
    log_info(logger_kernel,"Se conecto la interfaz de I/O");

    int cod_op = recibir_operacion(socket_cliente_a_interfaz);
    
    if(cod_op == 1){
        enviar_mensaje_de_exito(socket_cliente_a_interfaz, "Mensaje desde Kernel a Interfaz IO");   
        log_info(logger_kernel,"Ya te mande el valor y el OP esta bien, Interfaz IO");
        }else{
            log_info(logger_kernel, "Codigo de operacion no reconocido en el server");    
        }
    
    terminar_programa();
    return 0;
}

void leer_configuraciones(){
    IP_PROPIO = config_get_string_value(config_kernel,"IP_ESCUCHA");
    PUERTO_PROPIO = config_get_string_value(config_kernel,"PUERTO_ESCUCHA");
    IP_MEMORIA = config_get_string_value(config_kernel,"IP_MEMORIA");
    PUERTO_MEMORIA = config_get_string_value(config_kernel,"PUERTO_MEMORIA");
    IP_CPU = config_get_string_value(config_kernel,"IP_CPU");
    PUERTO_ESCUCHA_CPU = config_get_string_value(config_kernel,"PUERTO_ESCUCHA_CPU");

}

bool iniciar_conexiones(){
    socket_cliente_a_CPU = crear_conexion(IP_CPU,PUERTO_ESCUCHA_CPU);
    socket_cliente_a_MEMORIA = crear_conexion(IP_MEMORIA,PUERTO_MEMORIA); 

    return socket_cliente_a_CPU != -1 && socket_cliente_a_MEMORIA != -1; 
}

void terminar_programa(){

    config_destroy(config_kernel);
    log_destroy(logger_kernel);   
    close(server_socket);
}