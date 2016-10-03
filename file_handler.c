#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "file_handler.h"

errors_file_IO load_text_from_file(Text *text, char const *path)
{
	unsigned int buffers_used = 0, length;
	char *read_string, *read_result;
	FILE *file = fopen(path, "r");
	if (file == NULL)
		return FAILED_TO_OPEN_FILE;

	for (;;)
	{
		if (!buffers_used)
		{
			read_string = (char*)calloc(1, sizeof(char) * INPUT_BUFFER_SIZE);
			if (read_string == NULL)
				return ALLOCATION_ERROR;
			buffers_used++;
			read_result = fgets(read_string, INPUT_BUFFER_SIZE, file);
			if (read_result == NULL)
			{
				if (ferror(file))
					return READ_FROM_FILE_ERROR;
				if (feof(file))
				{
					if (read_string[0] != '\0')
					{
						if (create_and_save_line(text, read_string))
							return ALLOCATION_ERROR;
					}
					else
						free(read_string);
					break;
				}
			}
		}
		else
		{
			length = strlen(read_string);
			if (length < INPUT_BUFFER_SIZE * buffers_used - 1 ||
			   (length == INPUT_BUFFER_SIZE * buffers_used - 1 && read_string[INPUT_BUFFER_SIZE * buffers_used - 1] == '\n'))
			{
				if (create_and_save_line(text, read_string))
					return ALLOCATION_ERROR;
				buffers_used = 0;
				if (feof(file))
					break;
				continue;
			}
			buffers_used++;
			read_result = (char*)realloc(read_string, sizeof(char) * INPUT_BUFFER_SIZE * buffers_used);
			if (read_result == NULL)
			{
				free(read_string);
				return ALLOCATION_ERROR;
			}
			read_string = read_result;
			read_result = fgets(read_string + (INPUT_BUFFER_SIZE * (buffers_used - 1)) * sizeof(char) - 1, INPUT_BUFFER_SIZE + 1, file);
			if (read_result == NULL)
			{
				if (ferror(file))
					return READ_FROM_FILE_ERROR;
				if (feof(file))
				{
					if (read_string[0] != '\0')
					{
						if (create_and_save_line(text, read_string))
							return ALLOCATION_ERROR;
					}
					else
						free(read_string);
					break;
				}
			}
		}
	}
	fclose(file);
	text->save_status = SAVED;
	return NO_ERROR;
}

errors_global create_and_save_line(Text *text, char *content)
{
	Line *line = (Line*)malloc(sizeof(Line));
	if (line == NULL)
		return ALLOCATION_ERROR;
	line->content = content;
	line->next_line = NULL;
	insert_line_to_last_position(text, line);
	return NO_ERROR;
}

errors_file_IO save_text_to_file(Text *text, char const *path)
{
	FILE *file = fopen(path, "w+");
	Line *current_line = text->first_line;

	if (file == NULL)
		return FAILED_TO_OPEN_FILE;
	while (current_line != NULL)
	{
		fwrite(current_line->content, sizeof(char), strlen(current_line->content), file);
		current_line = current_line->next_line;
	}
	if (ferror(file))
	{
		fclose(file);
		return WRITE_TO_FILE_ERROR;
	}
	fclose(file);
	return NO_ERROR;
}

errors_file_IO get_command_text(char **command)
{
	unsigned int buffers_used = 0;
	char *read_string;
	*command = (char*)malloc(sizeof(char) * INPUT_BUFFER_SIZE);
	if (*command == NULL)
		return ALLOCATION_ERROR;
	if (fgets(*command, INPUT_BUFFER_SIZE, stdin) == NULL)
		if (feof(stdin))
		{
		 /* checks if tried to read from already closed stream */
			free(*command);
			return END_OF_FILE_ALREADY_REACHED;
		}
	if (ferror(stdin))
		return FAILED_TO_READ_COMMAND;
	buffers_used++;
	while (strlen(*command) == INPUT_BUFFER_SIZE * buffers_used - 1 && (*command)[INPUT_BUFFER_SIZE * buffers_used - 1] != '\n')
	{
		buffers_used++;
		read_string = (char*)realloc(*command, sizeof(char) * INPUT_BUFFER_SIZE * buffers_used);
		if (read_string == NULL)
		{
			free(*command);
			return ALLOCATION_ERROR;
		}
		*command = read_string;
		fgets(read_string + (INPUT_BUFFER_SIZE * (buffers_used - 1)) * sizeof(char) - 1, INPUT_BUFFER_SIZE + 1, stdin);
		if (ferror(stdin))
			return FAILED_TO_READ_COMMAND;
	}
	return NO_ERROR;
}

errors_file_IO print_text(Text const *text, run_mode mode, const unsigned int screen_width)
{
	int i;
	Line *current_line;

	iterate_line_pointer(text, &current_line, (int)text->show_range_start);
	for (i = text->show_range_start; i <= text->show_range_end; i++)
	{
		if (mode == INTERACTIVE || mode == OUTPUT_ONLY)
		{
			print_line_with_interface(text, current_line, i, screen_width, NULL);
			if (current_line->next_line == NULL && text->shift < strlen(current_line->content))
				putchar('\n');
		}
		else
			printf("%s", current_line->content);
		current_line = current_line->next_line;
	}
	if (current_line == NULL && mode != INTERACTIVE && mode != OUTPUT_ONLY)
		putchar('\n');
	return ferror(stdout) ? FAILED_TO_WRITE_IN_OUTPUT_STREAM : NO_ERROR;
}

errors_global print_line_with_interface(Text const *text, Line *line, int line_number, const unsigned int screen_width, unsigned int *truncates_counter)
{
	set_colors(BLACK, text->color_scheme.left_column);
	printf("%*s", DEFAULT_INTERFACE_SIZE - 2, " ");
	set_colors(text->color_scheme.line_numbers, text->color_scheme.background);
	printf(" %*d ", text->number_indent, line_number);
	set_colors(text->color_scheme.text, text->color_scheme.background);
	if (text->wrap == TRUNCATE)
		print_line_truncated(line->content, text->tab_width, text->interface_indent, screen_width - text->interface_indent, truncates_counter);
	else
		print_line_continued(line->content, text->tab_width, text->shift, screen_width - text->interface_indent);
	reset_colors();
	return ferror(stdout) ? FAILED_TO_WRITE_IN_OUTPUT_STREAM : NO_ERROR;
}

errors_global print_line_truncated(char const *line, const unsigned int tab_width, const unsigned int indent, const unsigned int print_width, unsigned int *truncates_counter)
{
	int i, left_width = (int)print_width, line_length = strlen(line);

	for (i = 0; i < line_length; i++, left_width--)
	{
		if (left_width <= 0)
		{
			if (truncates_counter != NULL)
				(*truncates_counter)++;
			left_width = (int)print_width;
			printf("\n%*s", indent, " ");
		}
		if (line[i] != '\t')
			putchar(line[i]);
		else
		{
			if (tab_width)
			{
				printf("%*s", (left_width - tab_width <= 0) ? left_width : tab_width, " ");
				left_width -= tab_width - 1;
			}
			else
				left_width++;
		}
	}
	return ferror(stdout) ? FAILED_TO_WRITE_IN_OUTPUT_STREAM : NO_ERROR;
}

errors_global print_line_continued(char const *line, const unsigned int tab_width, const unsigned int shift, int print_width)
{
	int i, line_length = strlen(line);
	for (i = 0; print_width > 0 && i + shift < line_length; i++, print_width--)
		if (line[i + shift] != '\t')
			putchar(line[i + shift]);
		else
		{
			if (tab_width)
			{
				printf("%*s", (print_width - (int)tab_width < 0) ? print_width : tab_width, " ");
				print_width -= tab_width - 1;
			}
			else
				print_width++;
		}
	if (shift >= line_length)
		putchar('\n');
	else
		if (i + shift < line_length)
				putchar('\n');
	return ferror(stdout) ? FAILED_TO_WRITE_IN_OUTPUT_STREAM : NO_ERROR;
}

errors_global print_text_in_interactive_mode(Text *text, const unsigned int screen_width, const unsigned int screen_height)
{
	unsigned int actual_screen_height = screen_height - 1;
	int character;

	printf("\033[?25l\033[;H\033[J");
	if (change_terminal_input_mode_to(CHARACTER_BY_CHARACTER))
		return FAILED_TO_SET_TERMINAL_ATTRIBUTES;
	text->show_range_start = 1;
	text->show_range_end = (actual_screen_height < text->size) ? actual_screen_height : text->size;
	print_text_screen(text, screen_width);
	while ((character = getchar()) != EOF && character != 'q' && character != 'Q')
	{
		switch (character)
		{
		case ' ':
		{
			if (text->show_range_end < text->size)
			{
				printf("\033[;H\033[J");
				text->show_range_start = text->show_range_end;
				text->show_range_end = (text->show_range_end + actual_screen_height < text->size) ? text->show_range_end + actual_screen_height - 1: text->size;
				print_text_screen(text, screen_width);
			}
			else
			{
				if (actual_screen_height < text->size)
				{
					printf("\033[;H\033[J");
					text->show_range_start = 1;
					text->show_range_end = (actual_screen_height < text->size) ? actual_screen_height : text->size;
					print_text_screen(text, screen_width);
				}
			}
			break;
		}
		case '\n':
		{
			if (text->show_range_start > 1)
			{
				printf("\033[;H\033[J");
				text->show_range_end = text->show_range_start;
				text->show_range_start = (text->show_range_start > actual_screen_height) ? text->show_range_start - actual_screen_height + 1: 1;
				print_text_screen(text, screen_width);
			}
			else
			{
				if (actual_screen_height < text->size)
				{
					printf("\033[;H\033[J");
					text->show_range_start = text->size - actual_screen_height + 2;
					text->show_range_end = text->size;
					print_text_screen(text, screen_width);
				}
			}
			break;
		}
		case ',':
		case '<':
		{
			if (text->wrap == CONTINUE)
			{
				printf("\033[;H\033[J");
				text->shift = (text->shift > 0) ? text->shift - 1 : 0;
				print_text_screen(text, screen_width);
			}
			break;
		}
		case '.':
		case '>':
		{
			if (text->wrap == CONTINUE)
			{
				printf("\033[;H\033[J");
				text->shift++;
				print_text_screen(text, screen_width);
			}
			break;
		}
		default:
			break;
		}
	}
	if (change_terminal_input_mode_to(DEFAULT))
		return FAILED_TO_SET_TERMINAL_ATTRIBUTES;
	printf("\033[;H\033[J\033[?25h");
	return ferror(stdout) ? FAILED_TO_WRITE_IN_OUTPUT_STREAM : NO_ERROR;
}

errors_global print_text_screen(Text *text, const unsigned int screen_width)
{
	int i;
	unsigned int truncates_counter;
	Line *current_line;

	iterate_line_pointer(text, &current_line, (int)text->show_range_start);
	for (i = text->show_range_start, truncates_counter = 0; i <= text->show_range_end; i++, text->show_range_end -= truncates_counter, truncates_counter = 0)
	{
		print_line_with_interface(text, current_line, i, screen_width, &truncates_counter);
		if (current_line->next_line == NULL && text->shift < strlen(current_line->content))
			putchar('\n');
		current_line = current_line->next_line;
	}
	printf_colored_attributed("[space] [enter] [</>] [q] ", YELLOW, BLACK, 1, BOLD);
	return ferror(stdout) ? FAILED_TO_WRITE_IN_OUTPUT_STREAM : NO_ERROR;
}