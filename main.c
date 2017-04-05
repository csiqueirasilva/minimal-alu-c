#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#define BASE_CHAR_CODE 48

#define WRITE_REG_A 1
#define WRITE_REG_B 2
#define READ_REG_A 3
#define READ_REG_B 4
#define READ_FLAGS 5
#define WRITE_OP 6
#define PRINT_OP 7
#define RUN_ALU 8
#define OP_QUIT 9

#define REG_SIZE 8 // bits

#define MAX_NAME_SIZE 8

#define OP_ALU_ADD 1
#define OP_ALU_SUB 2
#define OP_ALU_MULT 3
#define OP_ALU_DIV 4

#define OP_ALU_AND 5
#define OP_ALU_OR 6
#define OP_ALU_NOT 7
#define OP_ALU_XOR 8

struct reg {
#define REGBITSIZE REG_SIZE + 1
	char bits[REGBITSIZE];
	int val;

#define REGNAMESIZE MAX_NAME_SIZE + 1
	char name[REGNAMESIZE];
};

static struct reg * A,
*B,
*IR; // RI, instruction register

static int flagCarry = 0;
static int flagZero = 0;
static int flagOverflow = 0;
static int flagDivisionZero = 0;

/* alguns protótipos, para funções definidas antes dessas */

void OP_add(struct reg * r1, struct reg * r2);
void initFlags(void);
void runALU(void);
void updateValFromBits(struct reg * r);
void updateBitsFromVal(struct reg * r);
int addBit(struct reg * r1, struct reg * r2, int i);
struct reg * createRegister(char * name);
struct reg * cloneRegister(struct reg *);

/* fim de protótipos */

/* funcao de UI */

void clearScreen() {
#ifdef __linux__
	system("clear");
#elif _WIN32
	system("pause");
#endif
}

void pauseInput() {
#ifdef __linux__
	int enter = 0;
	printf("\n\nPressione ENTER para continuar...\n");
	while (enter != '\r' && enter != '\n') {
		enter = getchar();
	}
#elif _WIN32
	system("pause");
#endif
}

/* fim de funcao de UI */

/* ops */

void OP_and(struct reg * r1, struct reg * r2) {
	int i;
	for (i = 0; i < REG_SIZE; i++) {
		r1->bits[i] = r1->bits[i] == r2->bits[i] && r1->bits[i] == '1' ? '1' : '0';
	}
}

void OP_or(struct reg * r1, struct reg * r2) {
	int i;
	for (i = 0; i < REG_SIZE; i++) {
		r1->bits[i] = r2->bits[i] == '1' || r1->bits[i] == '1' ? '1' : '0';
	}
}

void OP_xor(struct reg * r1, struct reg * r2) {
	int i;
	for (i = 0; i < REG_SIZE; i++) {
		r1->bits[i] = r1->bits[i] != r2->bits[i] ? '1' : '0';
	}
}

void OP_compl1(struct reg * r) {
	int i;
	for (i = 0; i < REG_SIZE; i++) {
		r->bits[i] = r->bits[i] == '1' ? '0' : '1';
	}
}

void OP_compl2(struct reg * r) {
	struct reg * compl2 = createRegister("compl2");
	compl2->val = 1;
	updateBitsFromVal(compl2);
	OP_compl1(r);
	OP_add(r, compl2);
	free(compl2);
}

void OP_add(struct reg * r1, struct reg * r2) {
	int i;
	flagCarry = 0;
	flagZero = 1;
	for (i = REG_SIZE - 1; i >= 0; i--) {
		flagZero = !addBit(r1, r2, i) && flagZero;
	}
}

void OP_sub(struct reg * r1, struct reg * r2) {
	struct reg * aux = cloneRegister(r2);
	OP_compl2(aux);
	OP_add(r1, aux);
	free(aux);
}

void OP_div(struct reg *r1, struct reg * r2) {
	int base = abs(r1->val);
	int times = abs(r2->val);
	int sign = (r1->val * r2->val) >= 0 ? 1 : -1;
	struct reg * aux;

	if (r2->val != 0) {
		aux = createRegister("AUX");
		r1->val = 0;
		aux->val = 1;
		updateBitsFromVal(r1);
		updateBitsFromVal(aux);
		flagZero = 1;

		while (base >= times) {
			OP_add(r1, aux);
			base -= times;
		}

		if (sign < 0) {
			OP_compl2(r1);
		}

		free(aux);
	}
}

void OP_mult(struct reg * r1, struct reg * r2) {
	int base = abs(r1->val);
	int times = abs(r2->val);
	int i;
	int sign = (r1->val * r2->val) >= 0 ? 1 : -1;
	struct reg * aux = createRegister("AUX");

	r1->val = 0;
	aux->val = base;
	updateBitsFromVal(r1);
	updateBitsFromVal(aux);
	flagZero = 1;

	for (i = 1; i <= times; i++) {
		OP_add(r1, aux);
	}

	if (sign < 0) {
		OP_compl2(r1);
	}

	free(aux);
}

/* fim de ops */

int addBit(struct reg * r1, struct reg * r2, int i) {
	int sum = (r1->bits[i] - BASE_CHAR_CODE) + (r2->bits[i] - BASE_CHAR_CODE) + flagCarry;
	if (sum > 1) {
		flagCarry = 1;
	}
	else {
		flagCarry = 0;
	}
	r1->bits[i] = (sum % 2) + BASE_CHAR_CODE;
	return r1->bits[i] - BASE_CHAR_CODE;
}

void checkOverflow(int operation) {
	int max = (int) pow(2, REG_SIZE - 1) - 1;
	int min = -(max + 1);

	switch (operation) {
	case OP_ALU_ADD:
		if ((A->val + B->val > max) || (A->val + B->val < min)) {
			flagOverflow = 1;
		}
		break;
	case OP_ALU_MULT:
		if ((A->val * B->val > max) || (A->val * B->val < min)) {
			flagOverflow = 1;
		}
		break;
	case OP_ALU_SUB:
		if ((A->val - B->val > max) || (A->val - B->val < min)) {
			flagOverflow = 1;
		}
		break;
	case OP_ALU_DIV:
		if(B->val == 0) {
			flagDivisionZero = 1;
		} else if ((A->val / B->val > max) || (A->val / B->val < min)) {
			flagOverflow = 1;
		}
		break;
	}

}

void runALU(void) {
	int success = 1;

	initFlags();

	checkOverflow(IR->val);

	switch (IR->val) {
	case OP_ALU_ADD:
		OP_add(A, B);
		break;
	case OP_ALU_SUB:
		OP_sub(A, B);
		break;
	case OP_ALU_MULT:
		OP_mult(A, B);
		break;
	case OP_ALU_DIV:
		OP_div(A, B);
		break;
	case OP_ALU_AND:
		OP_and(A, B);
		break;
	case OP_ALU_OR:
		OP_or(A, B);
		break;
	case OP_ALU_NOT:
		OP_compl1(A);
		break;
	case OP_ALU_XOR:
		OP_xor(A, B);
		break;
	default:
		success = 0;
		printf("\n\nOPERACAO NAO RECONHECIDA\n");
	}

	updateValFromBits(A);

	if (success) {
		printf("\n\nOPERACAO REALIZADA COM SUCESSO\n");
	}

	pauseInput();
}

void initFlags(void) {
	flagOverflow = 0;
	flagZero = 0;
	flagCarry = 0;
	flagDivisionZero = 0;
}

void updateValFromBits(struct reg * r) {
	int i;
	int sign = r->bits[0] == '1' ? -1 : 1;
	r->val = 0;

	if (sign < 0) {
		OP_compl2(r);
	}

	for (i = REG_SIZE - 1; i >= 0; i--) {
		r->val += (int)pow(2, REG_SIZE - 1 - i) * (r->bits[i] - BASE_CHAR_CODE);
	}

	if (sign < 0) {
		r->val *= sign;
		OP_compl2(r);
	}
}

void updateBitsFromVal(struct reg * r) {
	int i;
	int buffer = abs(r->val);
	int sign = r->val < 0 ? -1 : 1;

	for (i = 0; i < REG_SIZE; i++) {
		int bit = (int)pow(2, REG_SIZE - 1 - i);
		if (buffer >= bit) {
			buffer -= bit;
			r->bits[i] = '1';
		}
		else {
			r->bits[i] = '0';
		}
	}

	if (sign < 0) {
		OP_compl2(r);
	}
}

struct reg * cloneRegister(struct reg * r) {
	struct reg * c = createRegister("TEMP");
	strcpy(c->bits, r->bits);
	c->val = r->val;
	return c;
}

struct reg * createRegister(char * name) {
	struct reg * ret = (struct reg *) malloc(sizeof(struct reg));
	if (ret == NULL) {
		printf("falha ao alocar memoria. saindo da aplicacao\n");
		pauseInput();
		exit(-1);
	}
	else {
		int i;
		for (i = 0; i < REG_SIZE; i++) {
			ret->bits[i] = '0';
		}
		ret->bits[REG_SIZE] = 0;
		strcpy(ret->name, name);
		ret->val = 0;
	}

	return ret;
}

void freeAllData(void) {
	free(A);
	free(B);
	free(IR);
}

void readReg(struct reg *r) {
	int maxInput = ((int)pow(2, REG_SIZE) - 1) / 2;
	int minInput = -(maxInput + 1);
	char buffer[REGBITSIZE];
	int i = 0;
	int strSize = 0;
	int zeroSize = 0;
	printf("\nDigite um valor numerico binario, em representacao de complemento a 2, para o registrador %s (MAX : %d, MIN: %d, Tamanho: %d bits) => ", r->name, maxInput, minInput, REG_SIZE);
	scanf("%s", buffer);

	strSize = strlen(buffer);
	zeroSize = REG_SIZE - strSize;
	
	for (i = 0; i < zeroSize; i++) {
		r->bits[i] = '0';
	}

	strcpy(&r->bits[i], buffer);

	updateValFromBits(r);
}

void printReg(struct reg *r) {
	printf("Valor do registrador %s: %d (0b%s)\n\n", r->name, r->val, r->bits);
	pauseInput();
}

void printFlags(void) {
	printf("Carry: %d\nOverflow: %d\nZero: %d\nDivisao por Zero: %d\n\n", flagCarry, flagOverflow, flagZero, flagDivisionZero);
	pauseInput();
}

void printOps(void) {
	printf("\n\
ARITMETICA\n\
1. ADD (A = A + B)\n\
10. SUB (A = A - B)\n\
11. MULT (A = A * B)\n\
100. DIV (A = A / B)\n\
\n\
LOGICA\n\
101. AND (A = A & B)\n\
110. OR (A = A | B)\n\
111. NOT, COMPL1 (A = !A)\n\
1000. XOR (A = A XOR B)\n\
");

}

void operacao(int op) {
	switch (op) {
	case WRITE_REG_A:
		readReg(A);
		break;
	case WRITE_REG_B:
		readReg(B);
		break;
	case READ_REG_A:
		printReg(A);
		break;
	case READ_REG_B:
		printReg(B);
		break;
	case READ_FLAGS:
		printFlags();
		break;
	case WRITE_OP:
		printOps();
		readReg(IR);
		break;
	case PRINT_OP:
		printOps();
		printReg(IR);
		break;
	case RUN_ALU:
		runALU();
		break;
	case OP_QUIT:
		break;
	default:
		printf("\nOPCAO INVALIDA\n");
		pauseInput();
	}
}

int menu(void) {

	int op = 0;

	clearScreen();
	printf(
		"Menu Principal da ULA\n\
\n\
\t1. Definir registrador A\n\
\t2. Definir registrador B\n\
\t3. Ler registrador A (Acc)\n\
\t4. Ler registrador B\n\
\t5. Ler registrador de flags\n\
\t6. Definir operacao\n\
\t7. Ler operacao (OPCODE)\n\
\t8. Executar ULA\n\
\t9. Sair\n\
\n\
Escolha a opcao => ");

	scanf("%d", &op);

	return op;
}

void main(void) {

	int op = -1;

	A = createRegister("A");
	B = createRegister("B");
	IR = createRegister("OPCODE");

	while (op != OP_QUIT) {
		op = menu();
		operacao(op);
	}

	freeAllData();
}