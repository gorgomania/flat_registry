#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <windows.h>
#include <conio.h>
#include <stdlib.h>

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
//применяется для группировки элементов по названию улицы
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

//Главная функция, отвечает за инициализацию главного меню
int main() {
	pair back;
	bi_list * beg = NULL, *end = NULL;
	char** menu;
	short n = 0, c, i;
	make_good_console();
	menu = new char* [11];
	for (i = 0; i < 11; i++)
		menu[i] = new char[55];
	strncpy(menu[0], "Создать таблицу", 30);
	strncpy(menu[1], "Просмотр", 30);
	strncpy(menu[2], "Добавить новую запись", 30);
	strncpy(menu[3], "Удалить запись", 30);
	strncpy(menu[4], "Редактировать запись", 30);
	strncpy(menu[5], "Сортировка записей по фамилии", 30);
	strncpy(menu[6], "Поиск записей по названию улицы", 35);
	strncpy(menu[7], "Сохранить таблицу", 30);
	strncpy(menu[8], "Загрузить таблицу", 30);
	strncpy(menu[9], "Поиск пяти самых заселённых квартир на каждой улице", 55);
	strncpy(menu[10], "Выход из программы", 20);

	while (true) {
		c = run_menu(menu, 11);
		system("cls");
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

//Процедура, которая отвечает за настройку консоли
void make_good_console() {
	HANDLE hCon = GetStdHandle(STD_OUTPUT_HANDLE);
	COORD NewSBSize;
	SMALL_RECT DisplayArea = { 0, 0, 0, 0 };
	SetConsoleTextAttribute(hCon, 240);
	SetConsoleCP(1251);
	SetConsoleOutputCP(1251);
	keybd_event(VK_MENU, 0x38, 0, 0);
	keybd_event(VK_RETURN, 0x1c, 0, 0);
	keybd_event(VK_RETURN, 0x1c, KEYEVENTF_KEYUP, 0);
	keybd_event(VK_MENU, 0x38, KEYEVENTF_KEYUP, 0);
	NewSBSize = GetLargestConsoleWindowSize(hCon);
	SetConsoleScreenBufferSize(hCon, NewSBSize);
	DisplayArea.Right = NewSBSize.X - 1;
	DisplayArea.Bottom = NewSBSize.Y - 1;
	SetConsoleWindowInfo(hCon, TRUE, &DisplayArea);
}

//Процедура, которая производит отступы при выводе текста в консоли
void indent() {
	printf("         ");
}

//Функция, которая задаёт графический интерфейс меню, а также отвечает за работу селектора
//Может принимать следующие параметры
//menu массив эллементов меню
//n количество элементов меню
//Остальные элементы являются необязательными и служат для поднастройки меню
//top указатель на двусвязный список, необходим при синхронном выводе таблицы и работе меню
//k при значении true переводит вывод таблице в режим отображения только одного элемента, в обратном случае происходит постраничный вывод с возможностью скроллинга
//position позволяет предустанавливать курсор на опеределённом элементе меню при его вызове
//page определяет номер страницы показа
//max_page определяет количество страниц
//s позволяет установить заголовок таблицы
//top_str указатель на результаты обработки данных, обеспечивает синхронный вывод меню с результатами
//Функция возвращает номер пункта меню выбранного пользователем, а также код 27 в случае нажатия пользователем клавиши Escape
short run_menu(char** menu, short n, bi_list *top, bool k, short position, short page, short max_page, const char *s, street *top_str) {
	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	bi_list* t;
	street* t_str;
	short i, c, j;
	while (true) {
		system("cls");
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
			for (i; i < 17; i++) {
				puts("");
				puts("");
			}
		}
		else if (top == NULL) {
			for (i = 0; i < 20; i++)
				puts("");
			if (s != "") {
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
				for (i; i < 10; i++) {
					puts("");
					puts("");
				}
			}
			else {
				if (s == "")
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
				SetConsoleTextAttribute(hConsole, 144);
				puts(menu[i]);
				SetConsoleTextAttribute(hConsole, 240);
			}
			else {
				printf("          ");
				puts(menu[i]);
			}
			puts("");
		} while ((c = _getch()) != 0 && c != 224 && c != 13 && c != 27);
		if (c == 0 || c == 224)
			c = _getch();
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

//Процедура служит для вывода сообщения об ошибке
//s является соответствующим сообщением
void error(const char* s) {
	system("cls");
	system("color F4");
	puts(s);
	system("pause");
	system("cls");
	system("color F0");
}

//Процедура служит для предотвращения случайной потери данных в последствии действий пользователя. Процедура предупреждает об этом и предлагает сохранить данные в файл на комьютере.
//beg указатель на начало двухсвязного списка
void request_empty_bi_list(bi_list* beg) {
	char** menu;
	int c;

	menu = new char* [2];
	for (c = 0; c < 2; c++)
		menu[c] = new char[5];
	strncpy(menu[0], "Да", 5);
	strncpy(menu[1], "Нет", 5);

	c = run_menu(menu, 2, NULL, 0, 0, 0, 0, "Данные могут быть потеряны! Сохранить?");
	if (c == 0)
		save(beg);
}

//Процедура для очистки памяти выделенной под двусвязный список
//beg указатель на начало двусвязного списка
void empty_bi_list(bi_list* beg) {
	if (beg) {
		empty_bi_list(beg->r);
		free(beg);
	}
}

//Процедура для очистки памяти выделенной под односвязный список
//top указатель на начало односвязного списка
void empty_list(list* top) {
	if (top) {
		empty_list(top->next);
		free(top);
	}
}

//Процедура для очистки памяти выделенной под результаты функции обработки
//top указатель на начало списка результатов фукнции обработки
void empty_str(street* top) {
	if (top) {
		empty_str(top->r);
		empty_list(top->top);
		free(top);
	}
}

//Процедура вывода шапки таблицы
//s является необязательным параметром и позволяет установить заголовок таблицы
void write_top_table(const char *s) {
	for (int i = 0; i < 5; i++)
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

//Процедура вывода элемента списка
//fl информационное поле элемета списка
void write(flat fl) {
	indent();
	printf("| %4hi | %20s | %20s | %20s | ", fl.n, fl.surname, fl.name, fl.patronymic);
	printf("%30s | %5hi | %8hi | %14hi | %7.2f |\n", fl.street, fl.house, fl.flat, fl.residents, fl.area);
	indent();
	puts("------------------------------------------------------------------------------------------------------------------------------------------------------------");
}

//Процедура исправления регистра в строке
//s строка переданная для исправления
void refresh_register(char* s) {
	bool p = true;
	short i = 0, c;
	do {
		c = s[i];
		if (p) {
			if (c <= 122 && c >= 97)
				s[i] = char(65 + c - 97);
			else if (c <= -1 && c >= -32)
				s[i] = char(-64 + c + 32);
			else if (c == -72)
				s[i] = char(-88);
			p = false;
		}
		else {
			if (c <= 90 && c >= 65)
				s[i] = char(97 + c - 65);
			else if (c <= -33 && c >= -64)
				s[i] = char(-32 + c + 64);
			else if (c == -88)
				s[i] = char(-72);
		}
		if (c == 45) {
			c = short(s[i - 1]);
			if (c < 48 || c > 57)
				p = true;
		}
		if (c == 32)
			p = true;
		i++;
	} while (s[i] != '\0');
}

//Функция служит для обеспечения корректного ввода пути к файлу
//message комментарий к вводу
//s строка ввода
//Функция возвращает false при отмене ввода, в обратном случае true
bool read_way(const char* message, char* s) {
	short i = 0, c1 = 0;
	char c;
	s[i] = '\0';
	system("cls");
	puts("Для отмены операции нажмите Escape");
	puts(message);
	while (true) {
		c = _getch();
		c1 = int(c);
		if (c1 == 27 || c1 == 13 && i > 4)
			break;
		else if (c1 == 8) {
			if (i > 0) {
				s[i - 1] = '\0';
				i--;
			}
			system("cls");
			system("color F0");
			puts("Для отмены операции нажмите Escape");
			puts(message);
			printf("%s", s);
		}
		else if (i < 50 && c1 != 13) {
			system("color F0");
			printf("%c", c);
			s[i] = c;
			i++;
			s[i] = '\0';
		}
		else
			system("color F4");		
	}
	system("cls");
	system("color F0");
	if (c1 == 27)
		return false;
	else
		return true;
}

//Функция служит для обеспечения корректного ввода информации для большинства строчных полей двухсвязного списка. Применяются правила исходящие из семантического анализа назначения переменных.
//message комментарий к вводу
//s строка ввода
//Функция возвращает false при отмене ввода, в обратном случае true
bool read_str(const char* message, char* s) {
	short i = 0, c1 = 0;
	char c;
	s[i] = '\0';
	system("cls");
	puts("Для отмены операции нажмите Escape");
	puts(message);
	while (true) {
		c = _getch();
		c1 = int(c);
		if (c1 == 27 || c1 == 13 && i > 0)
			break;
		else if (c1 == 8) {
			if (i > 0) {
				s[i - 1] = '\0';
				i--;
			}
			system("cls");
			system("color F0");
			puts("Для отмены операции нажмите Escape");
			puts(message);
			printf("%s", s);
		}
		else if (i < 20 && ((c1 <= -1 && c1 >= -64 || c1 <= 90 && c1 >= 65 || c1 <= 122 && c1 >=97 || c1 == -88 || c1 == -72) || (c1 == 45 && i > 0 && short(s[i - 1]) != 45))) {
			system("color F0");
			printf("%c", c);
			s[i] = c;
			i++;
			s[i] = '\0';
		}
		else
			system("color F4");
	}
	refresh_register(s);
	system("cls");
	system("color F0");
	if (c1 == 27)
		return true;
	else
		return false;
}

//Функция служит для обеспечения корректного ввода строчной информации для поля содержащего название улицы. Применяются правила исходящие из семантического анализа назначения переменных.
//message комментарий к вводу
//s строка ввода
//Функция возвращает false при отмене ввода, в обратном случае true
bool read_str_for_street(const char* message, char* s) {
	short i = 0, c1 = 0;
	char c;
	s[i] = '\0';
	system("cls");
	puts("Для отмены операции нажмите Escape");
	puts(message);
	while (true) {
		c = _getch();
		c1 = int(c);
		if (c1 == 27 || c1 == 13 && i > 0)
			break;
		else if (c1 == 8) {
			if (i > 0) {
				s[i - 1] = '\0';
				i--;
			}
			system("cls");
			system("color F0");
			puts("Для отмены операции нажмите Escape");
			puts(message);
			printf("%s", s);
		}
		else if (i < 30 && ((c1 <= -1 && c1 >= -64 || c1 <= 90 && c1 >= 65 || c1 <= 122 && c1 >= 97 || c1 <= 57 && c1 >= 48 || c1 == -88 || c1 == -72) || (i > 0 && ((c1 == 45 || c1 == 32) && short(s[i - 1]) != 32 && short(s[i - 1]) != 45)))) {
			system("color F0");
			printf("%c", c);
			s[i] = c;
			i++;
			s[i] = '\0';
		}
		else
			system("color F4");
	}
	refresh_register(s);
	system("cls");
	system("color F0");
	if (c1 == 27)
		return true;
	else
		return false;
}

//Функция служит для обеспечения корректного ввода информации целого типа. Применяются правила исходящие из семантического анализа назначения переменных.
//s комментарий к вводу
//n ссылка на переменную для ввода
//Функция возвращает false при отмене ввода, в обратном случае true
bool read_short(const char* s, short& n) {
	bool p;
	short c = -1, n1;
	n = 0;
	system("cls");
	system("color F0");
	puts("Для отмены операции нажмите Escape");
	puts(s);
	do {
		p = false;
		c = _getch();
		if (c < 48 || c > 57) {
			if (c == 8) {
				n = n / 10;
				system("cls");
				system("color F0");
				puts("Для отмены операции нажмите Escape");
				puts(s);
				if (n > 0)
					printf("%hi", n);
				else
					n = 0;
			}
			else if (c == 13 && n > 0)
				p = true;
			else if (c == 27) {
				break;
			}
			else
				system("color F4");
		}
		else {
			n1 = n * 10 + c - 48;
			if (n <= n1) {
				system("color F0");
				printf("%hi", c - 48);
				n = n1;
			}
			else
				system("color F4");
		}
	} while (!p);
	system("color F0");
	if (c == 27)
		return true;
	else
		return false;
}

//Функция служит для обеспечения корректного ввода информации вещественного типа. Применяются правила исходящие из семантического анализа назначения переменных.
//s комментарий к вводу
//result ссылка на переменную для ввода
//Функция возвращает false при отмене ввода, в обратном случае true
bool read_float(const char* s, float& result) {
	bool p;
	short c = -1, n = 0, k = 0, n1, a = 0, b = 0;
	system("cls");
	puts("Для отмены операции нажмите Escape");
	puts(s);
	do {
		if (n > 0)
			printf("%hi", n);
		do {
			p = false;
			c = _getch();
			if (c < 48 || c > 57) {
				if (c == 8) {
					n = n / 10;
					system("cls");
					system("color F0");
					puts("Для отмены операции нажмите Escape");
					puts(s);
					if (n > 0)
						printf("%hi", n);
					else {
						k = 0;
						n = 0;
					}
				}
				else if (c == 13 && n > 0)
					p = true;
				else if (c == 27) {
					system("color F0");
					return true;
				}
				else if (c == 46 && k == 0) {
					k++;
					p = true;
					system("color F0");
					printf(".");
				}
				else if (c != 46)
					system("color F4");
			}
			else {
				n1 = n * 10 + c - 48;
				if (n <= n1 && n1 < 10000) {
					n = n1;
					system("color F0");
					printf("%hi", c - 48);
				}
				else
					system("color F4");
			}
		} while (!p);

		result = n;
		k = 0;
		if (c == 46) {
			do {
				p = false;
				c = _getch();
				if (c < 48 || c > 57) {
					if (c == 8) {
						system("cls");
						system("color F0");
						puts("Для отмены операции нажмите Escape");
						puts(s);
						if (k == 0)
							break;
						printf("%hi.", n);
						k--;
						if (k == 1)
							printf("%hi", a);
					}
					else if (c == 13 && n >= 0)
						p = true;
					else if (c == 27) {
						system("color F0");
						return true;
					}
					else
						system("color F4");
				}
				else {
					if (k <= 1) {
						system("color F0");
						k++;
						if (k == 1) {
							a = c - 48;
							printf("%hi", a);
						}
						else {
							b = c - 48;
							printf("%hi", b);
						}
					}
					else
						system("color F4");
				}
			} while (!p);
		}
	} while (!p);
	result += a / float(10) + b / float(100);
	return false;
}

//Функция ввода структуры информационного поля элемента списка
//p ссылка на логический элемент, который является индикатором успешного ввода. Будет равен true при успешном вводе, false в обратном случае
//Функция возвращет структуру, образовавшуюся в результате ввода
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
	system("cls");
	return fl;
}

//Процедура служит для пропуска символов не несущих смысловое значение, при чтении данных из текстового файла
//f указатель на текстовый файл
void skip(FILE* f) {
	char c;
	do {
		c = fgetc(f);
	} while (c == ' ' || c == '|' || c == '-' || c == '\n');
	fseek(f, -1, SEEK_CUR);
}

//Функция считывания данных из текстового файла
//f указатель на текстовый файл
//Функция возвращает полученную в результате чтения структуру данных
flat read_from_text(FILE* f) {
	flat fl;
	short i = 0;
	skip(f);
	fscanf(f, "%hi", &fl.n);
	skip(f);
	fscanf(f, "%s", &fl.surname);
	skip(f);
	fscanf(f, "%s", &fl.name);
	skip(f);
	fscanf(f, "%s", &fl.patronymic);
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

//Процедура обновления нумерации элементов в двусвязном списке
//beg указатель на начало двусвязного списка
void refresh(bi_list* beg) {
	short n = 0;
	for (beg; beg != NULL; beg = beg->r) {
		n++;
		beg->inf.n = n;
	}
}

//Функция служит для организация или реорганизации двусвязного списка
//beg указатель на начало двусвязного списка 
//n ссылка на количество элементов в двусвязном списке
//Возвращает указатель на начало двусвязного списка
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

//Функция добавления новых элементов в двусвязный список
//end указатель на конец списка
//k ссылка на количество элементов в списке
//Функция возвращает указатель на конец списка
bi_list* add(bi_list* end, short& k) {
	bool p;
	bi_list* t;
	short c;
	char** menu;
	menu = new char* [2];
	for (c = 0; c < 2; c++)
		menu[c] = new char[30];
	strncpy(menu[0], "Добавить новую запись", 30);
	strncpy(menu[1], "Выход в главное меню", 30);

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

//Процедура служит для просмотра элементов в двусвязном списке
//beg указатель на начало списка
//n количество элементов в списке
void browse(bi_list* beg, short n) {
	char** menu;
	short i, k = 0, page = 1, new_page = 0;
	bool e;

	menu = new char*[4];
	for (i = 0; i < 4; i++) {
		menu[i] = new char[30];
	}
	strncpy(menu[0], "Следующая страница", 30);
	strncpy(menu[1], "Предыдущая страница", 30);
	strncpy(menu[2], "Перейти к странице по номеру", 30);
	strncpy(menu[3], "Выход из просмотра", 30);

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

//Функция удаления элементов из двусвязного списка
//beg указатель на начало списка
//end указатель на конец списка
//n ссылка на количество элементов в списке
//Функция возвращает структуру pair, в которой содержатся указатели на начало и конец обновлённого списка
pair del(bi_list* beg, bi_list* end, short& n) {
	pair back;
	bi_list* t;
	short k = 0, m, c;
	bool e, o;
	char** menu_1, **menu_2;
	menu_1 = new char*[2];
	menu_2 = new char*[2];
	for (m = 0; m < 2; m++) {
		menu_1[m] = new char[5];
		menu_2[m] = new char[25];
	}
	strncpy(menu_1[0], "Да", 5);
	strncpy(menu_1[1], "Нет", 5);
	strncpy(menu_2[0], "Удалить запись", 25);
	strncpy(menu_2[1], "Выход в главное меню", 25);
	
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

//Процедура редактирования элементов в двусвязном списке
//beg указатель на начало списка
//n количество элементов в списке
void correct(bi_list* beg, short n) {
	bi_list* t;
	flat fl;
	bool e;
	short m, c = 8, i;
	char** menu;
	menu = new char* [10];
	for (i = 0; i < 10; i++)
		menu[i] = new char[35];
	strncpy(menu[0], "Редактировать фамилию", 30);
	strncpy(menu[1], "Редактировать имя", 30);
	strncpy(menu[2], "Редактировать отчество", 30);
	strncpy(menu[3], "Редактировать улицу", 30);
	strncpy(menu[4], "Редактировать номер дома", 30);
	strncpy(menu[5], "Редактировать номер квартиры", 30);
	strncpy(menu[6], "Редактировать количество жильцов", 35);
	strncpy(menu[7], "Редактировать площадь квартиры", 35);
	strncpy(menu[8], "Редактировать другую запись", 35);
	strncpy(menu[9], "Выход в главное меню", 35);

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

//Функция сортировки элементов двусвязного списка в указанной последовательности методом вставки
//beg указатель на начало списка
//d задаёт условие сортировки, значение 1 сортировку по возрастанию, -1 по убыванию
//Функция возвращает структуру pair, в которой содержатся указатели на начало и конец обновлённого списка
pair sort_process(bi_list* beg, short d) {
	pair back;
	bi_list* temp, * t, * t1;
	temp = beg;
	d *= -1;
	while (temp->r != NULL) {
		if (strcmp(temp->r->inf.surname, temp->inf.surname) == d) {
			t1 = temp->r;
			t1->l->r = t1->r;
			if (t1->r)
				t1->r->l = t1->l;
			if (temp->l)
				t = temp->l;
			else
				t = temp;
			while (true) {
				if (strcmp(t1->inf.surname, t->inf.surname) != d) {
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

//Функция инициализирует меню сортировки
//beg указатель на начало двусвязного списка
//Функция возвращает структуру pair, в которой содержатся указатели на начало и конец обновлённого списка
pair sort(bi_list* beg) {
	pair back;
	short c, i;
	char** menu;
	back.beg = NULL;
	back.end = NULL;

	menu = new char* [3];
	for (i = 0; i < 3; i++)
		menu[i] = new char[21];
	strncpy(menu[0], "По возрастанию", 21);
	strncpy(menu[1], "По убыванию", 21);
	strncpy(menu[2], "Выход в главное меню", 21);

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

//Процедура поиска элементов в двусвязном спике по названию улицы
//beg указатель на начало списка
void search(bi_list* beg) {
	bi_list* top = NULL, * end = NULL, * t, * temp;
	short n, c;
	char **menu, s[20] = "";
	bool e;

	menu = new char* [2];
	for (n = 0; n < 2; n++)
		menu[n] = new char[35];
	strncpy(menu[0], "Поиск записей по названию улицы", 35);
	strncpy(menu[1], "Выход в главное меню", 25);
		
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

//Функция определения типа файла
//s путь к файлу
//Функция возвращает true, если файл текстовый, в обратном случае false
bool define(char* s) {
	short n;
	n = strlen(s);
	if (s[n - 1] == 't' && s[n - 2] == 'x' && s[n - 3] == 't')
		return true;
	else
		return false;
}

//Процедура сохранения двусвязного списка в файл
//beg указатель на начало списка
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
					fprintf(f, "| %4hi | %20s | %20s | %20s | ", fl.n, fl.surname, fl.name, fl.patronymic);
					fprintf(f, "%30s | %5hi | %8hi | %14hi | %7.2f |\n", fl.street, fl.house, fl.flat, fl.residents, fl.area);
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

//Функция загрузки двусвязного списка из файла
//beg указатель на начало списка
//k ссылка на переменную, отвечающую за количество элементов в списке
//Функция возвращает структуру pair, в которой содержатся указатели на начало и конец обновлённого списка
pair load(bi_list* beg, short& k) {
	pair back;
	short n, c;
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
				fseek(f, 474, SEEK_SET);
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

//Фукнция добавления элемента в список группировки по названию улицы
//top указатель на начало списка
//inf структура подлежащая добавлению в список
//Фукнция возвращает указатель на начало списка
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

//Процедура печатает шапку результатов обработки списка
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

//Процедура отвечает за выравнивание таблицы обработки за счёт заполнения пустых полей в таблице
void space_write_str() {
	short i;
	indent();
	printf("|");
	for (i = 0; i < 32; i++)
		printf(" ");
}

//Функция вывода списка группировок полученных в результате обработки двусвязного списка
//top указатель на начало списка
//Функция возвращает количество элементов выведенных на экран
short write_str(list * top) {
	short j = 0;
	flat fl = top->inf;
	indent();
	printf("| %30s", top->inf.street);
	printf(" | %4hi | %20s | %20s | %20s | ", fl.n, fl.surname, fl.name, fl.patronymic);
	printf("%5hi | %8hi | %14hi | %7.2f |\n", fl.house, fl.flat, fl.residents, fl.area);
	if (top->next) {
		space_write_str();
		puts("---------------------------------------------------------------------------------------------------------------------------");
		for (top = top->next; top != NULL; top = top->next) {
			j++;
			fl = top->inf;
			space_write_str();
			printf("| %4hi | %20s | %20s | %20s | ", fl.n, fl.surname, fl.name, fl.patronymic);
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

//Процедура инициализации меню просмотра результатов обработки двусвязного списка
//top_str указатель на начало двусвязного списка результатов обработки
//n количество страниц просмотра
void browse_str(street* top_str, short n) {
	char** menu;
	short i, k = 0, page = 1, new_page = 0;
	bool e;

	menu = new char* [4];
	for (i = 0; i < 4; i++) {
		menu[i] = new char[30];
	}
	strncpy(menu[0], "Следующая страница", 30);
	strncpy(menu[1], "Предыдущая страница", 30);
	strncpy(menu[2], "Перейти к странице по номеру", 30);
	strncpy(menu[3], "Выход из просмотра", 30);

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

//Функция создания элемента двусвязного списка для хранения результатов обработки в виде группировки элементов
//fl структура элемента исходно двусвязного списка
//Функция возвращает указатель на полученный элемент списка
street* new_street(flat fl) {
	street* t;
	t = (street*)malloc(size_street);
	t->n = 1;
	strncpy(t->inf,fl.street, 30);
	t->top = (list*)malloc(size_list);
	t->top->inf = fl;
	t->r = NULL;
	t->l = NULL;
	t->top->next = NULL;
	return t;
}

//Функция которая определяет страницу показа элементов результатов обработки при просмотре
//top указатель на начало двусвязного списка, хранящего результаты обработки
//Функция возвращает количество полученных страниц
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

//Процедура проводящая обработку элементов двусвязного списка
//beg указатель на начало двусвязного списка
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
			if (c == 1) {
				if (t->r == NULL) {
					t1 = new_street(beg->inf);
					t1->l = t;
					t->r = t1;
					break;
				}
				else
					t = t->r;
			}
			else if (c == -1) {
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
