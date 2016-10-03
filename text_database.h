#ifndef __TEXT_DATABASE_H__
#define __TEXT_DATABASE_H__

#include "miscellaneous_stuff.h"

#define TRUNCATE 0
#define CONTINUE 1

#define DEFAULT_INTERFACE_SIZE 4
#define DEFAULT_TAB_WIDTH 8
#define DEFAULT_NUMBER_INDENT 2

#define UNSAVED 0
#define SAVED 1

#define AFTER_LINE 0
#define BEFORE_LINE 1

#define INVALID_POSITION_IN_LINE 44
	 /* see enum errors_commands */

#define REPLACES_COUNT_DELTA 3

typedef struct Line
{
	char *content;
	unsigned int size;
	struct Line *next_line;
} Line;

typedef struct Color_scheme
{
	color left_column, line_numbers, text, background;
} Color_scheme;

typedef struct Text
{
	Line *first_line, *last_line;
	char *file_association;
	unsigned int size, show_range_start, show_range_end; /* starts from 1 */
	unsigned short tab_width, wrap, shift, number_indent, interface_indent, save_status;
	Color_scheme color_scheme;
} Text;

errors_global initialize_empty_text(Text **text);
void deinitialize_text(Text *text);
void update_indents(Text *text);
void insert_line_to_first_position(Text *text, Line *line);
void insert_line_to_last_position(Text *text, Line *line);
void insert_block_of_lines_after_position(Text *text, Line *block_start, const unsigned int position);
void iterate_line_pointer(Text const *text, Line **current_line, unsigned int i);
void delete_line(Text *text, const unsigned int line_number);
void delete_range(Text *text, const unsigned int range_start, const unsigned int range_end);
errors_global edit_line(Text *text, const unsigned int line_number, const unsigned int position, char replace_symbol);
errors_global insert_symbol(Text *text, const unsigned int line_number, unsigned int position, char insert_symbol);
errors_global transform_multiply_strings_in_lines(char *string, Line **result);
void normalize_text_ending(Text *text);
errors_global normalize_lines_endings(Text *text);
errors_global expand_duplicated_lines(Text *text);
errors_global delete_braces_from_text(Text *text, const unsigned int range_start, const unsigned int recommended_range_end);
errors_global add_string_to_range(const unsigned int add_mode, Text *text, char *line, const unsigned int range_start, const unsigned int range_end);
const unsigned short replace_substring_in_range(Text *text, char *substring, char *replacement_pattern, const unsigned int range_start, const unsigned int range_end);

#endif /* __TEXT_DATABASE_H__ */