#include <../../cpu/include/cpu.h>

int main(int argc, char* argv[]) {
    decir_hola("CPU");
    //CREO CONEXION CPU A MEMORIA Y RECIBO MENSAJE//

    logger_cpu = log_create("cpu_logs.log","cpu",1,LOG_LEVEL_INFO);
    config_cpu = config_create("./configs/cpu.config");

    leer__configuraciones();

    //Iniciar server dispatch
    fd_escucha_dispatch = iniciar_servidor(ip_propio,puerto_cpu_dispatch, logger_cpu,"Dispatch");
    //Iniciar server interrupt
    fd_escucha_interrupt = iniciar_servidor(ip_propio,puerto_cpu_interrupt , logger_cpu ,"Interrupt");
    //Me conecto a Memoria
    fd_conexion_memoria = crear_conexion(ip_memoria,puerto_memoria, logger_cpu, "Memoria");
    //Espero a Kernel desde Dispatch
    fd_conexion_dispatch = esperar_cliente(fd_escucha_dispatch,logger_cpu,"Dispatch, Kernel");
    //Espero Kernel desde Interrupt
    fd_conexion_interrupt = esperar_cliente(fd_escucha_interrupt,logger_cpu,"Interrupt, Kernel");
    
    
    
    //atender a Kernel-Dispatch
    args_atendedor * args_dispatch = crear_args(fd_conexion_dispatch,logger_cpu,"Kernel - Dispatch");
    pthread_t hilo_dispatch;
    pthread_create(&hilo_dispatch,NULL,(void*)atender_conexion,(void *)args_dispatch);
    pthread_detach(hilo_dispatch);

    //atender a Kernel-Interrupt
    args_atendedor *  args_interrupt = crear_args(fd_conexion_interrupt,logger_cpu,"Kernel - Interrupt");
    pthread_t hilo_interrupt;
    pthread_create(&hilo_interrupt,NULL,(void*)atender_conexion,(void *)args_interrupt);
    pthread_detach(hilo_interrupt);

    //atender a Memoria
    args_atendedor * args_memoria = crear_args(fd_conexion_memoria,logger_cpu,"Memoria");
    pthread_t hilo_memoria;
    pthread_create(&hilo_memoria,NULL,(void*)atender_conexion,(void *)args_memoria);
    // pthread_detach(hilo_memoria); no hago detach porque me terminaría el programa, terminando mis otros hilos
    pthread_join(hilo_memoria,NULL); // así, la cpu se ejecuta hasta que terminen todos los hilos
    /*

    atender_conexion(fd_conexion_dispatch,logger_cpu,"Kernel-Dispatch");
    //Atender mensajes de Interrupt
    atender_conexion(fd_conexion_dispatch,logger_cpu,"Kernel-Interrupt");
    //Atender mensajes de Memoria
    atender_conexion(fd_conexion_memoria,logger_cpu,"Kernel-Interrupt");
    
    Si solo hiciesemos esto, entra a atender la primera conexion, 
    hasta que no termine de atender todas las peticiones de la primera, 
    no pasa a la segunda

    */


    //Atender mensajes de Dispatch
    
    
    /*
    if(!fd_conexion_client_memoria){
        log_error(logger_cpu,"CPU no se puedo conectar a memoria");
        terminar_programa();
        exit(2);
    }

    enviar_operacion(fd_conexion_client_memoria, handshakeCPU);

    char mensaje[1024] = {0};
    recv(fd_conexion_client_memoria, mensaje, 1024, 0);
    printf("Respuesta del servidor: %s\n", mensaje);    

    
    //ABRIR CPU COMO SERVIDOR DE KERNEL(unico cliente)//
    fd_escucha_cpu = iniciar_servidor(NULL,puerto_propio);
    
    log_info(logger_cpu, "Puerto de CPU habilitado para su UNICO cliente");

    fd_conexion_server_kernel = esperar_cliente(fd_escucha_cpu);

    op_code cod_op = recibir_operacion(fd_conexion_server_kernel);
    if(cod_op == handshakeKERNEL){
        log_info(logger_cpu,"Se conecto KERNEL(unico cliente)");
        enviar_mensaje_de_exito(fd_conexion_server_kernel, "HOLA KERNEL!! SOY CPU");
        }else{
            log_info(logger_cpu,"Codigo de operacion no reconocido en el server");
        }
    
    */
    
    terminar_programa();
    return EXIT_SUCCESS;    
}

void leer__configuraciones(){
    puerto_cpu_dispatch = config_get_string_value(config_cpu,"PUERTO_ESCUCHA_DISPATCH");
    puerto_cpu_interrupt = config_get_string_value(config_cpu,"PUERTO_ESCUCHA_INTERRUPT");
    ip_memoria = config_get_string_value(config_cpu,"IP_MEMORIA");
    puerto_memoria = config_get_string_value(config_cpu,"PUERTO_MEMORIA");
}


void terminar_programa(){
    if(logger_cpu != NULL){
		log_destroy(logger_cpu);
	}
	if(config_cpu != NULL){
		config_destroy(config_cpu);
	}
    liberar_conexion(fd_conexion_memoria);
}

