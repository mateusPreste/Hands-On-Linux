#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/stat.h>

// Definimos o caminho temporário para o mock
#define MOCK_DIR "/tmp/smartlamp"
#define LDR_FILE "mock_sysfs/ldr_value"

int main() {
    // inicializa o gerador de números aleatórios
    srand(time(NULL));

    // cria o diretório (0777 dá permissões totais). 
    // se o diretório já existir, o mkdir falha.
    mkdir(MOCK_DIR, 0777);

    printf("Mock do SmartLamp iniciado.\n");
    printf("Simulando sensor LDR em: %s\n", LDR_FILE);
    printf("Pressione Ctrl+C para encerrar.\n\n");

    // loop infinito pra simular o ldr
    while(1) {
        // abre o arquivo em modo de escrita ("w"), o que sobrescreve o valor antigo
        FILE *file = fopen(LDR_FILE, "w");
        
        if (file != NULL) {
            // Simula um valor de ADC de 12 bits do ESP32 (0 a 4095)
            int ldr_value = rand() % 4096; 
            
            // Escreve o valor no arquivo
            fprintf(file, "%d\n", ldr_value);
            fclose(file); // É crucial fechar o arquivo para salvar as alterações no disco
            
            printf("[Mock] Valor do LDR atualizado: %d\n", ldr_value);
        } else {
            perror("Erro ao criar o arquivo mock :(");
            return 1;
        }

        sleep(1); 
    }

    return 0;
}
