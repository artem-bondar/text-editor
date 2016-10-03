#ifndef __INTERFACE_CONTROLLER_H__
#define __INTERFACE_CONTROLLER_H__

#include "file_handler.h"

#define COMMANDS_AMOUNT 19
#define COMMAND_MAX_SIZE 2
#define COMMANDS_ARGUMENTS_KEYWORDS_AMOUNT 3

#define COMMANDS_ERRORS_GLOBAL_SHIFT 30
#define COMMAND_ERROR_MESSAGES_AMOUNT 24

typedef enum argument_receiving_order
{/* S = string 
	I = integer
	X = nothing */
	XX,
	SX,
	IX,
	SS,
	II,
	SI,
	IS,
	STRING, /* indicators of */
	INT		/* argument type */
} argument_receiving_order;

typedef enum command_codes
{					  /* argument requirements	| required receiving order */
	SET_TAB_WIDTH,    /* 1 int */
	SET_WRAP,		  /* 1 str */
	SET_NAME,		  /* 0-1 str */
	PRINT_ALL,		  /* nothing */
	PRINT_PAGES,      /* nothing */
	PRINT_RANGE,	  /* 0-1-2 int */
	PRINT_LINE,		  /* 1 int */
	EDIT_STRING,	  /* 2 int + 1 str			| II		*/
	INSERT_AFTER,	  /* 0-1 int + 1 str		| IS SX		*/
	INSERT_SYMBOL,    /* 2 int + 1 str			| II		*/
	REPLACE_SUBSTRING,/* 0-1-2 int + 2 str		| II IS SS  */
	DELETE_LINE,	  /* 1 int */
	DELETE_RANGE,	  /* 1-2 int */
	DELETE_BRACES,	  /* 0-1-2 int */
	READ,			  /* 1-2 str */
	OPEN,			  /* 1-2 str */
	WRITE,			  /* 0-1 str */
	HELP,			  /* nothing */
	EXIT,			  /* 0-1 str */
	EMPTY_COMMAND,
	INVALID_COMMAND
} command_codes;

typedef enum commands_arguments
{
	YES,
	NO,
	FORCE
} commands_arguments;

typedef struct command_arguments
{
	unsigned short received_numbers, received_strings;
	unsigned int first_number, second_number;
	char *first_string, *second_string;
	argument_receiving_order order;
} command_arguments;

typedef enum errors_commands
{
 /* are detected on reading stage */
	UNRECOGNIZED_COMMAND = 30,
	INVALID_NUMBER_ARGUMENT,
	TOO_MANY_NUMBER_ARGUMENTS,
	TOO_MANY_STRING_ARGUMENTS,
	INCORRECT_QUOTES_FORMATTING,

 /* are detected on checking stage */
	MISSING_NUMBER_ARGUMENT,
	MISSING_STRING_ARGUMENT,
	EXTRA_NUMBER_ARGUMENT,
	EXTRA_STRING_ARGUMENT,
	INCORRECT_ARGUMENTS_ORDER,

 /* are detected on executing stage */
	TRY_TO_EXECUTE_OPERATION_ON_EMPTY_TEXT,
	INCORRECT_ARGUMENT_TERM_WAS_USED,
	INCORRECT_CONTROL_CHARACTER_USED,
	TERM_INSTEAD_OF_STRING_WAS_USED,
	STRING_INSTEAD_OF_SYMBOL_WAS_USED,
	INVALID_RANGE_WAS_USED,
	INVALID_DELETE_RANGE_WAS_USED,
	INVALID_LINE_NUMBER_WAS_USED,
	INVALID_POSITION_IN_LINE_WAS_USED,
	TRYING_TO_OPEN_NEW_FILE_WITH_UNSAVED_CURRENT,
	TRYING_TO_OPEN_NONEXISTENT_FILE_OR_OPEN_ERROR,
	NO_PATH_TO_SAVE_FILE_IN,
	MISSING_HELP_FILE_OR_OPEN_ERROR,
	TRYING_TO_EXIT_WITHOUT_SAVE
} errors_commands;

typedef enum decode_status
{
	NOTHING_READ,
	READING_FIRST_KEYWORD,
	GOT_FIRST_KEYWORD,
	READING_SECOND_KEYWORD,
	READING_ARGUMENTS,
	READING_NUMBER,
	READING_STRING_TERM,
	READING_SINGLE_STRING_ARGUMENT,
	READING_MULTIPLE_STRINGS_ARGUMENT
} decode_status;

extern const char *command_keywords[COMMANDS_AMOUNT][COMMAND_MAX_SIZE];
extern const char *command_error_messages[COMMAND_ERROR_MESSAGES_AMOUNT];
extern const char *command_arguments_keywords[COMMANDS_ARGUMENTS_KEYWORDS_AMOUNT];

errors_global initialize_empty_arguments_structure(command_arguments **arguments);
void deinitialize_arguments_structure(command_arguments *arguments);
void clear_received_arguments(command_arguments *arguments);
void free_received_arguments(command_arguments *arguments);
void update_arguments_order(argument_receiving_order *order, argument_receiving_order argument_type);
void update_arguments_amount(unsigned short *amount_of_detected_arguments, command_arguments *arguments, argument_receiving_order argument_type, ...);
command_codes identify_keywords(const char *first_keyword, const char *second_keyword);
errors_commands show_input_invite(char *username, color foreground);
errors_commands show_command_error_message(const unsigned short error_code, run_mode mode);
errors_commands decode_command(char *command, char **command_address, command_codes *command_code, command_arguments *arguments);
errors_commands check_command_arguments(command_codes command_code, command_arguments const *arguments);
errors_commands execute_command(command_codes command_code, command_arguments *arguments, Text *text, Text **text_address, run_mode mode, const unsigned int screen_width, const unsigned int screen_height);

#endif /* __INTERFACE_CONTROLLER_H__ */