#include "../../libc/string.h"

#include "../../include/drivers/tty/main-tty-struct.h"
#include "../../include/drivers/tty/tty-render.h"
#include "../../include/drivers/tty/tty-cell.h"
#include "../../include/drivers/tty/tty-cursor.h"
#include "../../include/drivers/tty/tty-getchar.h"
#include "../../include/drivers/tty/tty-putchar.h"
#include "../../include/drivers/tty/tty-terminal-control.h"


// ОЧИСТИТЬ BACKLOG
void terminal_clear() {
    struct tty_parameters *parameter_info = interact_tty_parameters(); //<<ПОЛУЧАЕМ ДОСТУП ДО ГЛАВНОЙ TTY СТРУКТУРЕ

    // ~сдвигаем границы и курсор в начало
    parameter_info -> upper_limit = 0; 
    parameter_info -> lower_limit = 0;

    parameter_info -> cursor_position.y = 0;
    parameter_info -> cursor_position.x = VGA_SERVICE_LENGTH;

    parameter_info -> getchar_start_at = 0;

    // 1. Очищаем служебные поля и беклог теми же данными, что и при инициализации 
    // 1.1 Создаем пустую ячейку (цвет фона и символа должны совпадать)
    uint8_t tty_background_color_theme = parameter_info -> tty_background_color_theme;
    uint8_t blended_color = vga_blend_color(tty_background_color_theme, tty_background_color_theme);  
    uint8_t symbol = ' ';

    for (int str = 0; str < VGA_BACKLOG_HEIGHT && str < VGA_SERVICE_HEIGHT; str++) {
        
        // 1.2.1. Инициализируем служебные поля
        for (int service_col = 0; service_col < VGA_SERVICE_LENGTH; service_col++) {
            parameter_info -> backlog.blended_colors[str][service_col] = blended_color;
            parameter_info -> backlog.symbols[str][service_col] = symbol;
        }

        // 1.2.2. Инициализируем беклог
        for (int back_col = VGA_SERVICE_LENGTH; back_col < VGA_SERVICE_LENGTH; back_col++) {
            parameter_info -> service_field.blended_colors[str][back_col] = blended_color;
            parameter_info -> service_field.symbols[str][back_col] = symbol;
        }
    }
}

// ОБРАБОТАТЬ ВВЕДЕННЫЙ ПРОМТ
void proccess_command() {
    struct tty_parameters *parameter_info = interact_tty_parameters(); //<<ПОЛУЧАЕМ ДОСТУП ДО ГЛАВНОЙ TTY СТРУКТУРЕ
    current_coordinates coordinate = get_cursor_position();            //<<получаем текущую координату

    int y = coordinate.y;
    int x = coordinate.x;

    /*1. ОБРАБОТКА ВВЕДЕННОГО ПРОМТА
        do_command();
        
        *изменяем координаты
    */

    // *пока вручную, так как комманд у нас пока нету
    y++;
    x = VGA_SERVICE_LENGTH;

    // 2. Обнуляем буфер и кол-во символов. ВАЖНО, весь буфер ПЕРВОНАЧАЛЬНО инициализирован пробелами
    for (int i = 0; i < GETCHAR_MAX_LENGTH; i++) {
        parameter_info -> getchar_buffer[i] = ' ';
    }

    parameter_info -> getchar_buffer_length = 0;

    parameter_info -> cursor_position.y = y;
    parameter_info -> cursor_position.x = x;
}

// РАСПЕЧАТАТЬ ИМЯ ПОЛЬЗОВАТЕЛЯ (СРАБАТЫВАЕТ ПРИ НАЖАТИИ НА ENTER)
void print_username() {
    struct tty_parameters *parameter_info = interact_tty_parameters(); //<<ПОЛУЧАЕМ ДОСТУП ДО ГЛАВНОЙ TTY СТРУКТУРЕ
    current_coordinates coordinate = get_cursor_position();            //<<получаем текущую координату

    int y = coordinate.y;  //<<по факту нужна только эта 
    // int x = coordinate.x;

    // 1. Получаем имя пользователя
    int user_name_length = parameter_info -> service_field.user_name_length;

    uint8_t user_name[user_name_length]; 
    strcpy((char*)user_name, (const char*)parameter_info -> service_field.user_name);

    // 2. Добавляем имя пользователя по переданному индексу строки в ее начало
    for (int i = 0; i < user_name_length; i++) {
   
    /*  пока заглушка, но в будущем добавляем ": " в конец имени
        if (i == user_name_length) {
            parameter_info -> service_field.symbols[y][i] = ':';
        }
    */
        parameter_info -> service_field.symbols[y][i] = user_name[i];
    }

    parameter_info -> getchar_start_at = y; // храним номер строки, в которой распечатали имя
}

// ОТРИСОВАТЬ ВИДИМЫЙ ЭКРАН 
void render() {
    struct tty_parameters *parameter_info = interact_tty_parameters(); //<<ПОЛУЧАЕМ ДОСТУП ДО ГЛАВНОЙ TTY СТРУКТУРЫ

    int upper_limit = parameter_info -> upper_limit;
    int lower_limit = parameter_info -> lower_limit;


   /* * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
    *                                                         *
    *         VGA_BUFFER_ADDRESS[0]   <-- cell[0][0]          *
    *         VGA_BUFFER_ADDRESS[1]   <-- cell[0][1]          *
    *                                                         *
    * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

    
    // 1. Сначала отрисовываем историю из service_field и backlog
    int VGA_ORIGIN = 0;  //<<с этого числа считаем индекс строки для VGA памяти (так как если будем считать по upper_limit и lower_limit) - мы выйдем за пределы памяти

    for (int str = upper_limit; str < lower_limit; str++, VGA_ORIGIN++) {
        
        // 1.1. Для служебного поля   
        for (int service_col = 0; service_col < VGA_SERVICE_LENGTH; service_col++) {
            int current_index = VGA_ORIGIN * VGA_OUTPUT_LENGTH + service_col;  //<<считаем линейный индекс VGA для отрисовки 

            uint8_t blended_color = parameter_info -> service_field.blended_colors[str][service_col];
            uint8_t symbol        = parameter_info -> service_field.symbols      [str][service_col];

            uint16_t cell = vga_cell(symbol, blended_color);                      // соединяем символ и цвет
            parameter_info -> vga_memory_address[current_index] = cell;          // передаем в VGA
        }

        // 1.2. Для истории ввода
        for(int back_col = VGA_SERVICE_LENGTH; back_col < VGA_OUTPUT_LENGTH; back_col++) {
            int current_index = VGA_ORIGIN * VGA_OUTPUT_LENGTH + back_col;    //<<считаем линейный индекс VGA для отрисовки 

            uint8_t blended_color = parameter_info -> backlog.blended_colors[str][back_col];
            uint8_t symbol        = parameter_info -> backlog.symbols      [str][back_col];

            uint16_t cell = vga_cell(symbol, blended_color);                      // соединяем символ и цвет
            parameter_info -> vga_memory_address[current_index] = cell;          // передаем в VGA
        }
    }

    // 2. Затем отрисовывем getchar (если есть). Отдельно мы не храним цвета символов из буффера getchar, поэтому достаем цвет и бадяжим на ходу
    uint8_t tty_background_color_theme = terminal_get_color(BACKGROUND);    
    uint8_t tty_symbol_color_theme     = terminal_get_color(SYMBOL);      

    uint8_t blended_color = vga_blend_color(tty_symbol_color_theme, tty_background_color_theme);

    if (parameter_info -> getchar_buffer_length > 0) {
        int current_index = (parameter_info -> getchar_start_at - upper_limit) * VGA_OUTPUT_LENGTH + VGA_SERVICE_LENGTH;  

        for (int get_col = 0; get_col < GETCHAR_MAX_LENGTH; get_col++, current_index++) {
            uint8_t symbol = parameter_info -> getchar_buffer[get_col];
            uint16_t cell = vga_cell(symbol, blended_color);                      // соединяем символ и цвет

            parameter_info -> vga_memory_address[current_index] = cell;          // передаем в VGA
        }
    }

    int y = parameter_info -> cursor_position.y - upper_limit; 
    int x = parameter_info -> cursor_position.x;

    update_cursor_position(y, x);
}

// ОБНОВИТЬ ТЕРМИНАЛ
void terminal_update(update_request_t *request) {
    struct tty_parameters *parameter_info = interact_tty_parameters(); //<<ПОЛУЧАЕМ ДОСТУП ДО ГЛАВНОЙ TTY СТРУКТУРЫ

    // II. ФОРМИРОВАНИЕ КАДРА ДЛЯ ОТРИСОВКИ
    switch (request -> mode) {
    
        case MODE_INIT: {
            
            // ~разбираем запрос
                strcpy((char*)parameter_info -> service_field.user_name, (const char*)request -> params.init_data.user_name);
                parameter_info -> service_field.user_name_length = request -> params.init_data.user_name_length;

                parameter_info -> tty_symbol_color_theme     = request -> params.init_data.tty_background_color_theme; 
                parameter_info -> tty_background_color_theme = request -> params.init_data.tty_symbol_color_theme;     

                parameter_info -> cursor_position.y = request -> params.init_data.y;
                parameter_info -> cursor_position.x = request -> params.init_data.x;

                parameter_info -> isEnter = request -> params.init_data.isEnter; // после отрисовки заполняем левую границу именем пользователя

                parameter_info -> upper_limit = request -> params.init_data.upper_limit = 0;                    //<<ВЕРХНЯЯ ГРАНИЦА ВИДИМОГО ЭКРАНА
                parameter_info -> lower_limit = request -> params.init_data.lower_limit = 0;                    //<<НИЖНЯЯ ГРАНИЦА ВИДИМОГО ЭКРАНА

            // 1. Инициализируем беклог переданными стартовыми данными
                // 1.1 Создаем пустую ячейку (цвет фона и символа должны совпадать)
                uint8_t tty_background_color_theme = parameter_info -> tty_background_color_theme;
                uint8_t blended_color = vga_blend_color(tty_background_color_theme, tty_background_color_theme);  
                uint8_t symbol = ' ';

                // 1.2. Заполняем данными служебные поля и беклог
                for (int str = 0; str < VGA_BACKLOG_HEIGHT && str < VGA_SERVICE_HEIGHT; str++) {
                    
                    // 1.2.1. Инициализируем служебные поля
                    for (int service_col = 0; service_col < VGA_SERVICE_LENGTH; service_col++) {
                        parameter_info -> backlog.blended_colors[str][service_col] = blended_color;
                        parameter_info -> backlog.symbols[str][service_col] = symbol;
                    }

                    // 1.2.2. Инициализируем беклог
                    for (int back_col = VGA_SERVICE_LENGTH; back_col < VGA_SERVICE_LENGTH; back_col++) {
                        parameter_info -> service_field.blended_colors[str][back_col] = blended_color;
                        parameter_info -> service_field.symbols[str][back_col] = symbol;
                    }
                }

                break;
            }
            
        case MODE_SCROLL_DOWN: {    /*СКРОЛЛИМ ДО НИЖНЕГО ПРЕДЕЛА */

            // ~достаем данные 
                    int lines = request -> params.scroll_down.lines;
                    
            // 1. Проверяем, не находимся ли в самом низу беклога
                    if (parameter_info -> lower_limit == VGA_BACKLOG_HEIGHT) {
                        break; 
                    }

            // 2. Вычисляем как далеко можем крутить
                int how_long = parameter_info -> lower_limit + lines;

            // 3. Если новая нижняя граница превышает конец всего backlog
                if (how_long > VGA_BACKLOG_HEIGHT) {
                    
                    // 3.1. Устанавливаем нижнюю границу в конец backlog
                    parameter_info -> lower_limit = VGA_BACKLOG_HEIGHT;
                    parameter_info -> upper_limit = VGA_BACKLOG_HEIGHT - VGA_OUTPUT_HEIGHT;

                } else { 

                    // 3.2. Просто сдвигаем границы вниз
                    parameter_info -> lower_limit = how_long;
                    parameter_info -> upper_limit = how_long - VGA_OUTPUT_HEIGHT;
                }

            // 4. Доп проверка
                if (parameter_info -> upper_limit < 0) {
                    parameter_info -> upper_limit = 0;
                    parameter_info -> lower_limit = VGA_OUTPUT_HEIGHT;
                } 

                break;
            }
            
        case MODE_SET_COLOR_THEME: {

            // ~достаем новую цветовую тему
                uint8_t tty_symbol_color_theme     = request -> params.set_color.symbol_color_theme;
                uint8_t tty_background_color_theme = request -> params.set_color.background_color_theme;

                uint8_t blended_color = vga_blend_color(tty_background_color_theme, tty_symbol_color_theme);

            // 1. Обновляем цвет в служебной части и беклоге
                for (int str = 0; str < VGA_BACKLOG_HEIGHT && str < VGA_SERVICE_HEIGHT; str++) {
                    
                    // 1.1. Обновляем в служебной части
                    for (int service_col = 0; service_col < VGA_SERVICE_LENGTH; service_col++) {
                        parameter_info -> backlog.blended_colors[str][service_col] = blended_color;
                    }

                    // 1.2. Обновляем в беклоге 
                    for (int back_col = VGA_SERVICE_LENGTH; back_col < VGA_SERVICE_LENGTH; back_col++) {
                        parameter_info -> service_field.blended_colors[str][back_col] = blended_color;
                    }
                }

                break;
            }

        case MODE_RETURN_CARRIEGE_INPUT: {

            // ~достаем актуальные координаты курсора
                current_coordinates coordinate = get_cursor_position(); //<<получаем текущую координату
                int y = coordinate.y;
                int x = coordinate.x;

            // 1. Если уже уперлись в начало, просто выходим
                if (x == VGA_SERVICE_LENGTH) break; // если начало координат, просто выходим

            // 2. В ином случае выставляем x в начало по координате y  
                parameter_info -> cursor_position.y = y;
                parameter_info -> cursor_position.x = VGA_SERVICE_LENGTH; // если не начало координат, возвращаемся к служебной границе

                break;
            }

        case MODE_BACKSPACE_INPUT: {

                // ~получить координаты 
                    current_coordinates coordinate = get_cursor_position();
                    int y = coordinate.y; 
                    int x = coordinate.x;

                // 1. Проверяем, можно ли вообще удалять
                    // Если getchar_buffer пуст, или курсор находится в самом начале области ввода (после промпта)
                    // ИЛИ если курсор уже на самой первой позиции ввода (VGA_SERVICE_LENGTH)
                    if (parameter_info -> getchar_buffer_length == 0 || (y == parameter_info -> getchar_start_at && x == VGA_SERVICE_LENGTH)) {
                        break; 
                    }

                // 2. Вычисляем индекс в command_buffer, который нужно удалить
                    // Этот индекс соответствует символу ПЕРЕД текущей позицией курсора.
                    int PROMPT_LENGTH = VGA_OUTPUT_LENGTH - VGA_SERVICE_LENGTH;           
                    int getchar_start_at = parameter_info -> getchar_start_at;          

                    int current_index = (y - getchar_start_at) * PROMPT_LENGTH + (x - VGA_SERVICE_LENGTH) - 1;

                    // 2.1. Дополнительная проверка на всякий случай, чтобы не выйти за границы
                    if (current_index < 0) {
                        break; 
                    }

                    // 2.2. Сдвигаем command_buffer влево
                    // Все символы, начиная с current_index, сдвигаются влево.
                    // Последний символ (который теперь дублируется) затирается пробелом.
                    for (int i = current_index; i < parameter_info -> getchar_buffer_length - 1; i++) {
                        parameter_info -> getchar_buffer[i] = parameter_info -> getchar_buffer[i + 1];
                    }

                // 3. Уменьшаем длину и очищаем последний символ (который теперь "лишний")
                    parameter_info -> getchar_buffer_length--;
                    parameter_info -> getchar_buffer[parameter_info -> getchar_buffer_length] = ' '; // Затираем пробелом

                    x--; 

                // 4. Проверяем перенос строки назад
                    if (x < VGA_SERVICE_LENGTH) { // Если курсор "ушёл" за начало области ввода
                        // Если мы не на самой первой строке ввода, переходим на конец предыдущей
                        if (y > parameter_info -> getchar_start_at) { // prompt_start_line - это логическая строка начала ввода
                            y--; // Переходим на предыдущую строку
                            x = VGA_OUTPUT_LENGTH - 1; // В конец предыдущей строки
                        } else {
                            // Если мы на первой строке ввода и пытаемся удалить дальше,
                            // курсор должен остаться на (prompt_start_line, VGA_SERVICE_LENGTH)
                            x = VGA_SERVICE_LENGTH;
                        }
                    }

                // 5. Обновляем глобальные координаты курсора
                    parameter_info -> cursor_position.y = y;
                    parameter_info -> cursor_position.x = x;

                break;
            }
        
        case MODE_TAB_INPUT: {

            // ~получаем координаты 
                current_coordinates coordinate = get_cursor_position(); //<<получаем текущую координату
                int y = coordinate.y;
                int x = coordinate.x;

            // 1. Проверяем, хватит ли места в буфере для всех пробелов
                if (parameter_info -> getchar_buffer_length + TAB > GETCHAR_MAX_LENGTH) {
                    // '\a'
                    break; 
                }

            // 2. Сдвигаем все символы, начиная с current_index, на TAB позиций вправо, итерируемся getchar_buffer_length элементов 
                // 2.1. Для начала рассчитаем текущий индекс getchar_buffer, на который указывает курсор. Немного математики: 
                int PROMPT_LENGTH = VGA_OUTPUT_LENGTH - VGA_SERVICE_LENGTH;           //<<тут храним длину поля, в которое пользователь может писать (вся ширина экрана - ширина служебных полей)
                int getchar_buffer_length = parameter_info -> getchar_buffer_length; //<<просто достаем текущую длину буфера (для удобства)
                int getchar_start_at = parameter_info -> getchar_start_at;          //<<просто достаем координату 'y', в которой был введен первый символ  

                // int count_of_str = (getchar_buffer_length + PROMPT_LENGTH - 1) / PROMPT_LENGTH; // получаем кол-во строк, которое по факту занимает текущее содержимое getchar_buffer на экране
                int current_index = (y - getchar_start_at) * PROMPT_LENGTH + (x - VGA_SERVICE_LENGTH); // получаем индекс getchar_buffer, на который указывает курсор 

                // 2.2. Двигаем все символы на [значение TAB] позиций 
            for (int get_col = getchar_buffer_length + TAB - 1; get_col >= current_index + TAB; get_col--) {
                    parameter_info -> getchar_buffer[get_col] = parameter_info -> getchar_buffer[get_col - TAB];
                }
                
                // 2.3. Вставляем [значение TAB] пробелов 
                for (int get_col = 0; get_col < TAB; get_col++, current_index++) {
                    parameter_info -> getchar_buffer[current_index] = ' ';
                }

                parameter_info -> getchar_buffer_length += TAB;

                x += TAB;

                // 3. Проверяем, нужно ли переносить на новую строку из-за TAB
                if (x > VGA_OUTPUT_LENGTH - TAB) {
                    x = VGA_SERVICE_LENGTH;  // Начинаем с начала служебной границы
                    y++;                    // Переходим на следующую строку

                }

                // 4. Проверяем, нужно ли скроллить
                if (y >= parameter_info -> lower_limit) { 
                    parameter_info -> upper_limit++;
                    parameter_info -> lower_limit++;
                }

                parameter_info -> cursor_position.y = y;
                parameter_info -> cursor_position.x = x;

                break;
            }

        case MODE_SPACE_INPUT: {
            
            // ~получаем координаты 
                current_coordinates coordinate_space = get_cursor_position();
                int y = coordinate_space.y; 
                int x = coordinate_space.x; 

            // 1. Проверяем, хватит ли места в буфере для пробела
                if (parameter_info -> getchar_buffer_length + SPACE > GETCHAR_MAX_LENGTH) {
                    // '\a'
                    break; 
                }

            // 2. Сдвигаем все символы, начиная с current_index, на SPACE позиций вправо, итерируемся getchar_buffer_length элементов 
                // 4.1. Для начала рассчитаем текущий индекс getchar_buffer, на который указывает курсор. Немного математики: 
                int PROMPT_LENGTH = VGA_OUTPUT_LENGTH - VGA_SERVICE_LENGTH;           //<<тут храним длину поля, в которое пользователь может писать (вся ширина экрана - ширина служебных полей)
                int getchar_buffer_length = parameter_info -> getchar_buffer_length; //<<просто достаем текущую длину буфера (для удобства)
                int getchar_start_at = parameter_info -> getchar_start_at;          //<<просто достаем координату 'y', в которой был введен первый символ  

                // int count_of_str = (getchar_buffer_length + PROMPT_LENGTH - 1) / PROMPT_LENGTH;           // получаем кол-во строк, которое по факту занимает текущее содержимое getchar_buffer на экране
                int current_index = (y - getchar_start_at) * PROMPT_LENGTH + (x - VGA_SERVICE_LENGTH);   // получаем индекс getchar_buffer, на который указывает курсор 

                // 2.2. Двигаем все символы на [значение SPACE] позиций 
                for (int get_col = getchar_buffer_length; get_col > current_index; get_col--) {
                    parameter_info -> getchar_buffer[get_col] = parameter_info -> getchar_buffer[get_col - SPACE];
                }

                // 2.3. Вставляем [значение SPACE]  
                parameter_info -> getchar_buffer[current_index] = ' ';
                parameter_info -> getchar_buffer_length += SPACE;

                x++;

            // 3. Проверяем, нужно ли переносить на новую строку из-за пробела
                if (x >= VGA_OUTPUT_LENGTH) {
                    x = VGA_SERVICE_LENGTH;   // Начинаем с начала служебной границы
                    y++;                     // Переходим на следующую строку
                } 

            // 4. Проверяем, нужно ли скроллить
                if (y >= parameter_info -> lower_limit) {                
                    parameter_info -> upper_limit++;
                    parameter_info -> lower_limit++;
                }

                parameter_info -> cursor_position.y = y;
                parameter_info -> cursor_position.x = x;

                break;
            }

        case MODE_CHAR_INPUT: { 
            
            // ~Получаем координаты
                current_coordinates coordinate = get_cursor_position();
                int y = coordinate.y; 
                int x = coordinate.x;

                uint8_t symbol = request -> params.char_input.symbol;

            // 1. Проверяем, влезет ли символ в буффер
                if (parameter_info -> getchar_buffer_length >= GETCHAR_MAX_LENGTH) {
                    // '\a'
                    break; 
                }

            // 2. Сопоставляем координату с getchar_buffer и находим текущий индекс, куда запишем символ 
                int PROMPT_LENGTH = VGA_OUTPUT_LENGTH - VGA_SERVICE_LENGTH;           //<<тут храним длину поля, в которое пользователь может писать (вся ширина экрана - ширина служебных полей)
                int getchar_buffer_length = parameter_info -> getchar_buffer_length; //<<просто достаем текущую длину буфера (для удобства)
                int getchar_start_at = parameter_info -> getchar_start_at;          //<<просто достаем координату 'y', в которой был введен первый символ  

                // int count_of_str = (getchar_buffer_length + PROMPT_LENGTH - 1) / PROMPT_LENGTH; // получаем кол-во строк, которое по факту занимает текущее содержимое getchar_buffer на экране
                int current_index = (y - getchar_start_at) * PROMPT_LENGTH + (x - VGA_SERVICE_LENGTH); // получаем индекс getchar_buffer, на который указывает курсор 
                
                // 2.1. Сдвигаем все символы, начиная с current_index, на одну позицию вправо (итерируемся с конца)
                for (int get_col = getchar_buffer_length; get_col > current_index; get_col--) {
                    parameter_info -> getchar_buffer[get_col] = parameter_info -> getchar_buffer[get_col - 1];
                }

                parameter_info -> getchar_buffer[current_index] = symbol;
                parameter_info -> getchar_buffer_length++;

                x++;

            // 3. Проверяем, нужно ли переносить на новую строку из-за введенного символа 
                if (x > VGA_OUTPUT_LENGTH - 1) {
                    x = VGA_SERVICE_LENGTH;   // Начинаем с начала служебной границы
                    y++;                     // Переходим на следующую строку
                } 

            // 4. Проверяем, нужно ли скроллить
                if (y >= parameter_info -> lower_limit) {                
                    parameter_info -> upper_limit++;
                    parameter_info -> lower_limit++;
                }

                parameter_info -> cursor_position.y = y;
                parameter_info -> cursor_position.x = x;

                break;
            }

        case MODE_BACKSPACE_OUTPUT: {
            
            // ~достаем координаты текущей свободной ячейки 
                int y = parameter_info -> putchar_free_cell[0];    
                int x = parameter_info -> putchar_free_cell[1];   

            // 1. Проверяем, можно ли вообще удалять

                // 1.1. Если курсор уже в самом начале пользовательской части строки
                if (x == VGA_SERVICE_LENGTH && y == 0) { 
                    break; 
                }

                // 1.2. Переходим на конец предыдущей строки
                if (x == VGA_SERVICE_LENGTH) { 
                    y--;
                    x = VGA_OUTPUT_LENGTH - 1;

                } else {
                    x--;
                }

            // 2. Затираем символ пробелом в putchar_buffer
                // (Мы не "сдвигаем" буфер, как в getchar, потому что putchar_buffer - это сетка,
                // и мы просто затираем символ в текущей позиции, а не удаляем его из "строки".)
                parameter_info -> putchar_buffer[y][x] = ' '; 

            // 3. Обновляем глобальные координаты свободной ячейки
                parameter_info -> putchar_free_cell[0] = y;
                parameter_info -> putchar_free_cell[1] = x;

                break;
            }

        case MODE_TAB_OUTPUT: {

            // ~получаем координаты пустой ячейки сетки 
                int y = parameter_info -> putchar_free_cell[0];    //<<получаем координаты пустой ячейки
                int x = parameter_info -> putchar_free_cell[1];   //<<получаем координаты пустой ячейки

            // 1. Если размер таба превышает остаток до края монитора, заполняем остаток и переходим на новую строку
                if (x > VGA_OUTPUT_LENGTH - TAB - 1) {
                    int iteration_to_end = VGA_OUTPUT_LENGTH - x;
                    int i = 0;

                    while (i < iteration_to_end) {
                        parameter_info->putchar_buffer[y][x] = ' ';
                        
                        x++; // *если мы на последей ячейке, и перешли на ячейку = VGA_OUTPUT_LENGTH, ошибки не будет
                        i++;
                    }

                    y++;
                    x = VGA_SERVICE_LENGTH;  // *так как в следующей итерации мы выйдем из цикла и присвоим значение служебной границы

                    parameter_info->putchar_free_cell[0] = y;
                    parameter_info->putchar_free_cell[1] = x;

                    break;
                }

            // 2. Если длина оставшегося пространства до края монитора больше размера таб, то заполняем буфер кол-вом пробелов для таб (по умолчанию 4)
                int i = 0;

                while (i != TAB) {
                    parameter_info->putchar_buffer[y][x] = ' ';
                    x++;
                    i++;
                }

                parameter_info -> putchar_free_cell[0] = y;
                parameter_info -> putchar_free_cell[1] = x;
                
                break;
            }
            
        case MODE_RETURN_CARRIAGE_OUTPUT: {

            // ~получаем координаты пустой ячейки сетки 
                int y = parameter_info -> putchar_free_cell[0];    //<<получаем координаты пустой ячейки
                int x = parameter_info -> putchar_free_cell[1];   //<<получаем координаты пустой ячейки

                if (x == VGA_SERVICE_LENGTH) return;

                x = VGA_SERVICE_LENGTH;

                parameter_info -> putchar_free_cell[0] = y;
                parameter_info -> putchar_free_cell[1] = x;

                break;
            }

        case MODE_CHAR_OUTPUT: {
            
            // ~получаем координаты пустой ячейки сетки и символ
                int y = parameter_info -> putchar_free_cell[0];    //<<получаем координаты пустой ячейки
                int x = parameter_info -> putchar_free_cell[1];   //<<получаем координаты пустой ячейки

                uint8_t symbol = request -> params.char_output.symbol;

            // 1. Если полученная координата колонки (x) расположена у края, отрисовываем символ вначале строки
                if (x == VGA_OUTPUT_LENGTH - 1) {
                    parameter_info -> putchar_buffer[y][x] = symbol;
                    y++;
                    x = VGA_SERVICE_LENGTH;

                    parameter_info -> putchar_free_cell[0] = y;
                    parameter_info -> putchar_free_cell[1] = x;

                    break;
                }

            // 2. Если есть место для символа
                parameter_info -> putchar_buffer[y][x] = symbol;
                x++;

                parameter_info -> putchar_free_cell[0] = y;
                parameter_info -> putchar_free_cell[1] = x;

                break;
            }

        case MODE_ENTER_OUTPUT: {

            // ~получаем координаты свободной ячейки
                int y = parameter_info -> putchar_free_cell[0];    //<<получаем координаты пустой ячейки
                int x = parameter_info -> putchar_free_cell[1];   //<<получаем координаты пустой ячейки

                y++;
                x = VGA_SERVICE_LENGTH;

                parameter_info -> putchar_free_cell[0] = y;
                parameter_info -> putchar_free_cell[1] = x;

                break;
            }
                
        case MODE_ENTER_INPUT: {

            // ~достаем координаты, выставляем флаг isEnter = true; 
                parameter_info -> isEnter = true;

                current_coordinates coordinate = get_cursor_position();
                int y = coordinate.y; 
                int x = coordinate.x;

            // 1. Если есть какие-то введенные данные, переносим в беклог
                if (parameter_info -> getchar_buffer_length != 0) {
                    uint8_t tty_background_color_theme = terminal_get_color(BACKGROUND);                           
                    uint8_t tty_symbol_color_theme     = terminal_get_color(SYMBOL);                                  
                    uint8_t blended_color              = vga_blend_color(tty_background_color_theme, tty_symbol_color_theme); 

                    int getchar_start_at = parameter_info -> getchar_start_at;
                    int getchar_buffer_length = parameter_info -> getchar_buffer_length;

                    int PROMPT_LENGTH = VGA_OUTPUT_LENGTH - VGA_SERVICE_LENGTH;            //<<длина поля для ввода 
                    int GETCHAR_CURRENT_HEIGHT = (getchar_buffer_length + PROMPT_LENGTH - 1) / PROMPT_LENGTH;  //<<получаем текущую высоту
                    
                    int current_index = 0;
                    
                    for (int str = getchar_start_at; str < getchar_start_at + GETCHAR_CURRENT_HEIGHT; str++) {

                        for (int col = VGA_SERVICE_LENGTH; col < VGA_OUTPUT_LENGTH && current_index < getchar_buffer_length; col++, current_index++) {
                            parameter_info -> backlog.symbols[str][col] = parameter_info -> getchar_buffer[current_index];
                            parameter_info -> backlog.blended_colors[str][col] = blended_color;
                        }
                    }

                    // ~очистка будет в command_proccess 
                }

            // 2. Переходим на строку ниже
                y++;
                x = VGA_SERVICE_LENGTH;

                // 2.1. Проверяем, нужно ли скроллить
                if (y >= VGA_OUTPUT_HEIGHT) { 
                    
                    // 2.2. Увеличиваем upper_limit и lower_limit
                    parameter_info -> upper_limit++;
                    parameter_info -> lower_limit++;
                }

                // 3.2. Также нужно обновить prompt_start_line, так как она тоже "съе...уехала в общем"
                parameter_info -> getchar_start_at = y;

                parameter_info -> cursor_position.y = y;
                parameter_info -> cursor_position.x = x;

                break;
            }

        default: break;
    }

    // II. ОТРИСОВКА 
    if (parameter_info -> isEnter) {
        int PROMPT_LENGTH = VGA_OUTPUT_LENGTH - VGA_SERVICE_LENGTH;
        int GETCHAR_MAX_HEIGHT = (GETCHAR_MAX_LENGTH + PROMPT_LENGTH - 1) / PROMPT_LENGTH;
    
        // 2.1. Находим максимальную высоту, который может занять введенный текст и проверяем, остается ли у нас место до очистки
        if(parameter_info -> lower_limit >= VGA_BACKLOG_HEIGHT - GETCHAR_MAX_HEIGHT) {

            void terminal_clear();
        }

        // 2.2. Выполняем введенный промт и отрисовываем имя пользователя
        proccess_command();
        print_username();

        parameter_info -> isEnter = false;
    }

    // 3. ГЛАВНАЯ ФУНКЦИЯ РЕНДЕРА, отрисовываем текущий кадр по обновленным данным 
    render();
}
