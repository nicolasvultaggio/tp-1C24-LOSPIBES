#include <../../cpu/include/cpu.h>

int main(int argc, char* argv[]) {
    decir_hola("CPU");
    //CREO CONEXION CPU A MEMORIA Y RECIBO MENSAJE//

    logger_cpu = log_create("cpu_logs.log","cpu",1,LOG_LEVEL_INFO);
    config_cpu = config_create("./configs/cpu.config");

    leer__configuraciones();
    socket_cliente= crear_conexion(ip__memoria,puerto__memoria);

    if(!socket_cliente){
        log_error(logger_cpu,"CPU no se puedo conectar a memoria");
        terminar_programa();
        exit(2);
    }

    enviar_operacion(socket_cliente, &handshakeDeCPU);

    char mensaje1[1024] = {0};
    recv(socket_cliente, mensaje1, 1024, 0);
    printf("Respuesta del servidor: %s\n", mensaje1);    

    

    //ABRIR CPU COMO SERVIDOR DE KERNEL(unico cliente)//
    int socket_server_CPU = iniciar_servidor(NULL,puerto__propio);
    
    log_info(logger_cpu, "Puerto de CPU habilitado para su UNICO cliente");

    socket_cliente_delKERNEL = esperar_cliente(socket_server_CPU);

    int cod_op = recibir_operacion(socket_cliente_delKERNEL);
    if(cod_op == 5){
        log_info(logger_cpu,"Se conecto KERNEL(unico cliente)");
        enviar_mensaje_de_exito(socket_cliente_delKERNEL, "HOLA KERNEL!! SOY CPU");
        }else{
            log_info(logger_cpu,"Codigo de operacion no reconocido en el server");
        }
    
    terminar_programa();
    return EXIT_SUCCESS;    
}

void leer__configuraciones(){
    puerto__propio = config_get_string_value(config_cpu,"PUERTO__ESCUCHA");
    ip__memoria = config_get_string_value(config_cpu,"IP_MEMORIA");
    puerto__memoria = config_get_string_value(config_cpu,"PUERTO_MEMORIA");
   }


void terminar_programa(){
    if(logger_cpu != NULL){
		log_destroy(logger_cpu);
	}
	if(config_cpu != NULL){
		config_destroy(config_cpu);
	}
    close(socket_cliente);
}