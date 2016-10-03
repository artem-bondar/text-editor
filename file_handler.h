#ifndef __FILE_HANDLER_H__
#define __FILE_HANDLER_H__

#include "terminal_controller.h"
#include "text_database.h"

#define INPUT_BUFFER_SIZE 256

typedef enum errors_file_IO
{
	FAILED_TO_OPEN_FILE = 20,
	READ_FROM_FILE_ERROR,
	WRITE_TO_FILE_ERROR,
	FAILED_TO_READ_COMMAND,
	END_OF_FILE_ALREADY_REACHED
} errors_file_IO;

errors_file_IO load_text_from_file(Text *text, char const *path);
errors_global create_and_save_line(Text *text, char *content);
errors_file_IO save_text_to_file(Text *text, char const *path);
errors_file_IO get_command_text(char **command);
errors_file_IO print_text(Text const *text, run_mode mode, const unsigned int screen_width);
errors_global print_line_with_interface(Text const *text, Line *line, int line_number, const unsigned int screen_width, unsigned int *truncates_counter);
errors_global print_line_truncated(char const *line, const unsigned int tab_width, const unsigned int indent, const unsigned int print_width, unsigned int *truncates_counter);
errors_global print_line_continued(char const *line, const unsigned int tab_width, const unsigned int shift, int print_width);
errors_global print_text_in_interactive_mode(Text *text, const unsigned int screen_width, const unsigned int screen_height);
errors_global print_text_screen(Text *text, const unsigned int screen_width);

#endif /* __FILE_HANDLER_H__ */