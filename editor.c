#include <unistd.h>
#include <locale.h>

#include <stdlib.h>

#include "interface_controller.h"

int main(int argc, char **argv)
{
	char *username = "user", *command;
	unsigned short screen_width, screen_height, indent, vertical_indent, possible_error;
	run_mode mode;
	Text *text;
	command_codes command_code = EMPTY_COMMAND;
	command_arguments *received_arguments;

	if (argc > 2)
	{
		printf_colored_attributed("T.ED! requires only one argument - file name to open/create \n", RED, BLACK, 2, ITALIC, BOLD);
		return INVALID_NUMBER_OF_COMMAND_LINE_ARGUMENTS;
	}

	setlocale(LC_CTYPE, "");
	/* for correct Unicode characters showing */

	if (initialize_empty_text(&text))
		return ALLOCATION_ERROR;
	if (initialize_empty_arguments_structure(&received_arguments))
		return ALLOCATION_ERROR;
	if (detect_environment(&username, &mode))
		return FAILED_TO_GET_TERMINAL_ATTRIBUTES;
	if (mode != AUTONOMOUS)
	{
		if ((possible_error = get_terminal_size(&screen_width, &screen_height)))
			return possible_error;
		calculate_indents(&indent, &vertical_indent, screen_width, screen_height);
	}
	if (mode == INTERACTIVE)
		show_loading_screen(indent, vertical_indent);
	if (argc == 2)
	{
		if ((possible_error = load_text_from_file(text, argv[1])))
		{
			if (possible_error == FAILED_TO_OPEN_FILE)
				show_command_error_message(TRYING_TO_OPEN_NONEXISTENT_FILE_OR_OPEN_ERROR, mode);
			else
				return possible_error;
		}
		else
		{
			text->show_range_end = text->size;
			update_indents(text);
			if (mode == INTERACTIVE || mode == OUTPUT_ONLY)
				print_text_in_interactive_mode(text, screen_width, screen_height);
		}
	}
	while (command_code != EXIT)
	{
		if (mode == INTERACTIVE)
			if (show_input_invite(username, text->color_scheme.left_column))
				return FAILED_TO_WRITE_IN_OUTPUT_STREAM;
		if ((possible_error = get_command_text(&command)))
		{
			if (possible_error == END_OF_FILE_ALREADY_REACHED)
				break;
			return possible_error;
		}
		if ((possible_error = decode_command(command, &command, &command_code, received_arguments)))
		{
			free(command);
			free_received_arguments(received_arguments);
			if ((possible_error = show_command_error_message(possible_error, mode)))
			 /* handling error of writing to stdout while showing error message */
				return possible_error;
		}
		else
		{
			if ((possible_error = check_command_arguments(command_code, received_arguments)))
			{
				free_received_arguments(received_arguments);
				if ((possible_error = show_command_error_message(possible_error, mode)))
					return possible_error;
			}
			else if ((possible_error = execute_command(command_code, received_arguments, text, &text, mode, screen_width, screen_height)))
			{

				free_received_arguments(received_arguments);
				if (possible_error == TRYING_TO_EXIT_WITHOUT_SAVE)
					command_code = EMPTY_COMMAND;
				if ((possible_error = show_command_error_message(possible_error, mode)))
					return possible_error;
			}
		}
		clear_received_arguments(received_arguments);
	}
	deinitialize_text(text);
	deinitialize_arguments_structure(received_arguments);
	return NO_ERROR;
}