#include "usr/include/stdio.h"
#include "usr/include/stdlib.h"
#include "usr/include/sys.h"
#include "usr/include/string.h"

// Структура для хранения переменных
#define MAX_VARS 26
int variables[MAX_VARS]; // Переменные A-Z

// Структура для хранения строк программы
#define MAX_LINES 100
#define MAX_LINE_LENGTH 80
struct Line {
    int number;
    char code[MAX_LINE_LENGTH];
};
struct Line program[MAX_LINES];
int programSize = 0;

// Структура для стека циклов FOR
#define MAX_FOR_STACK 10
struct ForState {
    int var;    // Индекс переменной (A-Z)
    int end;    // Конечное значение
    int step;   // Шаг
    int returnPc; // Номер строки для возврата (после FOR)
};
struct ForState forStack[MAX_FOR_STACK];
int forStackTop = -1;

// Прототипы функций
int findLine(int lineNum);
void executeLine(char *line, int *pc);
int evaluateExpression(char *expr);

// Утилита для пропуска пробелов
void skipWhitespace(char **str) {
    while (isspace(**str)) (*str)++;
}

// Парсинг числа
int parseNumber(char **str) {
    int num = 0;
    skipWhitespace(str);
    while (isdigit(**str)) {
        num = num * 10 + (**str - '0');
        (*str)++;
    }
    return num;
}

int main() {
    char input[MAX_LINE_LENGTH];
    printf("MountainOS Basic v0.1\n");
    printf("Enter program lines (end with RUN or empty line):\n");

    // Загрузка программы
    while (1) {
        fgets(input, MAX_LINE_LENGTH, stdin);
        
        input[strcspn(input, "\n")] = 0; // Удаляем \n

        if (strlen(input) == 0 || strcmp(input, "RUN") == 0) break;

        int lineNum = atoi(input);
        if (lineNum > 0 && programSize < MAX_LINES) {
            program[programSize].number = lineNum;
            strcpy(program[programSize].code, input + (lineNum > 9 ? 3 : 2));
            programSize++;
        }
    }

    // Сортировка строк по номеру
    for (int i = 0; i < programSize - 1; i++) {
        for (int j = 0; j < programSize - i - 1; j++) {
            if (program[j].number > program[j + 1].number) {
                struct Line temp = program[j];
                program[j] = program[j + 1];
                program[j + 1] = temp;
            }
        }
    }

    // Выполнение программы
    int pc = 0; // Program counter
    while (pc < programSize) {
        executeLine(program[pc].code, &pc);
        pc++;
    }

    return 0;
}

// Поиск строки по номеру
int findLine(int lineNum) {
    for (int i = 0; i < programSize; i++) {
        if (program[i].number == lineNum) return i;
    }
    return -1;
}

// Вычисление выражения (только + и -)
int evaluateExpression(char *expr) {
    char *ptr = expr;
    int result = 0;

    skipWhitespace(&ptr);
    if (isalpha(*ptr)) {
        result = variables[*ptr - 'A'];
        ptr++;
    } else {
        result = parseNumber(&ptr);
    }

    while (*ptr) {
        skipWhitespace(&ptr);
        if (*ptr == '+') {
            ptr++;
            skipWhitespace(&ptr);
            if (isalpha(*ptr)) {
                result += variables[*ptr - 'A'];
                ptr++;
            } else {
                result += parseNumber(&ptr);
            }
        } else if (*ptr == '-') {
            ptr++;
            skipWhitespace(&ptr);
            if (isalpha(*ptr)) {
                result -= variables[*ptr - 'A'];
                ptr++;
            } else {
                result -= parseNumber(&ptr);
            }
        } else if (*ptr == '*') {
            ptr++;
            skipWhitespace(&ptr);
            if (isalpha(*ptr)) {
                result *= variables[*ptr - 'A'];
                ptr++;
            } else {
                result *= parseNumber(&ptr);
            }
        } else if (*ptr == '/') {
            ptr++;
            skipWhitespace(&ptr);
            if (isalpha(*ptr)) {
                result /= variables[*ptr - 'A'];
                ptr++;
            } else {
                result /= parseNumber(&ptr);
            }
        } else {
            break;
        }
    }
    return result;
}

// Выполнение одной строки
void executeLine(char *line, int *pc) {
    char command[MAX_LINE_LENGTH];
    char *ptr = line;
    skipWhitespace(&ptr);
    sscanf(ptr, "%s", command);

    if (strcmp(command, "PRINT") == 0) {
        ptr += strlen("PRINT");
        skipWhitespace(&ptr);
        if (*ptr == '"') {
            ptr++;
            while (*ptr != '"') {
                putchar(*ptr);
                ptr++;
            }
            putchar('\n');
        } else {
            printf("%d\n", evaluateExpression(ptr));
        }
    } else if (strcmp(command, "LET") == 0) {
        ptr += strlen("LET");
        skipWhitespace(&ptr);
        int var = *ptr - 'A';
        ptr++;
        skipWhitespace(&ptr);
        if (*ptr == '=') {
            ptr++;
            skipWhitespace(&ptr);
            variables[var] = evaluateExpression(ptr);
        }
    } else if (strcmp(command, "INPUT") == 0) {
        ptr += strlen("INPUT");
        skipWhitespace(&ptr);
        int var = *ptr - 'A';
        scanf("%d", &variables[var]);
    } else if (strcmp(command, "IF") == 0) {
        ptr += strlen("IF");
        skipWhitespace(&ptr);
        char expr1[20], op[3], expr2[20];
        int lineNum;
        sscanf(ptr, "%s %s %s GOTO %d", expr1, op, expr2, &lineNum);

        int val1 = evaluateExpression(expr1);
        int val2 = evaluateExpression(expr2);
        int condition = 0;

        if (strcmp(op, "=") == 0) condition = (val1 == val2);
        else if (strcmp(op, "<") == 0) condition = (val1 < val2);
        else if (strcmp(op, ">") == 0) condition = (val1 > val2);

        if (condition) {
            int newPc = findLine(lineNum);
            if (newPc != -1) *pc = newPc - 1;
        }
    } else if (strcmp(command, "GOTO") == 0) {
        ptr += strlen("GOTO");
        int lineNum = parseNumber(&ptr);
        int newPc = findLine(lineNum);
        if (newPc != -1) *pc = newPc - 1;
    } else if (strcmp(command, "FOR") == 0) {
        ptr += strlen("FOR");
        skipWhitespace(&ptr);
        int var = *ptr - 'A'; // Переменная цикла
        ptr++;
        skipWhitespace(&ptr);
        if (*ptr == '=') {
            ptr++;
            skipWhitespace(&ptr);
            int start = evaluateExpression(ptr); // Начальное значение
            variables[var] = start;

            while (*ptr && strncmp(ptr, "TO", 2) != 0) ptr++; // Ищем "TO"
            if (strncmp(ptr, "TO", 2) == 0) {
                ptr += 2;
                skipWhitespace(&ptr);
                int end = evaluateExpression(ptr); // Конечное значение

                // По умолчанию шаг = 1
                int step = 1;

                // Сохраняем состояние цикла в стеке
                if (forStackTop < MAX_FOR_STACK - 1) {
                    forStackTop++;
                    forStack[forStackTop].var = var;
                    forStack[forStackTop].end = end;
                    forStack[forStackTop].step = step;
                    forStack[forStackTop].returnPc = *pc;
                }
            }
        }
    } else if (strcmp(command, "NEXT") == 0) {
        ptr += strlen("NEXT");
        skipWhitespace(&ptr);
        int var = *ptr - 'A';

        if (forStackTop >= 0 && forStack[forStackTop].var == var) {
            variables[var] += forStack[forStackTop].step; // Увеличиваем переменную
            if (variables[var] <= forStack[forStackTop].end) {
                // Возвращаемся к строке после FOR
                *pc = forStack[forStackTop].returnPc;
            } else {
                // Завершаем цикл, убираем его из стека
                forStackTop--;
            }
        }
    } else {
        
    }
}
