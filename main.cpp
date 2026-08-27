#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <sys/select.h>
#include <ctype.h>

//информационное поле
struct flat {
    float area; //площадь квартиры
    short n, house, flat, residents; //порядковый номер записи, номер дома, номер квартиры, количество жильцов.
    char surname[20], name[20], patronymic[20], street[30]; //фио владельца и название улицы
};

//двунаправленный список
struct bi_list {
    flat inf; //информационное поле
    bi_list * l, * r; //указатели на предыдущий и следующий элементы
};

//производная структура
struct pair {
    bi_list* beg, * end; //указатели на начало и конец списка
};

//однонаправленный список
struct list {
    flat inf; //информационное поле
    list* next; //указатель на следующий элемент
};

//двунаправленный список с вложенным однонаправленным списком
struct street {
    char inf[30]; //название улицы
    short page, n; //страница просмотра, количество элементов вложенного списка
    list* top; //указатель на начало вложенного списка
    street* l, * r; //указатели на предыдущий и следующий элементы
};

const int size_flat = sizeof(flat);
const int size_bi_list = sizeof(bi_list);
const int size_list = sizeof(list);
const int size_street = sizeof(street);

// === Управление терминалом ===

static struct termios orig_termios;

static void enter_raw() {
    struct termios raw = orig_termios;
    raw.c_lflag &= ~(ICANON | ECHO);
    raw.c_iflag &= ~ICRNL; // не переводить CR→LF, Enter будет слать 13
    raw.c_cc[VMIN] = 1;
    raw.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

static void enter_cooked() {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
}

static void cleanup_terminal() {
    enter_cooked();
    printf("\033[0m\n");
    fflush(stdout);
}

// Читает один символ в raw-режиме. Стрелки возвращаются как коды Windows (72/80).
// Использует read() напрямую — getchar() буферизует байты в stdio и select()
// не видит хвост escape-последовательности стрелки.
int _getch() {
    unsigned char ch;
    if (read(STDIN_FILENO, &ch, 1) <= 0) return -1;
    if (ch == 27) {
        fd_set fds;
        struct timeval tv = {0, 50000}; // 50мс
        FD_ZERO(&fds);
        FD_SET(STDIN_FILENO, &fds);
        if (select(STDIN_FILENO + 1, &fds, NULL, NULL, &tv) > 0) {
            unsigned char c2;
            if (read(STDIN_FILENO, &c2, 1) > 0 && c2 == '[') {
                unsigned char c3;
                if (read(STDIN_FILENO, &c3, 1) > 0) {
                    if (c3 == 'A') return 72; // стрелка вверх
                    if (c3 == 'B') return 80; // стрелка вниз
                    if (c3 == 'C') return 77; // стрелка вправо
                    if (c3 == 'D') return 75; // стрелка влево
                }
            }
        }
        return 27; // ESC
    }
    return ch;
}

static void cls() {
    system("clear");
}

// Читает строку в raw-режиме с эхом. ESC → возвращает true (отмена), Enter → false (ОК).
// Поддерживает Backspace и многобайтовый UTF-8 (кириллица).
static bool raw_readline(char* buf, int maxbytes) {
    int len = 0;
    buf[0] = '\0';
    while (true) {
        unsigned char c;
        if (read(STDIN_FILENO, &c, 1) <= 0) return true;
        if (c == 27) {
            // Проверяем: escape-последовательность (стрелка) или настоящий ESC
            fd_set fds;
            struct timeval tv = {0, 50000};
            FD_ZERO(&fds);
            FD_SET(STDIN_FILENO, &fds);
            if (select(STDIN_FILENO + 1, &fds, NULL, NULL, &tv) > 0) {
                unsigned char b;
                read(STDIN_FILENO, &b, 1);
                if (b == '[') { unsigned char b2; read(STDIN_FILENO, &b2, 1); }
                continue; // стрелки и прочие escape-seq игнорируем
            }
            return true; // настоящий ESC — отмена
        }
        if (c == 13 || c == 10) return false; // Enter — подтверждение
        if (c == 127 || c == 8) { // Backspace
            if (len > 0) {
                len--;
                while (len > 0 && ((unsigned char)buf[len] & 0xC0) == 0x80)
                    len--;
                buf[len] = '\0';
                printf("\b \b"); // стираем 1 символ (любой, в т.ч. кириллицу — 1 колонка)
                fflush(stdout);
            }
            continue;
        }
        if (c < 32) continue;
        int seqlen = (c >= 0xF0) ? 4 : (c >= 0xE0) ? 3 : (c >= 0xC0) ? 2 : 1;
        if (len + seqlen < maxbytes) {
            int start = len;
            buf[len++] = c;
            bool ok = true;
            for (int i = 1; i < seqlen; i++) {
                unsigned char cont;
                if (read(STDIN_FILENO, &cont, 1) > 0)
                    buf[len++] = cont;
                else { ok = false; break; }
            }
            if (!ok) { len = start; continue; } // откат при неполной последовательности
            buf[len] = '\0';
            write(STDOUT_FILENO, buf + start, seqlen);
            fflush(stdout);
        }
    }
}

// Возвращает количество «лишних» байт UTF-8 строки относительно отображаемой ширины.
// Считаем только ведущие байты многобайтовых последовательностей:
//   2-байтный символ (0xC0–0xDF): +1 лишний байт  ← кириллица всегда здесь
//   3-байтный (0xE0–0xEF): +2, 4-байтный (0xF0+): +3
static int utf8_extra(const char* s) {
    int extra = 0;
    while (*s) {
        unsigned char c = (unsigned char)*s;
        if      (c >= 0xF0) extra += 3;
        else if (c >= 0xE0) extra += 2;
        else if (c >= 0xC0) extra += 1;
        // 0x80–0xBF — байты-продолжения, не считаем
        s++;
    }
    return extra;
}

static void color_normal()    { printf("\033[0m");         fflush(stdout); }
static void color_highlight() { printf("\033[7m");         fflush(stdout); }
static void color_error()     { printf("\033[41m\033[30m"); fflush(stdout); }

// === Прототипы ===

void make_good_console();
void indent();
short run_menu(char** menu, short n, bi_list* top = NULL, bool k = 0, short position = 0, short page = 0, short max_page = 0, const char* s = "", street* top_str = NULL);
void error(const char *s);
void request_empty_bi_list(bi_list* beg);
void empty_bi_list(bi_list* beg);
void empty_list(list* top);
void empty_str(street* top);
void skip(FILE* f);
void write_top_table(const char *s = NULL);
void write(flat fl);
void refresh_register(char* s);
bool read_way(const char* message, char* s);
bool read_str(const char* message, char* s);
bool read_str_for_street(const char* message, char* s);
bool read_short(const char* s, short& n);
bool read_float(const char* s, float& result);
flat read(bool& p);
flat read_from_text(FILE* f);
void refresh(bi_list* beg);
bi_list* organize(bi_list* beg, short& n);
bi_list* add(bi_list* end, short& k);
void browse(bi_list* beg, short n);
pair del(bi_list* beg, bi_list* end, short& n);
void correct(bi_list* beg, short n);
pair sort_process(bi_list* beg, short d);
pair sort(bi_list* beg);
void search(bi_list* beg);
bool define(char* s);
void save(bi_list* beg);
pair load(bi_list* beg, short& k);
list* insert_str(list* top, flat inf);
short write_str(list* top);
void write_top_str();
void space_write_str();
street* new_street(flat fl);
short count_page_str(street* top);
void browse_str(street* top_str, short n);
void create_street(bi_list* beg);

// === Реализации ===

int main() {
    pair back;
    bi_list * beg = NULL, *end = NULL;
    char** menu;
    short n = 0, c, i;
    make_good_console();
    menu = new char* [11];
    for (i = 0; i < 11; i++)
        menu[i] = new char[110];
    strcpy(menu[0], "Создать таблицу");
    strcpy(menu[1], "Просмотр");
    strcpy(menu[2], "Добавить новую запись");
    strcpy(menu[3], "Удалить запись");
    strcpy(menu[4], "Редактировать запись");
    strcpy(menu[5], "Сортировка записей по фамилии");
    strcpy(menu[6], "Поиск записей по названию улицы");
    strcpy(menu[7], "Сохранить таблицу");
    strcpy(menu[8], "Загрузить таблицу");
    strcpy(menu[9], "Поиск пяти самых заселённых квартир на каждой улице");
    strcpy(menu[10], "Выход из программы");

    while (true) {
        c = run_menu(menu, 11);
        cls();
        if (n == 0)
            if (c != 0 && c != 8 && c != 27 && c != 10) {
                error("Таблица пуста! Для начала работы с таблицей нужно её создать или загрузить!");
                continue;
            }
        switch (c) {
        case 0:
            beg = organize(beg, n);
            if (n == 1)
                end = beg;
            break;
        case 1:
            browse(beg, n);
            break;
        case 2:
            end = add(end, n);
            break;
        case 3:
            back = del(beg, end, n);
            beg = back.beg;
            end = back.end;
            refresh(beg);
            break;
        case 4:
            correct(beg, n);
            break;
        case 5:
            back = sort(beg);
            if (back.end) {
                beg = back.beg;
                end = back.end;
                refresh(beg);
            }
            break;
        case 6:
            search(beg);
            break;
        case 7:
            save(beg);
            break;
        case 8:
            back = load(beg, n);
            if (back.beg != NULL) {
                beg = back.beg;
                end = back.end;
            }
            break;
        case 9:
            create_street(beg);
            break;
        default:
            if (beg)
                request_empty_bi_list(beg);
            return 1;
        }
    }
}

void make_good_console() {
    tcgetattr(STDIN_FILENO, &orig_termios);
    atexit(cleanup_terminal);
    enter_raw();
    cls();
}

void indent() {
    printf("         ");
}

short run_menu(char** menu, short n, bi_list *top, bool k, short position, short page, short max_page, const char *s, street *top_str) {
    bi_list* t;
    street* t_str;
    short i, c, j;
    while (true) {
        cls();
        if (top_str) {
            write_top_str();
            i = 0;
            for (t_str = top_str; t_str->page == page; t_str = t_str->r) {
                i += write_str(t_str->top);
                if (t_str->r == NULL)
                    break;
            }
            indent();
            printf("Страница %hi из %hi\n", page, max_page);
            for (; i < 9; i++)
                puts("");
        }
        else if (top == NULL) {
            for (i = 0; i < 10; i++)
                puts("");
            if (s[0] != '\0') {
                for (j = 0; j < 7; j++)
                    printf("          ");
                puts(s);
            }
            puts("");
        }
        else {
            if (k == 1) {
                write_top_table("Таблица");
                i = 0;
                for (t = top; i < 10; t = t->r) {
                    write(t->inf);
                    i++;
                    if (t->r == NULL)
                        break;
                }
                indent();
                printf("Страница %hi из %hi\n", page, max_page);
                for (; i < 5; i++)
                    puts("");
            }
            else {
                if (s[0] == '\0')
                    write_top_table("Запись");
                else
                    write_top_table(s);
                write(top->inf);
            }
            puts("");
        }
        for (i = 0; i < n; i++) {
            for (j = 0; j < 6; j++)
                printf("          ");
            if (position == i) {
                printf("        ->");
                color_highlight();
                puts(menu[i]);
                color_normal();
            }
            else {
                printf("          ");
                puts(menu[i]);
            }
            puts("");
        }
        fflush(stdout);
        // Ждём стрелку, Enter или ESC; остальные клавиши игнорируем
        while ((c = _getch()) != 13 && c != 27 && c != 72 && c != 80)
            ;
        if (c == 80) {
            if (position < n - 1)
                position++;
            else
                position = 0;
        }
        else if (c == 72) {
            if (position > 0)
                position--;
            else
                position = n - 1;
        }
        else if (c == 13)
            return position;
        else if (c == 27)
            return 27;
    }
    return 0;
}

void error(const char* s) {
    cls();
    color_error();
    puts(s);
    color_normal();
    fflush(stdout);
    printf("Нажмите любую клавишу...\n");
    _getch();
    cls();
}

void request_empty_bi_list(bi_list* beg) {
    char** menu;
    int c;

    menu = new char* [2];
    for (c = 0; c < 2; c++)
        menu[c] = new char[20];
    strcpy(menu[0], "Да");
    strcpy(menu[1], "Нет");

    c = run_menu(menu, 2, NULL, 0, 0, 0, 0, "Данные могут быть потеряны! Сохранить?");
    if (c == 0)
        save(beg);
}

void empty_bi_list(bi_list* beg) {
    if (beg) {
        empty_bi_list(beg->r);
        free(beg);
    }
}

void empty_list(list* top) {
    if (top) {
        empty_list(top->next);
        free(top);
    }
}

void empty_str(street* top) {
    if (top) {
        empty_str(top->r);
        empty_list(top->top);
        free(top);
    }
}

void write_top_table(const char *s) {
    for (int i = 0; i < 2; i++)
        puts("");
    if (s) {
        indent();
        puts(s);
    }
    indent();
    puts("------------------------------------------------------------------------------------------------------------------------------------------------------------");
    indent();
    puts("|   №  |        Фамилия       |          Имя         |       Отчество       |              Улица             |  Дом  | Квартира | Кол-во жильцов | Площадь |");
    indent();
    puts("------------------------------------------------------------------------------------------------------------------------------------------------------------");
}

void write(flat fl) {
    indent();
    printf("| %4hi | %*s | %*s | %*s | ",
        fl.n,
        20 + utf8_extra(fl.surname),    fl.surname,
        20 + utf8_extra(fl.name),       fl.name,
        20 + utf8_extra(fl.patronymic), fl.patronymic);
    printf("%*s | %5hi | %8hi | %14hi | %7.2f |\n",
        30 + utf8_extra(fl.street), fl.street,
        fl.house, fl.flat, fl.residents, fl.area);
    indent();
    puts("------------------------------------------------------------------------------------------------------------------------------------------------------------");
}

// Вспомогательные функции для UTF-8 строчных/прописных русских букв
static void utf8_to_upper(char* s, int i) {
    unsigned char c1 = (unsigned char)s[i];
    unsigned char c2 = (unsigned char)s[i+1];
    if (c1 == 0xD0 && c2 >= 0xB0 && c2 <= 0xBF)
        s[i+1] = (char)(c2 - 0x20);        // а-п → А-П
    else if (c1 == 0xD1 && c2 >= 0x80 && c2 <= 0x8F) {
        s[i] = (char)0xD0; s[i+1] = (char)(c2 + 0x20); // р-я → Р-Я
    } else if (c1 == 0xD1 && c2 == 0x91) {
        s[i] = (char)0xD0; s[i+1] = (char)0x81;         // ё → Ё
    }
}

static void utf8_to_lower(char* s, int i) {
    unsigned char c1 = (unsigned char)s[i];
    unsigned char c2 = (unsigned char)s[i+1];
    if (c1 == 0xD0 && c2 >= 0x90 && c2 <= 0x9F)
        s[i+1] = (char)(c2 + 0x20);        // А-П → а-п
    else if (c1 == 0xD0 && c2 >= 0xA0 && c2 <= 0xAF) {
        s[i] = (char)0xD1; s[i+1] = (char)(c2 - 0x20); // Р-Я → р-я
    } else if (c1 == 0xD0 && c2 == 0x81) {
        s[i] = (char)0xD1; s[i+1] = (char)0x91;         // Ё → ё
    }
}

void refresh_register(char* s) {
    bool capitalize = true;
    int i = 0;
    int len = strlen(s);
    while (i < len) {
        unsigned char c1 = (unsigned char)s[i];
        if (c1 == 0xD0 || c1 == 0xD1) {
            if (i + 1 >= len) break;
            if (capitalize) { utf8_to_upper(s, i); capitalize = false; }
            else              utf8_to_lower(s, i);
            i += 2;
        } else if (c1 >= 'A' && c1 <= 'Z') {
            if (!capitalize) s[i] = 'a' + (c1 - 'A');
            else capitalize = false;
            i++;
        } else if (c1 >= 'a' && c1 <= 'z') {
            if (capitalize) { s[i] = 'A' + (c1 - 'a'); capitalize = false; }
            i++;
        } else if (c1 == ' ') {
            capitalize = true; i++;
        } else if (c1 == '-') {
            if (i > 0) {
                unsigned char prev = (unsigned char)s[i-1];
                if (prev < '0' || prev > '9') capitalize = true;
            }
            i++;
        } else {
            i++;
        }
    }
}

bool read_way(const char* message, char* s) {
    char buf[60];
    cls();
    printf("Для отмены нажмите Escape\n");
    printf("%s: ", message);
    fflush(stdout);
    if (raw_readline(buf, sizeof(buf))) { cls(); return false; }
    cls();
    if (strlen(buf) == 0) return false;
    strncpy(s, buf, 49);
    s[49] = '\0';
    return true;
}

bool read_str(const char* message, char* s) {
    char buf[100];
    while (true) {
        cls();
        printf("Для отмены нажмите Escape\n");
        printf("%s: ", message);
        fflush(stdout);
        if (raw_readline(buf, 20)) return true;
        int len = strlen(buf);
        if (len == 0) continue;
        bool valid = true;
        for (int i = 0; i < len && valid; ) {
            unsigned char c1 = (unsigned char)buf[i];
            if ((c1 >= 'A' && c1 <= 'Z') || (c1 >= 'a' && c1 <= 'z')) {
                i++;
            } else if (c1 == 0xD0 || c1 == 0xD1) {
                if (i + 1 < len) i += 2; else valid = false;
            } else if (c1 == '-' && i > 0 && (unsigned char)buf[i-1] != '-') {
                i++;
            } else {
                valid = false;
            }
        }
        if (!valid) { error("Недопустимые символы! Только буквы и дефис."); continue; }
        strncpy(s, buf, 19);
        s[19] = '\0';
        refresh_register(s);
        return false;
    }
}

bool read_str_for_street(const char* message, char* s) {
    char buf[100];
    while (true) {
        cls();
        printf("Для отмены нажмите Escape\n");
        printf("%s: ", message);
        fflush(stdout);
        if (raw_readline(buf, 30)) return true;
        int len = strlen(buf);
        if (len == 0) continue;
        bool valid = true;
        for (int i = 0; i < len && valid; ) {
            unsigned char c1 = (unsigned char)buf[i];
            if ((c1 >= 'A' && c1 <= 'Z') || (c1 >= 'a' && c1 <= 'z') ||
                (c1 >= '0' && c1 <= '9')) {
                i++;
            } else if (c1 == 0xD0 || c1 == 0xD1) {
                if (i + 1 < len) i += 2; else valid = false;
            } else if (i > 0 && (c1 == '-' || c1 == ' ') &&
                       (unsigned char)buf[i-1] != '-' && (unsigned char)buf[i-1] != ' ') {
                i++;
            } else {
                valid = false;
            }
        }
        if (!valid) { error("Недопустимые символы! Буквы, цифры, пробел, дефис."); continue; }
        strncpy(s, buf, 29);
        s[29] = '\0';
        refresh_register(s);
        return false;
    }
}

bool read_short(const char* s, short& n) {
    char buf[30];
    while (true) {
        cls();
        printf("Для отмены нажмите Escape\n");
        printf("%s: ", s);
        fflush(stdout);
        if (raw_readline(buf, sizeof(buf))) return true;
        if (strlen(buf) == 0) continue;
        int val;
        if (sscanf(buf, "%d", &val) == 1 && val > 0 && val <= 32767) {
            n = (short)val;
            return false;
        }
        error("Введите положительное целое число (1–32767)");
    }
}

bool read_float(const char* s, float& result) {
    char buf[50];
    while (true) {
        cls();
        printf("Для отмены нажмите Escape\n");
        printf("%s (например: 45.50): ", s);
        fflush(stdout);
        if (raw_readline(buf, sizeof(buf))) return true;
        if (strlen(buf) == 0) continue;
        float val;
        if (sscanf(buf, "%f", &val) == 1 && val > 0 && val < 10000) {
            result = val;
            return false;
        }
        error("Введите положительное число (например: 45.50)");
    }
}

flat read(bool& p) {
    bool e = false;
    flat fl;
    if (e = read_str("Введите фамилию", fl.surname)) {
        p = false;
        return fl;
    }
    if (e = read_str("Введите имя", fl.name)) {
        p = false;
        return fl;
    }
    if (e = read_str("Введите отчество", fl.patronymic)) {
        p = false;
        return fl;
    }
    if (e = read_str_for_street("Введите улицу", fl.street)) {
        p = false;
        return fl;
    }
    if (e = read_short("Введите номер дома", fl.house)) {
        p = false;
        return fl;
    }
    if (e = read_short("Введите номер квартиры", fl.flat)) {
        p = false;
        return fl;
    }
    if (e = read_short("Введите количество жильцов", fl.residents)) {
        p = false;
        return fl;
    }
    if (e = read_float("Введите площадь", fl.area)) {
        p = false;
        return fl;
    }
    cls();
    return fl;
}

void skip(FILE* f) {
    char c;
    do {
        c = fgetc(f);
    } while (c == ' ' || c == '|' || c == '-' || c == '\n');
    fseek(f, -1, SEEK_CUR);
}

flat read_from_text(FILE* f) {
    flat fl;
    short i = 0;
    skip(f);
    fscanf(f, "%hi", &fl.n);
    skip(f);
    fscanf(f, "%19s", fl.surname);
    skip(f);
    fscanf(f, "%19s", fl.name);
    skip(f);
    fscanf(f, "%19s", fl.patronymic);
    skip(f);
    do {
        char c = fgetc(f);
        if (c == '|')
            break;
        fl.street[i] = c;
        i++;
    } while (true);
    fl.street[i - 1] = '\0';
    skip(f);
    fscanf(f, "%hi", &fl.house);
    skip(f);
    fscanf(f, "%hi", &fl.flat);
    skip(f);
    fscanf(f, "%hi", &fl.residents);
    skip(f);
    fscanf(f, "%f", &fl.area);
    return fl;
}

void refresh(bi_list* beg) {
    short n = 0;
    for (beg; beg != NULL; beg = beg->r) {
        n++;
        beg->inf.n = n;
    }
}

bi_list* organize(bi_list* beg, short& n) {
    flat fl;
    bool e;
    if (beg)
        request_empty_bi_list(beg);
    fl = read(e = true);
    if (!e)
        return beg;
    if (beg)
        empty_bi_list(beg);
    beg = (bi_list*)malloc(size_bi_list);
    beg->inf = fl;
    beg->inf.n = 1;
    beg->l = NULL;
    beg->r = NULL;
    n = 1;
    return beg;
}

bi_list* add(bi_list* end, short& k) {
    bool p;
    bi_list* t;
    short c;
    char** menu;
    menu = new char* [2];
    for (c = 0; c < 2; c++)
        menu[c] = new char[60];
    strcpy(menu[0], "Добавить новую запись");
    strcpy(menu[1], "Выход в главное меню");

    do {
        t = (bi_list*)malloc(size_bi_list);
        t->inf = read(p = true);
        if (!p) {
            free(t);
            c = run_menu(menu, 2);
        }
        else {
            k++;
            t->inf.n = k;
            end->r = t;
            t->l = end;
            end = t;
            c = run_menu(menu, 2, t, 0, 0, 0, 0, "Добавлена запись");
        }
    } while (c == 0);
    end->r = NULL;
    return end;
}

void browse(bi_list* beg, short n) {
    char** menu;
    short i, k = 0, page = 1, new_page = 0;
    bool e;

    menu = new char*[4];
    for (i = 0; i < 4; i++) {
        menu[i] = new char[70];
    }
    strcpy(menu[0], "Следующая страница");
    strcpy(menu[1], "Предыдущая страница");
    strcpy(menu[2], "Перейти к странице по номеру");
    strcpy(menu[3], "Выход из просмотра");

    if (n % 10 == 0)
        n /= 10;
    else
        n = n / 10 + 1;

    while (true) {
        if (new_page == 0)
                k = run_menu(menu, 4, beg, 1, k, page, n);
        if (k == 2 && new_page == 0) {
            e = read_short("Введите номер страницы", new_page);
            if (e || new_page > n || new_page == page) {
                if (new_page > n)
                    error("Введённое значение превосходит допустимое");
                new_page = 0;
            }
        }
        if (k == 3 || k == 27)
            break;
        i = 0;
        if (k == 0 && page < n || new_page && new_page > page) {
            page++;
            for (beg = beg->r; i < 9; beg = beg->r) {
                i++;
                if (beg->r == NULL)
                    break;
            }
        }
        if (k == 1 && page > 1 || new_page && new_page < page) {
            page--;
            for (beg; i < 10; beg = beg->l) {
                i++;
            }
        }
        if (new_page == page)
            new_page = 0;
    }
}

pair del(bi_list* beg, bi_list* end, short& n) {
    pair back;
    bi_list* t;
    short k = 0, m, c;
    bool e, o;
    char** menu_1, **menu_2;
    menu_1 = new char*[2];
    menu_2 = new char*[2];
    for (m = 0; m < 2; m++) {
        menu_1[m] = new char[20];
        menu_2[m] = new char[60];
    }
    strcpy(menu_1[0], "Да");
    strcpy(menu_1[1], "Нет");
    strcpy(menu_2[0], "Удалить запись");
    strcpy(menu_2[1], "Выход в главное меню");

    do {
        e = read_short("Введите номер записи для удаления", m);
        if (e)
            break;
        if (m > n)
            error("Введённое значение превосходит допустимое!");
        else {
            o = false;
            for (t = beg; t != NULL; t = t->r) {
                if (t->inf.n == m) {
                    o = true;
                    c = run_menu(menu_1, 2, t, 0, 0, 0, 0, "Вы действительно хотите удалить эту запись?");
                    if (c == 1)
                        break;
                    k++;
                    if (t == beg) {
                        beg = beg->r;
                        if (beg)
                            beg->l = NULL;
                        else
                            end = NULL;
                    }
                    else if (t == end) {
                        end = end->l;
                        end->r = NULL;
                    }
                    else {
                        t->l->r = t->r;
                        t->r->l = t->l;
                    }
                    free(t);
                    break;
                }
            }
            if (!o)
                error("Запись с заданным номером не обнаружена");
        }
        if (beg)
            c = run_menu(menu_2, 2);
    } while (beg && c == 0);
    n -= k;
    back.beg = beg;
    back.end = end;
    return back;
}

void correct(bi_list* beg, short n) {
    bi_list* t;
    flat fl;
    bool e;
    short m, c = 8, i;
    char** menu;
    menu = new char* [10];
    for (i = 0; i < 10; i++)
        menu[i] = new char[80];
    strcpy(menu[0], "Редактировать фамилию");
    strcpy(menu[1], "Редактировать имя");
    strcpy(menu[2], "Редактировать отчество");
    strcpy(menu[3], "Редактировать улицу");
    strcpy(menu[4], "Редактировать номер дома");
    strcpy(menu[5], "Редактировать номер квартиры");
    strcpy(menu[6], "Редактировать количество жильцов");
    strcpy(menu[7], "Редактировать площадь квартиры");
    strcpy(menu[8], "Редактировать другую запись");
    strcpy(menu[9], "Выход в главное меню");

    do {
        e = read_short("Введите номер записи для редактирования", m);
        if (e)
            break;
        if (m > n) {
            error("Введённое значение превосходит допустимое!");
            continue;
        }
        for (t = beg; t != NULL; t = t->r) {
            if (t->inf.n == m) {
                while (!e) {
                    fl = t->inf;
                    c = run_menu(menu, 10, t);
                    switch (c) {
                    case 0:
                        e = read_str("Введите новую фамилию", fl.surname);
                        break;
                    case 1:
                        e = read_str("Введите новое имя", fl.name);
                        break;
                    case 2:
                        e = read_str("Введите новое отчество", fl.patronymic);
                        break;
                    case 3:
                        e = read_str_for_street("Введите новую улицу", fl.street);
                        break;
                    case 4:
                        e = read_short("Введите новый номер дома", fl.house);
                        break;
                    case 5:
                        e = read_short("Введите новый номер квартиры", fl.flat);
                        break;
                    case 6:
                        e = read_short("Введите новое количество жильцов", fl.residents);
                        break;
                    case 7:
                        e = read_float("Введите новую площадь", fl.area);
                        break;
                    default:
                        e = true;
                        break;
                    }
                    t->inf = fl;
                }
            }
        }
    } while (c == 8);
}

pair sort_process(bi_list* beg, short d) {
    pair back;
    bi_list* temp, * t, * t1;
    temp = beg;
    d *= -1;
    while (temp->r != NULL) {
        if ((strcmp(temp->r->inf.surname, temp->inf.surname) > 0) == (d > 0)) {
            t1 = temp->r;
            t1->l->r = t1->r;
            if (t1->r)
                t1->r->l = t1->l;
            if (temp->l)
                t = temp->l;
            else
                t = temp;
            while (true) {
                if ((strcmp(t1->inf.surname, t->inf.surname) > 0) != (d > 0)) {
                    t1->l = t;
                    t1->r = t->r;
                    t->r = t1;
                    if (t1->r)
                        t1->r->l = t1;
                    break;
                }
                else if (t->l == NULL) {
                    t1->r = beg;
                    beg->l = t1;
                    beg = t1;
                    beg->l = NULL;
                    break;
                }
                else
                    t = t->l;
            }
        }
        else if (temp->r)
            temp = temp->r;
    }
    back.beg = beg;
    back.end = temp;
    return back;
}

pair sort(bi_list* beg) {
    pair back;
    short c, i;
    char** menu;
    back.beg = NULL;
    back.end = NULL;

    menu = new char* [3];
    for (i = 0; i < 3; i++)
        menu[i] = new char[60];
    strcpy(menu[0], "По возрастанию");
    strcpy(menu[1], "По убыванию");
    strcpy(menu[2], "Выход в главное меню");

    c = run_menu(menu, 3);
    switch (c) {
    case 0:
        back = sort_process(beg, 1);
        break;
    case 1:
        back = sort_process(beg, -1);
        break;
    default:
        break;
    }
    return back;
}

void search(bi_list* beg) {
    bi_list* top = NULL, * end = NULL, * t, * temp;
    short n, c;
    char **menu, s[30] = "";
    bool e;

    menu = new char* [2];
    for (n = 0; n < 2; n++)
        menu[n] = new char[80];
    strcpy(menu[0], "Поиск записей по названию улицы");
    strcpy(menu[1], "Выход в главное меню");

    do {
        n = 0;
        if (e = read_str_for_street("Укажите название улицы для поиска", s))
            return;
        if (top) {
            empty_bi_list(top);
            top = NULL;
            end = NULL;
        }
        for (temp = beg; temp != NULL; temp = temp->r) {
            if (strcmp(s, temp->inf.street) == 0) {
                if (!top) {
                    top = (bi_list*)malloc(size_bi_list);
                    top->inf = temp->inf;
                    top->inf.n = 1;
                    top->l = NULL;
                    top->r = NULL;
                    n = 1;
                    end = top;
                }
                else {
                    n++;
                    t = (bi_list*)malloc(size_bi_list);
                    t->inf = temp->inf;
                    end->r = t;
                    t->l = end;
                    end = t;
                }
            }
        }
        if (end)
            end->r = NULL;
        if (n > 0)
            browse(top, n);
        else
            error("Записей с указанной улицей найдено не было!");
        c = run_menu(menu, 2);
    } while (c == 0);
    empty_bi_list(top);
}

bool define(char* s) {
    short n;
    n = strlen(s);
    if (s[n - 1] == 't' && s[n - 2] == 'x' && s[n - 3] == 't')
        return true;
    else
        return false;
}

void save(bi_list* beg) {
    flat fl;
    char s[50];
    FILE* f;
    bool k;
    if (read_way("Введите путь файла для записи", s)) {
        k = define(s);
        if (k)
            f = fopen(s, "wt");
        else
            f = fopen(s, "wb");
        if (!f)
            error("Ошибка доступа!");
        else {
            if (k) {
                fprintf(f, "------------------------------------------------------------------------------------------------------------------------------------------------------------\n");
                fprintf(f, "|   №  |        Фамилия       |          Имя         |       Отчество       |              Улица             |  Дом  | Квартира | Кол-во жильцов | Площадь |\n");
                fprintf(f, "------------------------------------------------------------------------------------------------------------------------------------------------------------\n");
                for (beg; beg != NULL; beg = beg->r) {
                    fl = beg->inf;
                    fprintf(f, "| %4hi | %*s | %*s | %*s | ",
                        fl.n,
                        20 + utf8_extra(fl.surname),    fl.surname,
                        20 + utf8_extra(fl.name),       fl.name,
                        20 + utf8_extra(fl.patronymic), fl.patronymic);
                    fprintf(f, "%*s | %5hi | %8hi | %14hi | %7.2f |\n",
                        30 + utf8_extra(fl.street), fl.street,
                        fl.house, fl.flat, fl.residents, fl.area);
                    fprintf(f, "------------------------------------------------------------------------------------------------------------------------------------------------------------\n");
                }
            }
            else
                for (beg; beg != NULL; beg = beg->r)
                    fwrite(&beg->inf, size_flat, 1, f);
            fclose(f);
        }
    }
}

pair load(bi_list* beg, short& k) {
    pair back;
    short n;
    long c;
    bi_list* t, * end;
    char s[50];
    bool p;
    FILE* f;
    if (beg)
        request_empty_bi_list(beg);
    back.beg = NULL;
    back.end = NULL;
    if (read_way("Введите путь файла для загрузки", s)) {
        p = define(s);
        if (p)
            f = fopen(s, "rt");
        else
            f = fopen(s, "rb");
        if (!f)
            error("Файл не найден или доступ ограничен!");
        else {
            if (beg)
                empty_bi_list(beg);
            k = 0;
            if (p) {
                fseek(f, 0, SEEK_END);
                c = ftell(f);
                // Пропускаем 3 строки заголовка без хардкода смещения
                rewind(f);
                { int lines = 0, ch;
                  while (lines < 3 && (ch = fgetc(f)) != EOF)
                      if (ch == '\n') lines++; }
                beg = (bi_list*)malloc(size_bi_list);
                beg->inf = read_from_text(f);
                beg->inf.n = 1;
                beg->l = NULL;
                beg->r = NULL;
                end = beg;
                k = 1;
                while (c - ftell(f) > 162) {
                    k++;
                    t = (bi_list*)malloc(size_bi_list);
                    t->inf = read_from_text(f);
                    t->inf.n = k;
                    end->r = t;
                    t->l = end;
                    end = t;
                }
            }
            else {
                fseek(f, 0, SEEK_END);
                n = ftell(f) / size_flat;
                k = n;
                rewind(f);
                beg = (bi_list*)malloc(size_bi_list);
                fread(&beg->inf, size_flat, 1, f);
                beg->l = NULL;
                beg->r = NULL;
                end = beg;
                for (n--; n > 0; n--) {
                    t = (bi_list*)malloc(size_bi_list);
                    fread(&t->inf, size_flat, 1, f);
                    end->r = t;
                    t->l = end;
                    end = t;
                }
            }
            end->r = NULL;
            fclose(f);
            back.beg = beg;
            back.end = end;
        }
    }
    return back;
}

list* insert_str(list* top, flat inf) {
    short i;
    list* t, * t1;
    if (inf.residents > top->inf.residents) {
        t = (list*)malloc(size_list);
        t->inf = inf;
        t->next = top;
        top = t;
    }
    else {
        i = 0;
        for (t = top; t->next != NULL; t = t->next) {
            i++;
            if (inf.residents > t->next->inf.residents || i == 5)
                break;
        }
        if (i != 5) {
            t1 = (list*)malloc(size_list);
            t1->inf = inf;
            t1->next = t->next;
            t->next = t1;
        }
    }
    return top;
}

void write_top_str() {
    puts("");
    puts("");
    indent();
    puts("Сводка");
    indent();
    puts("------------------------------------------------------------------------------------------------------------------------------------------------------------");
    indent();
    puts("|              Улица             |   №  |        Фамилия       |          Имя         |       Отчество       |  Дом  | Квартира | Кол-во жильцов | Площадь |");
    indent();
    puts("------------------------------------------------------------------------------------------------------------------------------------------------------------");
}

void space_write_str() {
    short i;
    indent();
    printf("|");
    for (i = 0; i < 32; i++)
        printf(" ");
}

short write_str(list * top) {
    short j = 0;
    flat fl = top->inf;
    indent();
    printf("| %*s", 30 + utf8_extra(top->inf.street), top->inf.street);
    printf(" | %4hi | %*s | %*s | %*s | ",
        fl.n,
        20 + utf8_extra(fl.surname),    fl.surname,
        20 + utf8_extra(fl.name),       fl.name,
        20 + utf8_extra(fl.patronymic), fl.patronymic);
    printf("%5hi | %8hi | %14hi | %7.2f |\n", fl.house, fl.flat, fl.residents, fl.area);
    if (top->next) {
        space_write_str();
        puts("---------------------------------------------------------------------------------------------------------------------------");
        for (top = top->next; top != NULL; top = top->next) {
            j++;
            fl = top->inf;
            space_write_str();
            printf("| %4hi | %*s | %*s | %*s | ",
                fl.n,
                20 + utf8_extra(fl.surname),    fl.surname,
                20 + utf8_extra(fl.name),       fl.name,
                20 + utf8_extra(fl.patronymic), fl.patronymic);
            printf("%5hi | %8hi | %14hi | %7.2f |\n", fl.house, fl.flat, fl.residents, fl.area);
            if (j == 4)
                break;
            if (top->next) {
                space_write_str();
                puts("---------------------------------------------------------------------------------------------------------------------------");
            }
        }
    }
    indent();
    puts("------------------------------------------------------------------------------------------------------------------------------------------------------------");
    return j + 1;
}

void browse_str(street* top_str, short n) {
    char** menu;
    short i, k = 0, page = 1, new_page = 0;
    bool e;

    menu = new char* [4];
    for (i = 0; i < 4; i++) {
        menu[i] = new char[70];
    }
    strcpy(menu[0], "Следующая страница");
    strcpy(menu[1], "Предыдущая страница");
    strcpy(menu[2], "Перейти к странице по номеру");
    strcpy(menu[3], "Выход из просмотра");

    while (true) {
        if (new_page == 0)
            k = run_menu(menu, 4, NULL, 0, k, page, n, "", top_str);
        if (k == 2 && new_page == 0) {
            e = read_short("Введите номер страницы", new_page);
            if (e || new_page > n || new_page == page) {
                if (new_page > n)
                    error("Введённое значение превосходит допустимое");
                new_page = 0;
            }
        }
        if (k == 3 || k == 27)
            break;
        i = 0;
        if (k == 0 && page < n || new_page && new_page > page) {
            page++;
            for (top_str = top_str->r; top_str->r != NULL; top_str = top_str->r)
                if (top_str->page == page)
                    break;
        }
        if (k == 1 && page > 1 || new_page && new_page < page) {
            page--;
            for (top_str = top_str->l; top_str->l != NULL; top_str = top_str->l)
                if (top_str->l->page < page)
                    break;
        }
        if (new_page == page)
            new_page = 0;
    }
}

street* new_street(flat fl) {
    street* t;
    t = (street*)malloc(size_street);
    t->n = 1;
    strncpy(t->inf, fl.street, 30);
    t->top = (list*)malloc(size_list);
    t->top->inf = fl;
    t->r = NULL;
    t->l = NULL;
    t->top->next = NULL;
    return t;
}

short count_page_str(street* top) {
    short summa = 0, page = 1;
    for (top; top != NULL; top = top->r) {
        if (top->n < 5)
            summa += top->n;
        else
            summa += 5;
        if (summa > 15) {
            summa = top->n;
            page++;
        }
        top->page = page;
    }
    return page;
}

void create_street(bi_list* beg) {
    int c, n;
    street* top_str = NULL, *t, *t1;
    if (beg) {
        top_str = new_street(beg->inf);
        beg = beg->r;
    }
    for (beg; beg != NULL; beg = beg->r) {
        t = top_str;
        while (true) {
            c = strcmp(beg->inf.street, t->inf);
            if (c > 0) {
                if (t->r == NULL) {
                    t1 = new_street(beg->inf);
                    t1->l = t;
                    t->r = t1;
                    break;
                }
                else
                    t = t->r;
            }
            else if (c < 0) {
                t1 = new_street(beg->inf);
                if (t == top_str) {
                    t1->r = top_str;
                    top_str->l = t1;
                    top_str = t1;
                }
                else {
                    t->l->r = t1;
                    t1->l = t->l;
                    t->l = t1;
                    t1->r = t;
                }
                break;
            }
            else {
                t->n++;
                t->top = insert_str(t->top, beg->inf);
                break;
            }
        }
    }
    n = count_page_str(top_str);
    browse_str(top_str, n);
    empty_str(top_str);
}
