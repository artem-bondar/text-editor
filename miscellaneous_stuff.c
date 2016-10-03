/*
 * Bundle of some functions of global purpose
 * for operating with colors and strings
 *
 * author Artem Bondar (c) 2016
 */

#define _BSD_SOURCE /* usleep() macro requirement for glibc */

#include <unistd.h>

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>

#include "miscellaneous_stuff.h"

const char *title[TITLE_SIZE] =
{
	" _______   _____      _       _ ",
	"(__   __) |  ___)    /\\\\     / |",
	"   | |    | |__     /  \\\\    | |",
	"   | |    |  __)   / /\\ \\\\   |_/",
	"   | | _  | |__   / /__\\ \\\\   _ ",
	"   |_||_| |_____)/________\\\\ |_|",
	"",
	"  Created by Artem Bondar © 2016"
};

void set_colors(const color foreground, const color background)
{
	printf("\033[%d;%dm", 30 + foreground, 40 + background);
}

void set_attributes(const graphic_rendering_attributes first_attribute, const graphic_rendering_attributes second_attribute)
{
	printf("\033[%u;%um", first_attribute, second_attribute);
}

void reset_colors()
{
	printf("\033[0;37;40m");
}

void printf_colored(char const *string, const color foreground, const color background)
{
	printf("\033[%d;%dm%s\033[0;37;40m", 30 + foreground, 40 + background, string);
}

void printf_colored_attributed(char const *string, const color foreground, const color background, unsigned int attributes_number, ...)
{
	unsigned int attribute;

	if (attributes_number)
	{
		va_list argument_list;
		va_start(argument_list, attributes_number);
		while (attributes_number)
		{
			attribute = va_arg(argument_list, unsigned int);
			printf("\033[%um", attribute);
			attributes_number--;
		}
		va_end(argument_list);
	}
	printf_colored(string, foreground, background);
}

void show_loading_screen(const unsigned short indent, const unsigned short vertical_indent)
{
	int i, j;
	color load_color;

	printf("\033[?25l\033[2J\033[1;0H\033[%dB", vertical_indent);
	for (i = 0; i < TITLE_SIZE; i++)
	{
		printf("%*s", indent, "");
		printf_colored(title[i], YELLOW, BLACK);
		printf("\n");
	}
	putchar('\n');
	for (i = 0; i <= 100; i++)
	{
		printf("%*s  ", indent, "");
		load_color = (i > 33) ? (i > 66) ? GREEN : YELLOW : RED;
		for (j = i / 4; j > 0; j--)
		{
			printf_colored("█", load_color, BLACK);
		}
		if (i % 2 == 1)
			printf_colored("▌", load_color, BLACK);
		printf("%3d%%\r", i);
		usleep(30000);
	}
	usleep(100000);
	printf("%*s", indent, "");
	printf_colored("  █████████████████████████ DONE \n", GREEN, BLACK);
	usleep(500000);
	printf("\033[;H\033[J\033[?25h");
}

void calculate_indents(unsigned short *indent, unsigned short *vertical_indent, const unsigned short width, const unsigned short height)
{
	*indent = ((width - TITLE_WIDTH) / 2) > 0 ? (width - TITLE_WIDTH) / 2 : 0;
	*vertical_indent = ((height - LOAD_SCREEN_SIZE) / 2) > 0 ? (height - LOAD_SCREEN_SIZE) / 2 : 0;
}

const unsigned int log10_of(unsigned int number)
{
	unsigned int log10 = 0;
	while (number)
	{
		number /= 10;
		log10++;
	}
	return log10;
}

const unsigned int is_second_part_of_control_character(const char character)
{
	return (character == '0' || character == 'a' || character == 'b' ||
			character == 't' || character == 'n' || character == 'v' ||
			character == 'f' || character == 'r' || character == '\\' ||
			character == '\'' || character == '\"');
}

const unsigned int get_control_character_by_its_second_part(const char character)
{
	switch (character)
	{
	case '0':
		return '\0';
	case 'a':
		return '\a';
	case 'b':
		return '\b';
	case 't':
		return '\t';
	case 'n':
		return '\n';
	case 'v':
		return '\v';
	case 'f':
		return '\f';
	case 'r':
		return '\r';
	case '\\':
		return '\\';
	case '\'':
		return '\'';
	case '\"':
		return '\"';
	default:
		return '\0';
	}
}

unsigned int detect_string_type(char const *string)
{
	return (string[0] == '\"') ? (strlen(string) > 2 && string[1] == '\"') ? MULTIPLE_STRING : SINGLE_STRING : TERM_STRING;
}

unsigned int detect_number_of_strings_in_line(char *line)
{
	unsigned int strings_amount = 0;
	char *position_of_LF, *start_position = line, *end_of_string = strchr(line, '\0');
	for (;;)
	{
		position_of_LF = strchr(start_position, '\n');
		if (position_of_LF != NULL)
			strings_amount++;
		else
			break;
		position_of_LF++;
		start_position = position_of_LF;
		if (position_of_LF == end_of_string)
			break;
	}
	if (start_position < end_of_string && start_position != line)
		strings_amount++;
	return strings_amount;
}

unsigned int detect_number_of_substrings_in_string(char *string, char *substring)
{
	unsigned int substrings_amount = 0;
	char *search_start = string, *end_of_string = strchr(string, '\0');;
	
	do
	{
		search_start = strstr(search_start, substring);
		if (search_start != NULL)
		{
			substrings_amount++;
			search_start++;
			if (search_start == end_of_string)
				break;
		}
	} while (search_start != NULL);
	return substrings_amount;
}

errors_global convert_string_argument(char **string)
{
	char *buffer;
	unsigned int i, j, amount_of_symbols_to_omit, string_type_shift, string_length = strlen(*string);
	string_type_shift = detect_string_type(*string);
	amount_of_symbols_to_omit = string_type_shift * 2;
	for (i = string_type_shift; i < string_length - string_type_shift; i++)
		if ((*string)[i] == '\\' && i + 1 < string_length - string_type_shift)
			if (is_second_part_of_control_character((*string)[i + 1]))
			{
				amount_of_symbols_to_omit++;
				i++;
				continue;
			}
	buffer = (char*)malloc(sizeof(char) * (string_length - amount_of_symbols_to_omit + 1 /* for \0 */));
	if (buffer == NULL)
		return ALLOCATION_ERROR;
	for (i = string_type_shift, j = 0; i < string_length - string_type_shift; i++, j++)
	{
		if ((*string)[i] == '\\' && i + 1 < string_length - string_type_shift)
			if (is_second_part_of_control_character((*string)[i + 1]))
			{
				buffer[j] = get_control_character_by_its_second_part((*string)[i + 1]);
				i++;
				continue;
			}
		buffer[j] = (*string)[i];
	}
	buffer[j] = '\0';
	free(*string);
	*string = buffer;
	return NO_ERROR;
}