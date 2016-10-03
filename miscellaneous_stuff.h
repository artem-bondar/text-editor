#ifndef __MISCELLANEOUS_STUFF_H__
#define __MISCELLANEOUS_STUFF_H__

#define TITLE_SIZE 8
#define TITLE_WIDTH 32
#define LOAD_SCREEN_SIZE 10

#define SINGLE_STRING 1
#define TERM_STRING 2
#define MULTIPLE_STRING 3

typedef enum errors_global
{
	NO_ERROR,
	INVALID_NUMBER_OF_COMMAND_LINE_ARGUMENTS,
	ALLOCATION_ERROR,
	FAILED_TO_WRITE_IN_OUTPUT_STREAM
} errors_global;

typedef enum color
{
	BLACK,
	RED,
	GREEN,
	YELLOW,
	BLUE,
	MAGENTA,
	CYAN,
	WHITE
} color;

typedef enum graphic_rendering_attributes
{
	RESET,
	BOLD,
	ITALIC = 3,
	UNDERLINE,
	BLINK
} graphic_rendering_attributes;

extern const char *title[TITLE_SIZE];

void set_colors(const color foreground, const color background);
void set_attributes(const graphic_rendering_attributes first_attribute, const graphic_rendering_attributes second_attribute);
void reset_colors();
void printf_colored(char const *string, const color foreground, const color background);
void printf_colored_attributed(char const *string, const color foreground, const color background, unsigned int attributes_number, ...);
void show_loading_screen(const unsigned short indent, const unsigned short vertical_indent);
void calculate_indents(unsigned short *indent, unsigned short *vertical_indent, const unsigned short width, const unsigned short height);
const unsigned int log10_of(unsigned int number);
const unsigned int is_second_part_of_control_character(const char character);
const unsigned int get_control_character_by_its_second_part(const char character);
unsigned int detect_string_type(char const *string);
unsigned int detect_number_of_strings_in_line(char *line);
unsigned int detect_number_of_substrings_in_string(char *string, char *substring);
errors_global convert_string_argument(char **string);

#endif /* __MISCELLANEOUS_STUFF_H__ */