#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

#include "interface_controller.h"

const char *command_keywords[COMMANDS_AMOUNT][COMMAND_MAX_SIZE] =
{
	{ "set", "tabwidth" },
	{ "set", "wrap" },
	{ "set", "name" },
	{ "print", "all" },
	{ "print", "pages" },
	{ "print", "range" },
	{ "print", "line" },
	{ "edit", "string" },
	{ "insert", "after" },
	{ "insert", "symbol" },
	{ "replace", "substring" },
	{ "delete", "line" },
	{ "delete", "range" },
	{ "delete", "braces" },
	{ "read", "" },
	{ "open", "" },
	{ "write", "" },
	{ "help", "" },
	{ "exit", "" }
};

const char *command_arguments_keywords[COMMANDS_ARGUMENTS_KEYWORDS_AMOUNT] =
{
	"yes",
	"no",
	"force"
};

const char *command_error_messages[COMMAND_ERROR_MESSAGES_AMOUNT] =
{
	"command not found",
	"invalid number",
	"more than possible amount of number arguments",
	"more than possible amount of string arguments",
	"incorrect quotes placement",
	"required extra number argument",
	"required extra string argument",
	"extra number argument was used",
	"extra string argument was used",
	"incorrect arguments order",
	"current file is empty",
	"invalid command argument",
	"invalid control character",
	"argument instead of string was used",
	"string instead of symbol was used",
	"invalid range",
	"invalid range for delete",
	"invalid line number",
	"invalid position in line",
	"current file is unsaved, save it or use \"force\" to discard changes",
	"selected file is missing or can't be opened",
	"no associated path to file",
	"help file is missing or can't be opened",
	"current file is unsaved, save it or use \"force\" to discard changes"
};

errors_global initialize_empty_arguments_structure(command_arguments **arguments)
{
	*arguments = (command_arguments*)malloc(sizeof(command_arguments));
	if (*arguments == NULL)
		return ALLOCATION_ERROR;
	clear_received_arguments(*arguments);
	return NO_ERROR;
}

void deinitialize_arguments_structure(command_arguments *arguments)
{
	free_received_arguments(arguments);
	free(arguments);
}

void clear_received_arguments(command_arguments *arguments)
{
	arguments->first_string = arguments->second_string = NULL;
	arguments->received_numbers = arguments->received_strings = 0;
	arguments->order = XX;
}

void free_received_arguments(command_arguments *arguments)
{
	if (arguments->received_strings >= 1)
		if (arguments->first_string != NULL)
			free(arguments->first_string);
	if (arguments->received_strings == 2)
		if (arguments->second_string != NULL)
			free(arguments->second_string);
}

void update_arguments_order(argument_receiving_order *order, argument_receiving_order argument_type)
{
	if (*order == IX)
		*order = (argument_type == INT) ? II : IS;
	if (*order == SX)
		*order = (argument_type == INT) ? SI : SS;
	if (*order == XX)
		*order = (argument_type == INT) ? IX : SX;
}

void update_arguments_amount(unsigned short *amount_of_detected_arguments, command_arguments *arguments, argument_receiving_order argument_type, ...)
{
	/* some kind of implementing overloaded function */
	va_list argument_list;
	va_start(argument_list, argument_type);
	if (!(*amount_of_detected_arguments))
	{
		if (argument_type == INT)
			arguments->first_number = va_arg(argument_list, unsigned int);
		else
			arguments->first_string = va_arg(argument_list, char*);
	}
	if (*amount_of_detected_arguments == 1)
	{
		if (argument_type == INT)
			arguments->second_number = va_arg(argument_list, unsigned int);
		else
			arguments->second_string = va_arg(argument_list, char*);
	}
	(*amount_of_detected_arguments)++;
	va_end(argument_list);
}

command_codes identify_keywords(const char *first_keyword, const char *second_keyword)
{
	int i;
	for (i = 0; i < COMMANDS_AMOUNT; i++)
		if (!strcmp(first_keyword, command_keywords[i][0]) && !strcmp(second_keyword, command_keywords[i][1]))
			return i;
	return INVALID_COMMAND;
}

errors_commands show_input_invite(char *username, color foreground)
{
	printf_colored(username, foreground, BLACK);
	putchar('@');
	printf_colored_attributed("editor: ", GREEN, BLACK, 1, ITALIC);
	return ferror(stdout) ? FAILED_TO_WRITE_IN_OUTPUT_STREAM : NO_ERROR;
}

errors_commands show_command_error_message(const unsigned short error_code, run_mode mode)
{
	if (error_code < COMMAND_ERROR_MESSAGES_AMOUNT + COMMANDS_ERRORS_GLOBAL_SHIFT &&
		error_code >= COMMANDS_ERRORS_GLOBAL_SHIFT)
	{
		if (mode == INTERACTIVE || mode == OUTPUT_ONLY)
		{
			printf_colored_attributed("editor: ", RED, BLACK, 2, ITALIC, BOLD);
			printf_colored_attributed(command_error_messages[error_code - COMMANDS_ERRORS_GLOBAL_SHIFT], RED, BLACK, 2, ITALIC, BOLD);
			printf_colored_attributed(" \n", RED, BLACK, 2, ITALIC, BOLD);
		}
		else
			printf("editor: %s \n", command_error_messages[error_code - COMMANDS_ERRORS_GLOBAL_SHIFT]);
		return ferror(stdout) ? FAILED_TO_WRITE_IN_OUTPUT_STREAM : NO_ERROR;
	}
	else
		return error_code;
	return NO_ERROR;
}

errors_commands decode_command(char *command, char **command_address, command_codes *command_code, command_arguments *arguments)
{
	char *string_argument, *first_keyword, *second_keyword, *buffer;
	unsigned short detected_numbers = 0, detected_strings = 0;
	unsigned int i, number_argument, start_position, command_length = strlen(command);
	decode_status status = NOTHING_READ;
	arguments->order = XX;

	for (i = 0; i < command_length; i++)
	{	FOR_START:
		if (status == NOTHING_READ || status == GOT_FIRST_KEYWORD || status == READING_ARGUMENTS)
		{
			if (command[i] == '#')
				break;
			if (command[i] == ' ' || command[i] == '\t' || command[i] == '\n')
				continue;
		}
		switch (status)
		{
		case NOTHING_READ:
		{
			start_position = i;
			status = READING_FIRST_KEYWORD;
			break;
		}
		case READING_FIRST_KEYWORD:
		{
			if (command[i] == ' ' || command[i] == '\t' || command[i] == '\n')
			{
				first_keyword = (char*)calloc(i - start_position + 1, sizeof(char)); /* +1 for \0 */
				if (first_keyword == NULL)
					return ALLOCATION_ERROR;
				strncpy(first_keyword, command + sizeof(char) * start_position, sizeof(char) * (i - start_position));
				*command_code = identify_keywords(first_keyword, "");
				if (*command_code != INVALID_COMMAND)
				{	
					free(first_keyword);
					status = READING_ARGUMENTS;
					continue;
				}
				status = GOT_FIRST_KEYWORD;
			}
			break;
		}
		case GOT_FIRST_KEYWORD:
		{
			start_position = i;
			status = READING_SECOND_KEYWORD;
			break;
		}
		case READING_SECOND_KEYWORD:
		{
			if (command[i] == ' ' || command[i] == '\t' || command[i] == '\n')
			{
				second_keyword = (char*)calloc(i - start_position + 1, sizeof(char));
				if (second_keyword == NULL)
					return ALLOCATION_ERROR;
				strncpy(second_keyword, command + sizeof(char) * start_position, sizeof(char) * (i - start_position));
				*command_code = identify_keywords(first_keyword, second_keyword);
				free(first_keyword);
				free(second_keyword);
				if (*command_code != INVALID_COMMAND)
					status = READING_ARGUMENTS;
				else
					return UNRECOGNIZED_COMMAND;
			}
			break;
		}
		case READING_ARGUMENTS:
		{
			start_position = i;
			if (isdigit(command[i]))
				status = READING_NUMBER;
			else
				if (command[i] == '\"')
				{
					if (i + 1 < command_length)
					{
						if (command[i + 1] != '\"')
						{
							status = READING_SINGLE_STRING_ARGUMENT;
						}
						else
						{
							if (i + 2 < command_length)
							{
								if (command[i + 2] == '\"')
								{
									status = READING_MULTIPLE_STRINGS_ARGUMENT;
									i += 2;
									continue;
								}
								else
								{
									if (command[i + 2] != ' ' && command[i + 2] != '\t' && command[i + 2] != '\n')
										return INCORRECT_QUOTES_FORMATTING;
									else
										status = READING_SINGLE_STRING_ARGUMENT;
								}
							}
						}
					}
				}
				else
					status = READING_STRING_TERM;
			break;
		}
		case READING_NUMBER:
		{
			if (command[i] == ' ' || command[i] == '\t' || command[i] == '\n')
			{
				string_argument = (char*)calloc(i - start_position + 1, sizeof(char));
				if (string_argument == NULL)
					return ALLOCATION_ERROR;
				strncpy(string_argument, command + sizeof(char) * start_position, sizeof(char) * (i - start_position));
				if (strlen(string_argument) == 1 && detected_numbers == 2 && !detected_strings &&
					(*command_code == INSERT_SYMBOL || *command_code == EDIT_STRING))
				{
					detected_strings++;
					arguments->received_strings++;
					arguments->first_string = string_argument;
					status = READING_ARGUMENTS;
					break;
				}
				if (detected_numbers == 2)
				{
					free(string_argument);
					return TOO_MANY_NUMBER_ARGUMENTS;
				}
				number_argument = atoi(string_argument);
				free(string_argument);
				update_arguments_amount(&detected_numbers, arguments, INT, number_argument);
				update_arguments_order(&(arguments->order), INT);
				status = READING_ARGUMENTS;
				break;
			}
			if (!isdigit(command[i]))
				return INVALID_NUMBER_ARGUMENT;
			break;
		}
		case READING_STRING_TERM:
		{
			if (command[i] == ' ' || command[i] == '\t' || command[i] == '\n')
			{
				if (detected_strings == 2)
					return TOO_MANY_STRING_ARGUMENTS;
				string_argument = (char*)calloc(i - start_position + 1, sizeof(char));
				if (string_argument == NULL)
					return ALLOCATION_ERROR;
				strncpy(string_argument, command + sizeof(char) * start_position, sizeof(char) * (i - start_position));
				update_arguments_amount(&detected_strings, arguments, STRING, string_argument);
				update_arguments_order(&(arguments->order), STRING);
				status = READING_ARGUMENTS;
			}
			break;
		}
		case READING_SINGLE_STRING_ARGUMENT:
		{
			if (command[i] == '\"')
			{
				if (i - 1 >= 0)
					if (command[i - 1] == '\\')
						if (i + 1 < command_length)
							if (command[i + 1] != ' ' && command[i + 1] != '\t' && command[i + 1] != '\n')
								break;
				if (i + 1 < command_length)
					if (command[i + 1] != ' ' && command[i + 1] != '\t' && command[i + 1] != '\n')
						return INCORRECT_QUOTES_FORMATTING;
				if (detected_strings == 2)
					return TOO_MANY_STRING_ARGUMENTS;
				string_argument = (char*)calloc(i - start_position + 2, sizeof(char));
				if (string_argument == NULL)
					return ALLOCATION_ERROR;
				strncpy(string_argument, command + sizeof(char) * start_position, sizeof(char) * (i - start_position + 1));
				update_arguments_amount(&detected_strings, arguments, STRING, string_argument);
				update_arguments_order(&(arguments->order), STRING);
				status = READING_ARGUMENTS;
			}
			break;
		}
		case READING_MULTIPLE_STRINGS_ARGUMENT:
		{
			if (command[i] == '\"')
			{
				if (i - 1 >= 0)
					if (command[i - 1] == '\\')
						if (i + 1 < command_length)
							if (command[i + 1] != '\"')
								break;
				if (i + 1 < command_length)
				{
					if (command[i + 1] != '\"')
						return INCORRECT_QUOTES_FORMATTING;
					if (i + 2 < command_length)
					{
						if (command[i + 2] != '\"')
							return INCORRECT_QUOTES_FORMATTING;
						if (i + 3 < command_length)
							if (command[i + 3] != ' ' && command[i + 3] != '\t' && command[i + 3] != '\n')
								return INCORRECT_QUOTES_FORMATTING;
					}
				}
				else
					return INCORRECT_QUOTES_FORMATTING;
				if (detected_strings == 2)
					return TOO_MANY_STRING_ARGUMENTS;
				string_argument = (char*)calloc(i - start_position + 4, sizeof(char)); /* +4 to save """\0 */
				if (string_argument == NULL)
					return ALLOCATION_ERROR;
				strncpy(string_argument, command + sizeof(char) * start_position, sizeof(char) * (i - start_position + 3));
				update_arguments_amount(&detected_strings, arguments, STRING, string_argument);
				update_arguments_order(&(arguments->order), STRING);
				status = READING_ARGUMENTS;
				i += 3; /* to skip """X */
			}
			break;
		}
		} /* end of switch */
	}	  /* end of for	   */

	if (status == READING_MULTIPLE_STRINGS_ARGUMENT ||
		(*command_code == INSERT_AFTER && !detected_strings) ||
		(*command_code == REPLACE_SUBSTRING && detected_strings != 2))
	{
	/*	Mechanic of receiving multiply strings: for-loop is 
		restarted similar like it gets a new command string
		(which is an concatenated old one indeed) but keeps
		all variables (i, status, detected_strings, etc.
		(besides command_length)) with old value			*/

		if (*command_code == INSERT_AFTER && status == READING_SINGLE_STRING_ARGUMENT)
			return INCORRECT_QUOTES_FORMATTING;
		if (*command_code == INSERT_AFTER && !detected_strings && status != READING_MULTIPLE_STRINGS_ARGUMENT)
			printf_colored_attributed("editor: no input string argument for insert command \n", YELLOW, BLACK, 2, ITALIC, BOLD);
		if (*command_code == REPLACE_SUBSTRING && detected_strings != 2 && status != READING_MULTIPLE_STRINGS_ARGUMENT)
		{
			if (!detected_strings)
				printf_colored_attributed("editor: missing substring for replacement \n", YELLOW, BLACK, 2, ITALIC, BOLD);
			else
				printf_colored_attributed("editor: missing replacement pattern \n", YELLOW, BLACK, 2, ITALIC, BOLD);
		}
		if ((i = get_command_text(&string_argument)))
		/* using i here as error indicator,
		   using string_argument here just
		   as buffer for next received line */
		{
			if (i == END_OF_FILE_ALREADY_REACHED)
			{
				if (status == READING_MULTIPLE_STRINGS_ARGUMENT)
					return INCORRECT_QUOTES_FORMATTING;
			}
			else
				return i;
		}
		i = command_length;
		buffer = (char*)realloc(command, sizeof(char) * (command_length + strlen(string_argument) + 1 /* for '\0' */));
		if (buffer == NULL)
		{
			free(string_argument);
			free(command);
			return ALLOCATION_ERROR;
		}
		*command_address = buffer; 
	 /* required to save changes
		in main() to successfully
		free when error ocurred */
		command = buffer;
		strcat(command, string_argument);
		command_length = strlen(command);
		free(string_argument);
		goto FOR_START;
	 /* goto is used for optimization: previous for-loop 
		is needed to be restarted but after it's finished.
		Another way is to check for i == command_length - 1
		but it's required on every iteration so in this way
		goto helps to omit both unnecessary checks and
		code duplication									*/
	}

	switch (status)
	{
	case NOTHING_READ:
	{
		*command_code = EMPTY_COMMAND;
		break;
	}
	case READING_FIRST_KEYWORD:
	{
		first_keyword = (char*)calloc(i - start_position + 1, sizeof(char));
		if (first_keyword == NULL)
			return ALLOCATION_ERROR;
		strncpy(first_keyword, command + sizeof(char) * start_position, sizeof(char) * (i - start_position));
		*command_code = identify_keywords(first_keyword, "");
		free(first_keyword);
		if (*command_code == INVALID_COMMAND)
			return UNRECOGNIZED_COMMAND;
		break;
	}
	case GOT_FIRST_KEYWORD:
	{
		free(first_keyword);
		return UNRECOGNIZED_COMMAND;
	}
	case READING_SECOND_KEYWORD:
	{
		second_keyword = (char*)calloc(i - start_position + 1, sizeof(char));
		if (second_keyword == NULL)
			return ALLOCATION_ERROR;
		strncpy(second_keyword, command + sizeof(char) * start_position, sizeof(char) * (i - start_position));
		*command_code = identify_keywords(first_keyword, second_keyword);
		free(first_keyword);
		free(second_keyword);
		if (*command_code == INVALID_COMMAND)
			return UNRECOGNIZED_COMMAND;
		break;
	}
	case READING_NUMBER:
	{
		string_argument = (char*)calloc(i - start_position + 1, sizeof(char));
		if (string_argument == NULL)
			return ALLOCATION_ERROR;
		strncpy(string_argument, command + sizeof(char) * start_position, sizeof(char) * (i - start_position));
		if (strlen(string_argument) == 1 && detected_numbers == 2 && !detected_strings &&
		   (*command_code == INSERT_SYMBOL || *command_code == EDIT_STRING))
		{
			detected_strings++;
			arguments->received_strings++;
			arguments->first_string = string_argument;
			break;
		}
		if (detected_numbers == 2)
		{
			free(string_argument);
			return TOO_MANY_NUMBER_ARGUMENTS;
		}
		number_argument = atoi(string_argument);
		free(string_argument);
		update_arguments_amount(&detected_numbers, arguments, INT, number_argument);
		update_arguments_order(&(arguments->order), INT);
		break;
	}
	case READING_STRING_TERM:
	{
		if (detected_strings == 2)
			return TOO_MANY_STRING_ARGUMENTS;
		string_argument = (char*)calloc(i - start_position + 1, sizeof(char));
		if (string_argument == NULL)
			return ALLOCATION_ERROR;
		strncpy(string_argument, command + sizeof(char) * start_position, sizeof(char) * (i - start_position));
		update_arguments_amount(&detected_strings, arguments, STRING, string_argument);
		update_arguments_order(&(arguments->order), STRING);
		break;
	}
	case READING_SINGLE_STRING_ARGUMENT:
	{
		return INCORRECT_QUOTES_FORMATTING;
	}
	default:
/* READING_ARGUMENTS only,
	READING_MULTIPLE_STRINGS_ARGUMENT status will
	be never received, it's handled before switch */
		break;
	} /* end of switch */
	arguments->received_numbers = detected_numbers;
	arguments->received_strings = detected_strings;
	free(command);
	return NO_ERROR;
}

errors_commands check_command_arguments(command_codes command_code, command_arguments const *arguments)
{
 /* Checks only amount and order of string/integer arguments, not their value,
	value is checked in execute function (requirements are described in .h)	  */

	switch (command_code)
	{
	case SET_TAB_WIDTH:
	case PRINT_LINE:
	case DELETE_LINE:
	{
		if (!arguments->received_numbers)
			return MISSING_NUMBER_ARGUMENT;
		if (arguments->received_numbers > 1)
			return EXTRA_NUMBER_ARGUMENT;
		if (arguments->received_strings)
			return EXTRA_STRING_ARGUMENT;
		break;
	}
	case SET_WRAP:
	{
		if (!arguments->received_strings)
			return MISSING_STRING_ARGUMENT;
		if (arguments->received_strings > 1)
			return EXTRA_STRING_ARGUMENT;
		if (arguments->received_numbers)
			return EXTRA_NUMBER_ARGUMENT;
		break;
	}
	case READ:
	case OPEN:
	{
		if (!arguments->received_strings)
			return MISSING_STRING_ARGUMENT;
		if (arguments->received_numbers)
			return EXTRA_NUMBER_ARGUMENT;
		break;
	}
	case SET_NAME:
	case WRITE:
	case EXIT:
	{
		if (arguments->received_strings > 1)
			return EXTRA_STRING_ARGUMENT;
		if (arguments->received_numbers)
			return EXTRA_NUMBER_ARGUMENT;
		break;
	}
	case PRINT_ALL:
	case PRINT_PAGES:
	case HELP:
	{
		if (arguments->received_numbers)
			return EXTRA_NUMBER_ARGUMENT;
		if (arguments->received_strings)
			return EXTRA_STRING_ARGUMENT;
		break;
	}
	case PRINT_RANGE:
	case DELETE_BRACES:
	{
		if (arguments->received_strings)
			return EXTRA_STRING_ARGUMENT;
		break;
	}
	case EDIT_STRING:
	case INSERT_SYMBOL:
	{
		if (arguments->received_numbers != 2)
			return MISSING_NUMBER_ARGUMENT;
		if (!arguments->received_strings)
			return MISSING_STRING_ARGUMENT;
		if (arguments->received_strings > 1)
			return EXTRA_STRING_ARGUMENT;
		if (arguments->order != II)
			return INCORRECT_ARGUMENTS_ORDER;
		break;
	}
	case INSERT_AFTER:
	{
		if (arguments->received_numbers > 1)
			return EXTRA_NUMBER_ARGUMENT;
		if (!arguments->received_strings)
			return MISSING_STRING_ARGUMENT;
		if (arguments->received_strings > 1)
			return EXTRA_STRING_ARGUMENT;
		if (arguments->order != IS && arguments->order != SX)
			return INCORRECT_ARGUMENTS_ORDER;
		break;
	}
	case REPLACE_SUBSTRING:
	{
		if (arguments->received_strings != 2)
			return MISSING_STRING_ARGUMENT;
		if (arguments->order != II && arguments->order != IS && arguments->order != SS)
			return INCORRECT_ARGUMENTS_ORDER;
		break;
	}
	case DELETE_RANGE:
	{
		if (!arguments->received_numbers)
			return MISSING_NUMBER_ARGUMENT;
		if (arguments->received_strings)
			return EXTRA_STRING_ARGUMENT;
		break;
	}
	default:
		break;
	}
	return NO_ERROR;
}

errors_commands execute_command(command_codes command_code, command_arguments *arguments,
								Text *text, Text **text_address, run_mode mode,
								const unsigned int screen_width, const unsigned int screen_height)
{
	char *buffer = NULL;
	unsigned short possible_error;
	Text *help;
	Line *block_of_lines;

	switch (command_code)
	{
	case PRINT_ALL:
	case PRINT_PAGES:
	case PRINT_RANGE:
	case PRINT_LINE:
	case EDIT_STRING:
	case INSERT_SYMBOL:
	case REPLACE_SUBSTRING:
	case DELETE_LINE:
	case DELETE_RANGE:
	case DELETE_BRACES:
	{
		if (!text->size)
			return TRY_TO_EXECUTE_OPERATION_ON_EMPTY_TEXT;
	}
	default:
		break;
	}

	switch (command_code)
	{
	case SET_TAB_WIDTH:
	{
		text->tab_width = arguments->first_number;
		if (arguments->first_number > screen_width / 4)
			printf_colored_attributed("editor: setted tab width is big enough to deform showed text a lot \n", YELLOW, BLACK, 2, ITALIC, BOLD);
		break;
	}
	case SET_WRAP:
	{
		if (!(!strcmp(arguments->first_string, command_arguments_keywords[YES]) + !strcmp(arguments->first_string, command_arguments_keywords[NO])))
			return INCORRECT_ARGUMENT_TERM_WAS_USED;
		text->wrap = (!strcmp(arguments->first_string, command_arguments_keywords[YES])) ? CONTINUE : TRUNCATE;
		break;
	}
	case SET_NAME:
	{
		if (arguments->received_strings)
			if (convert_string_argument(&(arguments->first_string)))
				return ALLOCATION_ERROR;
		if (arguments->received_strings && strlen(arguments->first_string))
		{
			if (text->file_association != NULL)
			{
				if (!strcmp(arguments->first_string, text->file_association))
					text->save_status = UNSAVED;
				free(text->file_association);
			}
			else
				text->save_status = UNSAVED;
			text->file_association = arguments->first_string;
			arguments->received_strings--; /* to not free file_association after switch */
		}
		else
		{
			if (text->file_association != NULL)
				free(text->file_association);
			text->file_association = NULL;
			text->save_status = UNSAVED;
		}
		break;
	}
	case PRINT_ALL:
	{
		text->show_range_start = 1;
		text->show_range_end = text->size;
		print_text(text, mode, screen_width);
		break;
	}
	case PRINT_PAGES:
	{
		if (mode == INTERACTIVE || mode == OUTPUT_ONLY)
			print_text_in_interactive_mode(text, screen_width, screen_height);
		else
		{
			text->show_range_start = 1;
			text->show_range_end = text->size;
			print_text(text, mode, screen_width);
		}
		break;
	}
	case PRINT_RANGE:
	{
		if (!arguments->received_numbers)
		{
			text->show_range_start = 1;
			text->show_range_end = text->size;
		}
		if (arguments->received_numbers == 1)
		{
			if (!arguments->first_number || arguments->first_number > text->size)
				return INVALID_RANGE_WAS_USED;
			text->show_range_start = arguments->first_number;
			text->show_range_end = text->size;
		}
		if (arguments->received_numbers == 2)
		{
			if (!arguments->first_number || arguments->first_number > text->size ||
				 arguments->first_number > arguments->second_number || arguments->second_number > text->size)
				return INVALID_RANGE_WAS_USED;
			text->show_range_start = arguments->first_number;
			text->show_range_end = arguments->second_number;
		}
		print_text(text, mode, screen_width);
		break;
	}
	case PRINT_LINE:
	{
		if (!arguments->first_number || arguments->first_number > text->size)
			return INVALID_LINE_NUMBER_WAS_USED;
		text->show_range_start = text->show_range_end = arguments->first_number;
		print_text(text, mode, screen_width);
		break;
	}
	case EDIT_STRING:
	{
		if (strlen(arguments->first_string) == 2 && arguments->first_string[0] == '\\')
		{
			if (is_second_part_of_control_character(arguments->first_string[1]))
			{
				arguments->first_string[0] = get_control_character_by_its_second_part(arguments->first_string[1]);
				arguments->first_string[1] = '\0';
			}
			else
				return INCORRECT_CONTROL_CHARACTER_USED;
		}
		if (strlen(arguments->first_string) > 1)
			return STRING_INSTEAD_OF_SYMBOL_WAS_USED;
		if (!arguments->first_number || arguments->first_number > text->size)
			return INVALID_LINE_NUMBER_WAS_USED;
		if (edit_line(text, arguments->first_number, arguments->second_number, arguments->first_string[0]))
			return INVALID_POSITION_IN_LINE_WAS_USED;
		text->save_status = UNSAVED;
		break;
	}
	case INSERT_AFTER:
	{
		if (detect_string_type(arguments->first_string) == TERM_STRING)
			return TERM_INSTEAD_OF_STRING_WAS_USED;
		if (arguments->received_numbers)
			if (arguments->first_number > text->size)
				return INVALID_LINE_NUMBER_WAS_USED;
		if (!arguments->received_numbers)
			arguments->first_number = text->size;
		if (convert_string_argument(&(arguments->first_string)))
			return ALLOCATION_ERROR;
		if (!strlen(arguments->first_string))
		{
			printf_colored_attributed("editor: string for insert is an empty one \n", YELLOW, BLACK, 2, ITALIC, BOLD);
			break;
		}
		if (detect_number_of_strings_in_line(arguments->first_string) > 1)
		{
			if (transform_multiply_strings_in_lines(arguments->first_string, &block_of_lines))
				return ALLOCATION_ERROR;
		}
		else
		{
			block_of_lines = (Line*)malloc(sizeof(Line));
			if (block_of_lines == NULL)
				return ALLOCATION_ERROR;
			block_of_lines->content = arguments->first_string;
			block_of_lines->next_line = NULL;
			arguments->received_strings--; /* for proper free() after switch */
		}
		insert_block_of_lines_after_position(text, block_of_lines, arguments->first_number);
		normalize_lines_endings(text);
		update_indents(text);
		text->save_status = UNSAVED;
		break;
	}
	case INSERT_SYMBOL:
	{
		if (strlen(arguments->first_string) == 2 && arguments->first_string[0] == '\\')
		{
			if (is_second_part_of_control_character(arguments->first_string[1]))
			{
				arguments->first_string[0] = get_control_character_by_its_second_part(arguments->first_string[1]);
				arguments->first_string[1] = '\0';
			}
			else
				return INCORRECT_CONTROL_CHARACTER_USED;
		}
		if (strlen(arguments->first_string) > 1)
			return STRING_INSTEAD_OF_SYMBOL_WAS_USED;
		if (!arguments->first_number || arguments->first_number > text->size)
			return INVALID_LINE_NUMBER_WAS_USED;
		if (insert_symbol(text, arguments->first_number, arguments->second_number, arguments->first_string[0]))
			return ALLOCATION_ERROR;
		text->save_status = UNSAVED;
		break;
	}
	case REPLACE_SUBSTRING:
	{
		if (detect_string_type(arguments->second_string) == TERM_STRING)
			return TERM_INSTEAD_OF_STRING_WAS_USED;
		if (convert_string_argument(&(arguments->second_string)))
			return ALLOCATION_ERROR;
		if (!arguments->received_numbers)
		{
			arguments->first_number = 1;
			arguments->second_number = text->size;
		}
		if (arguments->received_numbers == 1)
		{
			if (!arguments->first_number || arguments->first_number > text->size)
				return INVALID_LINE_NUMBER_WAS_USED;
			arguments->second_number = text->size;
		}
		if (arguments->received_numbers == 2)
			if (!arguments->first_number || arguments->first_number > text->size ||
			 	 arguments->first_number > arguments->second_number || arguments->second_number > text->size)
				return INVALID_DELETE_RANGE_WAS_USED;
		if (detect_string_type(arguments->first_string) == TERM_STRING)
		{
			if (arguments->first_string[0] != '^' && arguments->first_string[0] != '$')
				return INCORRECT_ARGUMENT_TERM_WAS_USED;
			if (add_string_to_range((arguments->first_string[0] == '^') ? BEFORE_LINE : AFTER_LINE, text,
				arguments->second_string, arguments->first_number, arguments->second_number))
				return ALLOCATION_ERROR;
			expand_duplicated_lines(text);
			normalize_lines_endings(text);
			update_indents(text);
			text->save_status = UNSAVED;
		}
		else
		{
			if (convert_string_argument(&(arguments->first_string)))
				return ALLOCATION_ERROR;
			if (!strlen(arguments->first_string))
			{
				printf_colored_attributed("editor: substring for replace is an empty one \n", YELLOW, BLACK, 2, ITALIC, BOLD);
				break;
			}
			if ((possible_error = replace_substring_in_range(text, arguments->first_string, arguments->second_string, arguments->first_number, arguments->second_number)))
			{
				if (possible_error == ALLOCATION_ERROR)
					return ALLOCATION_ERROR;
				else
				{
					set_colors(YELLOW, BLACK);
					set_attributes(ITALIC, BOLD);
					if (possible_error - REPLACES_COUNT_DELTA)
					{
						printf("editor: amount of done replaces is %u \n", possible_error - REPLACES_COUNT_DELTA);
						expand_duplicated_lines(text);
						normalize_lines_endings(text);
						update_indents(text);
						text->save_status = UNSAVED;
					}
					else
						printf("editor: no substring occurrence found \n");
					reset_colors();

				}
			}
		}
		break;
	}
	case DELETE_LINE:
	{
		if (!arguments->first_number || arguments->first_number > text->size)
			return INVALID_LINE_NUMBER_WAS_USED;
		delete_line(text, arguments->first_number);
		normalize_text_ending(text);
		update_indents(text);
		text->save_status = UNSAVED;
		break;
	}
	case DELETE_RANGE:
	{
		if (!arguments->first_number || arguments->first_number > text->size)
			return INVALID_DELETE_RANGE_WAS_USED;
		if (arguments->received_numbers == 2)
		{
			if (arguments->first_number > arguments->second_number || arguments->second_number > text->size)
				return INVALID_DELETE_RANGE_WAS_USED;
			delete_range(text, arguments->first_number, arguments->second_number);
		}
		else
			delete_range(text, arguments->first_number, text->size);
		normalize_text_ending(text);
		update_indents(text);
		text->save_status = UNSAVED;
		break;
	}
	case DELETE_BRACES:
	{
		if (!arguments->received_numbers)
		{
			arguments->first_number = 1;
			arguments->second_number = text->size;
		}
		if (arguments->received_numbers == 1)
		{
			if (!arguments->first_number || arguments->first_number > text->size)
				return INVALID_LINE_NUMBER_WAS_USED;
			arguments->second_number = text->size;
		}
		if (arguments->received_numbers == 2)
			if (!arguments->first_number || arguments->first_number > text->size ||
				 arguments->first_number > arguments->second_number || arguments->second_number > text->size)
				return INVALID_DELETE_RANGE_WAS_USED;
		possible_error = text->size;
	 /* possible_error is used here just as buffer */
		if (delete_braces_from_text(text, arguments->first_number, arguments->second_number))
			return ALLOCATION_ERROR;
		if (text->size < possible_error)
		{
			normalize_text_ending(text);
			update_indents(text);
			text->save_status = UNSAVED;
		}
		break;
	}
	case READ:
	case OPEN:
	{
		if (detect_string_type(arguments->first_string) == TERM_STRING && arguments->received_strings == 1)
			return TERM_INSTEAD_OF_STRING_WAS_USED;
		if (text->save_status == UNSAVED && arguments->received_strings == 1)
			return TRYING_TO_OPEN_NEW_FILE_WITH_UNSAVED_CURRENT;
		if (text->file_association != NULL)
			buffer = text->file_association;
		if (arguments->received_strings == 2)
		{
			if (detect_string_type(arguments->second_string) == TERM_STRING)
				return TERM_INSTEAD_OF_STRING_WAS_USED;
			if (strcmp(arguments->first_string, command_arguments_keywords[FORCE]))
				return INCORRECT_ARGUMENT_TERM_WAS_USED;
			free(arguments->first_string);
			arguments->first_string = arguments->second_string;
			arguments->received_strings--; /* for proper free() after switch */
		}
		if (convert_string_argument(&(arguments->first_string)))
			return ALLOCATION_ERROR;
		deinitialize_text(text);
		initialize_empty_text(text_address);
		text = *text_address;
		if ((possible_error = load_text_from_file(text, arguments->first_string)))
		{
			if (possible_error == FAILED_TO_OPEN_FILE)
			{
				return TRYING_TO_OPEN_NONEXISTENT_FILE_OR_OPEN_ERROR;
			}
			else
				return possible_error;
		}
		if (command_code == READ)
		{
			text->save_status = UNSAVED;
			text->file_association = buffer;
		}
		else
		{
			if (text->file_association != NULL)
				free(text->file_association);
			text->file_association = arguments->first_string;
			arguments->received_strings--; /* to not free file_association after switch */
		}
		break;
	}
	case WRITE:
	{
		if (text->file_association == NULL && !arguments->received_strings)
			return NO_PATH_TO_SAVE_FILE_IN;
		if (text->save_status == SAVED)
		{
			printf_colored_attributed("editor: current file is already saved \n", YELLOW, BLACK, 2, ITALIC, BOLD);
			break;
		}
		if (arguments->received_strings)
		{
			if (convert_string_argument(&(arguments->first_string)))
				return ALLOCATION_ERROR;
			if (text->file_association == NULL)
			{
				if (!strlen(arguments->first_string))
				{
					printf_colored_attributed("editor: file name for save is an empty one \n", YELLOW, BLACK, 2, ITALIC, BOLD);
					break;
				}
				text->file_association = arguments->first_string;
				arguments->received_strings--; /* to not free file_association after switch */
			}
			else
				printf_colored_attributed("editor: association with file was already set, save was done there \n", YELLOW, BLACK, 2, ITALIC, BOLD);
		}
		if ((possible_error = save_text_to_file(text, text->file_association)))
			return possible_error;
		text->save_status = SAVED;
		break;
	}
	case HELP:
	{
		initialize_empty_text(&help);
		if ((possible_error = load_text_from_file(help, "help.txt")))
			return (possible_error == FAILED_TO_OPEN_FILE) ? MISSING_HELP_FILE_OR_OPEN_ERROR : possible_error;
		if (mode == INTERACTIVE || mode == OUTPUT_ONLY)
			print_text_in_interactive_mode(help, screen_width, screen_height);
		else
		{
			help->show_range_start = 1;
			help->show_range_end = text->size;
			print_text(help, mode, screen_width);
		}
		deinitialize_text(help);
		break;
	}
	case EXIT:
	{
		if (arguments->received_strings)
		{
			if (strcmp(arguments->first_string, command_arguments_keywords[FORCE]))
				return INCORRECT_ARGUMENT_TERM_WAS_USED;
		}
		if (text->save_status != SAVED)
		{
			if (arguments->received_strings)
				break;
			else
				return TRYING_TO_EXIT_WITHOUT_SAVE;
		}
		break;
	}
	default:
/*	EMPTY_COMMAND
	INVALID_COMMAND	*/
		break;
	}
	free_received_arguments(arguments);
	return NO_ERROR;
}