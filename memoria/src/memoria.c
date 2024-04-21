#include <../../memoria/include/memoria.h>
int main() {
    
    decir_hola("Memoria, verdad y justicia");

    logger_memoria = log_create("memoria_logs.log","memoria",1,LOG_LEVEL_INFO);
    config_memoria = config_create("./configs/memoria.config");
    leer_configuraciones();
    //iniciar servidor de memoria
    fd_escucha_memoria = iniciar_servidor(NULL, puerto_propio,logger_memoria,"Memoria");
    
    log_info(logger_memoria, "Puerto de memoria habilitado para sus clientes");

    log_info(logger_memoria, "Esperando CPU");
    fd_conexion_cpu = esperar_cliente(fd_escucha_memoria, logger_memoria, "CPU");
  
    log_info(logger_memoria, "Esperando KERNEL");
    fd_conexion_kernel = esperar_cliente(fd_escucha_memoria, logger_memoria, "KERNEL");
    
    log_info(logger_memoria, "Esperando IO");
    fd_conexion_io = esperar_cliente(fd_escucha_memoria, logger_memoria, "IO");
    
    args_atendedor * args_cpu = crear_args(fd_conexion_cpu,logger_memoria,"CPU");
    pthread_t hilo_cpu;
    pthread_create(&hilo_cpu,NULL,(void*) atender_conexion,(void *)args_cpu);
    pthread_detach(hilo_cpu);

    //atender a Kernel-Interrupt
    args_atendedor * args_kernel = crear_args(fd_conexion_kernel,logger_memoria,"Kernel");
    pthread_t hilo_kernel;
    pthread_create(&hilo_kernel,NULL,(void*) atender_conexion,(void *)args_kernel);
    pthread_detach(hilo_kernel);

    //atender a Memoria
    args_atendedor * args_io = crear_args(fd_conexion_io,logger_memoria,"IO");
    pthread_t hilo_io;
    pthread_create(&hilo_io,NULL,(void*)atender_conexion,(void *)args_io);
    // pthread_detach(hilo_memoria); no hago detach porque me terminaría el programa, terminando mis otros hilos
    pthread_join(hilo_io,NULL); // así, la cpu se ejecuta hasta que terminen todos los hilos

    /*
    while ((fd_conexion_server = esperar_cliente(fd_escucha_memoria)) != (-1))
    {
        log_info(logger_memoria,"Se conecto un cliente");
        op_code cod_op = recibir_operacion(fd_conexion_server);
        switch (cod_op)
        {
        case handshakeCPU:
            enviar_mensaje_de_exito(fd_conexion_server, "Mensaje desde memoria a CPU");
            log_info(logger_memoria,"Ya te mande el valor y el OP esta bien, cpu");
            break;
        case handshakeKERNEL:
            enviar_mensaje_de_exito(fd_conexion_server, "Mensaje desde memoria a Kernel");
            log_info(logger_memoria,"Ya te mande el valor y el OP esta bien, kernel");
            break;
        case handshakeIO:
            enviar_mensaje_de_exito(fd_conexion_server, "Mensaje desde memoria a Interfaz IO");   
            log_info(logger_memoria,"Ya te mande el valor y el OP esta bien, Interfaz IO");
            break;
        default:
            log_info(logger_memoria, "Codigo de operacion no reconocido en el server");
            log_info(logger_memoria,"Ya te mande el valor");
            break;
        }
        
    }
    */
    //NO cerramos conexion porq no esta hecho con memoria dinamica
	return EXIT_SUCCESS;    
}

//Esta pensado para que en un futuro lea mas de una configuracion. 
void leer_configuraciones(){
    puerto_propio = config_get_string_value(config_memoria,"PUERTO_PROPIO");
}

void terminar_programa(){
    config_destroy(config_memoria);
    log_destroy(logger_memoria);
}


