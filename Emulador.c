#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Tipos
typedef unsigned char byte; // 8 bits
typedef unsigned int palavra; // 32 bits
typedef unsigned long int microinstrucao; // 64 bits, no caso de acordo com a arquitetura cada microinstrução usa apenas 36 bits

// Registradores
palavra MAR = 0, MDR = 0, PC = 0; // Acesso à Memória
byte MBR = 0;                     // Acesso à Memória

palavra SP = 0, LV = 0, TOS = 0, OPC = 0, CPP = 0, H = 0; // Operação da ULA

microinstrucao MIR; // Contém a Microinstrução Atual
palavra MPC = 0;    // Contém o endereço para a próxima Microinstrução

// Barramentos
palavra Barramento_B, Barramento_C;

// Flip-Flops
byte N, Z;

// Auxiliares para Decodificar Microinstrução
byte MIR_B, MIR_Operacao, MIR_Deslocador, MIR_MEM, MIR_pulo;
palavra MIR_C;

// Armazenamento de Controle
microinstrucao Armazenamento[512]; // SUGESTÃO: Usar um define para o tamanho da memória de controle

// Memória Principal
byte Memoria[100000000]; // SUGESTÃO: Usar define para facilitar manutenção

// Protótipos das Funções
void carregar_microprogram_de_controle();
void carregar_programa(const char *programa);
void exibir_processos();
void decodificar_microinstrucao();
void atribuir_barramento_B();
void realizar_operacao_ALU();
void atribuir_barramento_C();
void operar_memoria();
void pular();
void binario(void* valor, int tipo);

// Laço Principal
int main(int argc, const char *argv[]) {
    if (argc < 2) {
        printf("Uso: %s <arquivo_programa>\n", argv[0]); // SUGESTÃO: Checar se o nome do programa foi passado
        return 1;
    }

    carregar_microprogram_de_controle();
    carregar_programa(argv[1]);

    while (1) {
        exibir_processos();
        MIR = Armazenamento[MPC];

        decodificar_microinstrucao();
        atribuir_barramento_B();
        realizar_operacao_ALU();
        atribuir_barramento_C();
        operar_memoria();
        pular();
    }

    return 0;
}

// Implementação das Funções
void carregar_microprogram_de_controle() {
    FILE* MicroPrograma = fopen("microprog.rom", "rb");

    if (MicroPrograma != NULL) {
        fread(Armazenamento, sizeof(microinstrucao), 512, MicroPrograma);
        fclose(MicroPrograma);
    } else {
        printf("Erro ao abrir microprog.rom\n"); // SUGESTÃO: Tratamento de erro para abertura do arquivo
        exit(1);
    }
}

void carregar_programa(const char* prog) {
    FILE* Programa = fopen(prog, "rb");
    palavra tamanho;
    byte tamanho_temp[4];

    if (Programa != NULL) {
        fread(tamanho_temp, sizeof(byte), 4, Programa);
        memcpy(&tamanho, tamanho_temp, 4);

        fread(Memoria, sizeof(byte), 20, Programa);
        fread(&Memoria[0x0401], sizeof(byte), tamanho - 20, Programa);
        fclose(Programa);
    } else {
        printf("Erro ao abrir arquivo do programa: %s\n", prog); // SUGESTÃO: Mensagem de erro informativa
        exit(1);
    }
}

void decodificar_microinstrucao() {
    MIR_B = MIR & 0b1111;
    MIR_MEM = (MIR >> 4) & 0b111;
    MIR_C = (MIR >> 7) & 0b111111111;
    MIR_Operacao = (MIR >> 16) & 0b111111;
    MIR_Deslocador = (MIR >> 22) & 0b11;
    MIR_pulo = (MIR >> 24) & 0b111;
    MPC = (MIR >> 27) & 0b111111111;
}

void atribuir_barramento_B() {
    switch (MIR_B) {
        case 0: Barramento_B = MDR; break;
        case 1: Barramento_B = PC; break;
        case 2:
            Barramento_B = MBR;
            if (MBR & (0b10000000))
                Barramento_B |= (0xFFFFFF << 8); // SUGESTÃO: usar máscara hexadecimal legível
            break;
        case 3: Barramento_B = MBR; break;
        case 4: Barramento_B = SP; break;
        case 5: Barramento_B = LV; break;
        case 6: Barramento_B = CPP; break;
        case 7: Barramento_B = TOS; break;
        case 8: Barramento_B = OPC; break;
        default:
            Barramento_B = 0;
            printf("Aviso: Valor MIR_B inválido (%d)\n", MIR_B); // SUGESTÃO: Diagnóstico para valor inesperado
            break;
    }
}

void realizar_operacao_ALU() {
    switch (MIR_Operacao) {
        case 12: Barramento_C = H & Barramento_B; break;
        case 17: Barramento_C = 1; break;
        case 18: Barramento_C = -1; break;
        case 20: Barramento_C = Barramento_B; break;
        case 24: Barramento_C = H; break;
        case 26: Barramento_C = ~H; break;
        case 28: Barramento_C = H | Barramento_B; break;
        case 44: Barramento_C = ~Barramento_B; break;
        case 53: Barramento_C = Barramento_B + 1; break;
        case 54: Barramento_C = Barramento_B - 1; break;
        case 57: Barramento_C = H + 1; break;
        case 59: Barramento_C = -H; break;
        case 60: Barramento_C = H + Barramento_B; break;
        case 61: Barramento_C = H + Barramento_B + 1; break;
        case 63: Barramento_C = Barramento_B - H; break;
        default:
            printf("Aviso: Código de operação ULA inválido: %d\n", MIR_Operacao); // SUGESTÃO: Tratamento de operação inválida
            break;
    }

    Z = Barramento_C ? 1 : 0;
    N = Barramento_C ? 0 : 1;

    switch (MIR_Deslocador) {
        case 1: Barramento_C <<= 8; break;
        case 2: Barramento_C >>= 1; break;
        // SUGESTÃO: caso 0 e default explícitos podem tornar o código mais robusto
    }
}

void atribuir_barramento_C() {
    if (MIR_C & 0b000000001) MAR = Barramento_C;
    if (MIR_C & 0b000000010) MDR = Barramento_C;
    if (MIR_C & 0b000000100) PC  = Barramento_C;
    if (MIR_C & 0b000001000) SP  = Barramento_C;
    if (MIR_C & 0b000010000) LV  = Barramento_C;
    if (MIR_C & 0b000100000) CPP = Barramento_C;
    if (MIR_C & 0b001000000) TOS = Barramento_C;
    if (MIR_C & 0b010000000) OPC = Barramento_C;
    if (MIR_C & 0b100000000) H   = Barramento_C;
}

void operar_memoria() {
    if (MIR_MEM & 0b001) MBR = Memoria[PC]; // SUGESTÃO: validar se PC está dentro do tamanho da memória
    if (MIR_MEM & 0b010) memcpy(&MDR, &Memoria[MAR * 4], 4);
    if (MIR_MEM & 0b100) memcpy(&Memoria[MAR * 4], &MDR, 4);
}

void pular() {
    if (MIR_pulo & 0b001) MPC |= (N << 8);
    if (MIR_pulo & 0b010) MPC |= (Z << 8);
    if (MIR_pulo & 0b100) MPC |= MBR;
}

void exibir_processos() {
    if (LV && SP) {
        printf("\t\t  PILHA DE OPERANDOS\n");
        printf("========================================\n");
        printf("     END\t   BINARIO DO VALOR\t\tVALOR\n");
        for (int i = SP; i >= LV; i--) {
            palavra valor;
            memcpy(&valor, &Memoria[i * 4], 4);

            if (i == SP) printf("SP ->");
            else if (i == LV) printf("LV ->");
            else printf("     ");

            printf("%X ", i);
            binario(&valor, 1); printf(" ");
            printf("%d\n", valor);
        }
        printf("========================================\n");
    }

    if (PC >= 0x0401) {
        printf("\n\t\t\tÁrea do Programa\n");
        printf("========================================\n");
        printf("\t\tBinário\t HEX  END DE BYTE\n");
        for (int i = PC - 2; i <= PC + 3; i++) {
            if (i == PC) printf("Em execução >>  ");
            else printf("\t\t");

            binario(&Memoria[i], 2);
            printf(" 0x%02X \t%X\n", Memoria[i], i);
        }
        printf("========================================\n\n");
    }

    printf("\t\tREGISTRADORES\n");
    printf("\tBINARIO\t\t\t\tHEX\n");
    printf("MAR: "); binario(&MAR, 3); printf("\t%x\n", MAR);
    printf("MDR: "); binario(&MDR, 3); printf("\t%x\n", MDR);
    printf("PC:  "); binario(&PC, 3);  printf("\t%x\n", PC);
    printf("MBR: \t\t"); binario(&MBR, 2); printf("\t\t%x\n", MBR);
    printf("SP:  "); binario(&SP, 3);  printf("\t%x\n", SP);
    printf("LV:  "); binario(&LV, 3);  printf("\t%x\n", LV);
    printf("CPP: "); binario(&CPP, 3); printf("\t%x\n", CPP);
    printf("TOS: "); binario(&TOS, 3); printf("\t%x\n", TOS);
    printf("OPC: "); binario(&OPC, 3); printf("\t%x\n", OPC);
    printf("H:   "); binario(&H, 3);   printf("\t%x\n", H);
    printf("MPC: \t\t"); binario(&MPC, 5); printf("\t%x\n", MPC);
    printf("MIR: "); binario(&MIR, 4); printf("\n");

    getchar(); // SUGESTÃO: Ativar apenas em modo debug
}

void binario(void* valor, int tipo) {
    switch (tipo) {
        case 1: {
            printf(" ");
            byte* v = (byte*)valor;
            for (int i = 3; i >= 0; i--) {
                for (int j = 0; j < 8; j++) {
                    printf("%d", (v[i] >> (7 - j)) & 1);
                }
                printf(" ");
            }
        } break;
        case 2: {
            byte v = *((byte*)valor);
            for (int j = 0; j < 8; j++) {
                printf("%d", (v >> (7 - j)) & 1);
            }
        } break;
        case 3: {
            palavra v = *((palavra*)valor);
            for (int j = 0; j < 32; j++) {
                printf("%d", (v >> (31 - j)) & 1);
            }
        } break;
        case 4: {
            microinstrucao v = *((microinstrucao*)valor);
            for (int j = 0; j < 36; j++) {
                if (j == 9 || j == 12 || j == 20 || j == 29 || j == 32) printf(" ");
                printf("%ld", (v >> (35 - j)) & 1);
            }
        } break;
        case 5: {
            palavra v = *((palavra*)valor) << 23;
            for (int j = 0; j < 9; j++) {
                printf("%d", (v >> 31) & 1);
                v <<= 1;
            }
        } break;
    }
}
