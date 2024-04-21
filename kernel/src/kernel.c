#include <../../kernel/include/kernel.h>

int main(int argc, char* argv[]) {
    
    decir_hola("Hola soy el Kernel SAPE");

    logger_kernel = log_create("kernel_logs.log","kernel",1,LOG_LEVEL_INFO);
    config_kernel = config_create("./kernel.config");
    leer_configuraciones();
    
    //Se conecta a Memoria
    fd_conexion_memoria = crear_conexion(ip_memoria,puerto_memoria,logger_kernel,"Memoria");
    //Se conecta a dispatch
    fd_conexion_dispatch = crear_conexion(ip_cpu,puerto_cpu_dispatch,logger_kernel,"Dispatch, Kernel");
    //Se conecta a dispatch
    fd_conexion_interrupt = crear_conexion(ip_cpu,puerto_cpu_interrupt,logger_kernel,"Interrupt, Kernel");
    //Espera IO
    fd_escucha_kernel = iniciar_servidor(NULL,puerto_propio,logger_kernel,"Kernel");
    fd_conexion_io = esperar_cliente(fd_escucha_kernel,logger_kernel,"IO");
    
    //atender a CPU-Dispatch
    args_atendedor * args_dispatch = crear_args(fd_conexion_dispatch,logger_kernel,"CPU-Dispatch");
    pthread_t hilo_dispatch;
    pthread_create(&hilo_dispatch,NULL,(void*)atender_conexion,(void *)args_dispatch);
    pthread_detach(hilo_dispatch);

    //atender a CPU-Interrupt
    args_atendedor * args_interrupt = crear_args(fd_conexion_interrupt,logger_kernel,"CPU-Interrupt");
    pthread_t hilo_interrupt;
    pthread_create(&hilo_interrupt,NULL,(void*)atender_conexion,(void *)args_interrupt);
    pthread_detach(hilo_interrupt);

    //atender a Memoria
    args_atendedor * args_memoria = crear_args(fd_conexion_memoria,logger_kernel,"Memoria");
    pthread_t hilo_memoria;
    pthread_create(&hilo_memoria,NULL,(void*)atender_conexion,(void *)args_memoria);
    pthread_detach(hilo_memoria); 

    args_atendedor *  args_io = crear_args(fd_conexion_io,logger_kernel,"IO");
    pthread_t hilo_io;
    pthread_create(&hilo_io,NULL,(void*)atender_conexion,(void *)args_io);
    // pthread_detach(hilo_memoria); no hago detach porque me terminaría el programa, terminando mis otros hilos
    pthread_join(hilo_io,NULL); // así, la cpu se ejecuta hasta que terminen todos los hilos


    /*
    if(!iniciar_conexiones()){
        log_error(logger_kernel,"Alguna conexion esta tirando error");
        terminar_programa();
        exit(2);
    }


    //KERNEL A MEMORIA//
    enviar_operacion(fd_conexion_client_memoria, handshakeKERNEL);
    char mensaje_ok_memoria[1024] = {0};
    recv(fd_conexion_client_memoria, mensaje_ok_memoria, 1024, 0);
    printf("Respuesta de memoria: %s\n", mensaje_ok_memoria);    

    //KERNEL A CPU//
    enviar_operacion(fd_conexion_client_cpu, handshakeKERNEL);
    char mensaje_ok_cpu[1024] = {0};
    recv(fd_conexion_client_cpu, mensaje_ok_cpu, 1024, 0);
    printf("Respuesta de cpu: %s\n", mensaje_ok_cpu);  


    fd_escucha_kernel = iniciar_servidor(NULL , puerto_propio);
    fd_conexion_server_io = esperar_cliente(fd_escucha_kernel);
    log_info(logger_kernel,"Se conecto la interfaz de I/O");

    op_code cod_op = recibir_operacion(fd_conexion_server_io);
    
    if(cod_op == handshakeIO){
        enviar_mensaje_de_exito(fd_conexion_server_io, "Mensaje desde Kernel a Interfaz IO");   
        log_info(logger_kernel,"Ya te mande el valor y el OP esta bien, Interfaz IO");
    }else{
            log_info(logger_kernel, "Codigo de operacion no reconocido en el server");    
    }
    */
    
    terminar_programa();
    return 0;
}

void leer_configuraciones(){
    puerto_propio = config_get_string_value(config_kernel,"PUERTO_PROPIO");
    ip_memoria = config_get_string_value(config_kernel,"IP_MEMORIA");
    puerto_memoria = config_get_string_value(config_kernel,"PUERTO_MEMORIA");
    ip_cpu= config_get_string_value(config_kernel,"IP_CPU");
    puerto_cpu_dispatch = config_get_string_value(config_kernel,"PUERTO_CPU_DISPATCH");
    puerto_cpu_interrupt = config_get_string_value(config_kernel,"PUERTO_CPU_INTERRUPT");
}
/*
bool iniciar_conexiones(){
    fd_conexion_client_cpu = crear_conexion(ip_cpu,puerto_cpu);
    fd_conexion_client_memoria = crear_conexion(ip_memoria,puerto_memoria); 

    return fd_conexion_client_cpu != -1 && fd_conexion_client_memoria != -1; 
}
*/



void terminar_programa(){

    config_destroy(config_kernel);
    log_destroy(logger_kernel);   
    liberar_conexion(fd_escucha_kernel);
}
