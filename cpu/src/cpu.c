#include <../../cpu/include/cpu.h>

int main(int argc, char* argv[]) {
    decir_hola("CPU");
    //CREO CONEXION CPU A MEMORIA Y RECIBO MENSAJE//

    logger_cpu = log_create("cpu_logs.log","cpu",1,LOG_LEVEL_INFO);
    config_cpu = config_create("./configs/cpu.config");

    leer__configuraciones();
    fd_conexion_client_memoria= crear_conexion(ip_memoria,puerto_memoria);

    if(!fd_conexion_client_memoria){
        log_error(logger_cpu,"CPU no se puedo conectar a memoria");
        terminar_programa();
        exit(2);
    }

    enviar_operacion(fd_conexion_client_memoria, &handshakeDeCPU);

    char mensaje[1024] = {0};
    recv(fd_conexion_client_memoria, mensaje, 1024, 0);
    printf("Respuesta del servidor: %s\n", mensaje);    

    

    //ABRIR CPU COMO SERVIDOR DE KERNEL(unico cliente)//
    fd_escucha_cpu = iniciar_servidor(NULL,puerto_propio);
    
    log_info(logger_cpu, "Puerto de CPU habilitado para su UNICO cliente");

    fd_conexion_server_kernel = esperar_cliente(fd_escucha_cpu);

    int cod_op = recibir_operacion(fd_conexion_server_kernel);
    if(cod_op == 5){
        log_info(logger_cpu,"Se conecto KERNEL(unico cliente)");
        enviar_mensaje_de_exito(fd_conexion_server_kernel, "HOLA KERNEL!! SOY CPU");
        }else{
            log_info(logger_cpu,"Codigo de operacion no reconocido en el server");
        }
    
    terminar_programa();
    return EXIT_SUCCESS;    
}

void leer__configuraciones(){
    puerto_propio = config_get_string_value(config_cpu,"PUERTO_PROPIO");
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
    liberar_conexion(fd_conexion_client_memoria);
}