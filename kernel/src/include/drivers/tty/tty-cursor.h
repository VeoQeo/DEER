#ifndef TTY_CURSOR_H
#define TTY_CURSOR_H

// ПОЛУЧИТЬ ТЕКУЩУЮ КООРДИНАТУ КУРСОРА 
typedef struct {
    int x;
    int y;

} current_coordinates;

current_coordinates get_cursor_position();
void update_cursor_position(int y, int x);
void set_cursor_shape();
// void set_cursor_shape(uint8_t start_line, uint8_t end_line, bool blink_enable, bool visible);

#endif