#include <stdlib.h>
#include <string.h>

#include "text_database.h"

errors_global initialize_empty_text(Text **text)
{
	*text = (Text*)malloc(sizeof(Text));
	if (*text == NULL)
		return ALLOCATION_ERROR;
	(*text)->file_association = NULL;
	(*text)->save_status = SAVED;
	(*text)->first_line = NULL;
	(*text)->last_line = NULL;
	(*text)->size = 0;
	(*text)->show_range_start = 1;
	(*text)->show_range_end = 0;
	(*text)->tab_width = DEFAULT_TAB_WIDTH;
	(*text)->wrap = CONTINUE;
	(*text)->number_indent = DEFAULT_NUMBER_INDENT;
	(*text)->interface_indent = DEFAULT_INTERFACE_SIZE + (*text)->number_indent;
	(*text)->shift = 0;
	(*text)->color_scheme.left_column = YELLOW;
	(*text)->color_scheme.line_numbers = CYAN;
	(*text)->color_scheme.text = WHITE;
	(*text)->color_scheme.background = BLACK;
	return NO_ERROR;
}

void deinitialize_text(Text *text)
{
	Line *previous_line;
	Line *current_line = text->first_line;
	while (current_line != NULL)
	{
		free(current_line->content);
		previous_line = current_line;
		current_line = current_line->next_line;
		free(previous_line);
	}
	if (text->file_association != NULL)
		free(text->file_association);
	free(text);
}

void update_indents(Text *text)
{
	unsigned int new_indent = log10_of(text->size);
	text->number_indent = (new_indent > 2) ? new_indent : 2;
	text->interface_indent = text->number_indent + DEFAULT_INTERFACE_SIZE;
}

void insert_line_to_first_position(Text *text, Line *line)
{
	text->size++;
	if (text->first_line == NULL)
	{
		text->first_line = text->last_line = line;
		line->next_line = NULL;
	 /* for correct insert block of lines */
	}
	else
	{
		line->next_line = text->first_line;
		text->first_line = line;
	}
}

void insert_line_to_last_position(Text *text, Line *line)
{
	text->size++;
	if (text->last_line == NULL)
		text->first_line = text->last_line = line;
	else
	{
		text->last_line->next_line = line;
		text->last_line = line;
	}
}

void insert_block_of_lines_after_position(Text *text, Line *block_start, const unsigned int position)
{
	Line *start_line, *current_line, *buffer;

	if (!position)
	{
		start_line = current_line = block_start->next_line;
		insert_line_to_first_position(text, block_start);
		if (current_line != NULL)
		{
			text->size++;
			while (current_line->next_line != NULL)
			{
				current_line = current_line->next_line;
				text->size++;
			}
		}
		buffer = block_start->next_line;
		block_start->next_line = start_line;
		if (current_line != NULL)
			current_line->next_line = buffer;
		if (buffer == NULL)
			text->last_line = (current_line != NULL) ? current_line : block_start;
		return;
	}
	if (position == text->size)
	{
		insert_line_to_last_position(text, block_start);
		current_line = block_start;
		while (current_line->next_line != NULL)
		{
			current_line = current_line->next_line;
			text->size++;
		}
		text->last_line = current_line;
		return;
	}
	if (position && position < text->size)
	{
		iterate_line_pointer(text, &start_line, position);
		buffer = start_line->next_line;
		start_line->next_line = block_start;
		current_line = block_start;
		text->size++;
		while (current_line->next_line != NULL)
		{
			current_line = current_line->next_line;
			text->size++;
		}
		current_line->next_line = buffer;
	}
}

void iterate_line_pointer(Text const *text, Line **current_line, unsigned int i)
{
	*current_line = text->first_line;
	while (i > 1)
	{
		*current_line = (*current_line)->next_line;
	  /* received iteration value is required to be valid */
		i--;
	}
}

errors_global edit_line(Text *text, const unsigned int line_number, const unsigned int position, char replace_symbol)
{
	unsigned int string_length;
	Line *line;
	iterate_line_pointer(text, &line, line_number);
	string_length = strlen(line->content);
	if (!position || position > string_length || (position >= string_length && line->content[string_length - 1]) == '\n')
		return INVALID_POSITION_IN_LINE;
	line->content[position - 1] = replace_symbol;
	return NO_ERROR;
}

errors_global insert_symbol(Text *text, const unsigned int line_number, unsigned int position, char insert_symbol)
{
	unsigned int string_length;
	char *edited_string;
	Line *line;

	iterate_line_pointer(text, &line, line_number);
	string_length = strlen(line->content);
	edited_string = (char*)malloc(sizeof(char) * (string_length + 2));
	if (edited_string == NULL)
		return ALLOCATION_ERROR;
	position = (!position) ? 0 : (position > string_length) ? (line->content[string_length - 1] == '\n') ?
			   (string_length > 1) ? (line->content[string_length - 2] == '\r') ?
				string_length - 2 : string_length - 1 : string_length - 1 : string_length : position - 1;
	strncpy(edited_string, line->content, position);
	edited_string[position] = insert_symbol;
	strncpy(edited_string + (position + 1) * sizeof(char), line->content + position * sizeof(char), string_length - position);
	edited_string[string_length + 1] = '\0';
	free(line->content);
	line->content = edited_string;
	return NO_ERROR;
}

void delete_line(Text *text, const unsigned int line_number)
{
	Line *line, *buffer;

	if (line_number > 1)
	{
		iterate_line_pointer(text, &line, line_number - 1);
		buffer = line->next_line->next_line;
		free(line->next_line->content);
		if (line->next_line == text->last_line)
			text->last_line = line;
		free(line->next_line);
		line->next_line = buffer;
	}
	else
	{
		buffer = text->first_line;
		text->first_line = text->first_line->next_line;
		free(buffer->content);
		free(buffer);
	}
	text->size--;
}

void delete_range(Text *text, const unsigned int range_start, const unsigned int range_end)
{
	int delete_amount = range_end - range_start + 1;
	Line *start_line, *current_line, *buffer;

	iterate_line_pointer(text, &start_line, (range_start > 0) ? range_start - 1 : 0);
	current_line = (text->first_line == start_line) ? start_line : start_line->next_line;
	while (delete_amount)
	{
		buffer = current_line->next_line;
		if (current_line == text->first_line)
			start_line = text->first_line = buffer;
		free(current_line->content);
		free(current_line);
		text->size--;
		if (buffer == NULL)
		{
			text->last_line = start_line;
			break;
		}
		current_line = buffer;
		delete_amount--;
	}
	if (range_start > 1)
		start_line->next_line = buffer;
}

errors_global transform_multiply_strings_in_lines(char *string, Line **result)
{
	unsigned int length_from_start_position, length_from_finded_position;
	char *position_of_LF, *start_position = string, *end_of_string = strchr(string, '\0');
	Line *previous_line = NULL, *current_line, *first_line;

	for (;;)
	{
		current_line = (Line*)malloc(sizeof(Line));
		if (current_line == NULL)
			return ALLOCATION_ERROR;
		length_from_start_position = strlen(start_position);
		position_of_LF = strchr(start_position, '\n');
		if (position_of_LF == NULL)
		{
			current_line->content = (char*)malloc(sizeof(char) * (length_from_start_position + 1));
			if (current_line->content == NULL)
				return ALLOCATION_ERROR;
			strncpy(current_line->content, start_position, sizeof(char) * length_from_start_position);
			current_line->content[length_from_start_position] = '\0';
			current_line->next_line = NULL;
			if (previous_line != NULL)
				previous_line->next_line = current_line;
			else
				first_line = current_line;
			break;
		}
		else
		{
			length_from_finded_position = strlen(position_of_LF);
			current_line->content = (char*)malloc(sizeof(char) * (length_from_start_position - length_from_finded_position + 2));
			if (current_line->content == NULL)
				return ALLOCATION_ERROR;
			strncpy(current_line->content, start_position, sizeof(char) * (length_from_start_position - length_from_finded_position + 1));
			current_line->content[length_from_start_position - length_from_finded_position + 1] = '\0';
			if (previous_line != NULL)
				previous_line->next_line = current_line;
			else
				first_line = current_line;
			previous_line = current_line;
			start_position = position_of_LF + sizeof(char);
			if (start_position == end_of_string)
			{
				current_line->next_line = NULL;
				break;
			}
		}
	}
	*result = first_line;
	return NO_ERROR;
}

void normalize_text_ending(Text *text)
{
	unsigned int line_length;

	if (!text->size)
		return;
	line_length = strlen(text->last_line->content);
	if (text->last_line->content[line_length - 1] == '\n')
		text->last_line->content[line_length - 1] = '\0';
	if (line_length > 1)
		if (text->last_line->content[line_length - 2] == '\r')
			text->last_line->content[line_length - 2] = '\0';
}

errors_global normalize_lines_endings(Text *text)
{
	unsigned int line_length;
	char *buffer;
	Line *current_line = text->first_line;

	if (!text->size)
		return NO_ERROR;
	while (current_line != text->last_line)
	{
		line_length = strlen(current_line->content);
		if (current_line->content[line_length - 1] != '\n')
		{
			buffer = (char*)malloc(sizeof(char) * (line_length + 3));
			if (buffer == NULL)
				return ALLOCATION_ERROR;
			strcpy(buffer, current_line->content);
			buffer[line_length] = '\r';
			buffer[line_length + 1] = '\n';
			buffer[line_length + 2] = '\0';
			free(current_line->content);
			current_line->content = buffer;
		}
		current_line = current_line->next_line;
	}
	line_length = strlen(current_line->content);
	if (current_line->content[line_length - 1] == '\n')
		current_line->content[line_length - 1] = '\0';
	return NO_ERROR;
}

errors_global expand_duplicated_lines(Text *text)
{
	unsigned int strings_amount, line_number = 0;
	Line *current_line = text->first_line, *block_of_lines;

	while (line_number + 1 <= text->size)
	{
		strings_amount = detect_number_of_strings_in_line(current_line->content);
		if (strings_amount > 1 && current_line != text->last_line)
		{
			if (transform_multiply_strings_in_lines(current_line->content, &block_of_lines))
				return ALLOCATION_ERROR;
			current_line = current_line->next_line;
			delete_line(text, line_number + 1);
			insert_block_of_lines_after_position(text, block_of_lines, line_number);
			line_number += strings_amount;
			continue;
		}
		if (strings_amount > 1 && current_line == text->last_line)
		{
			if (transform_multiply_strings_in_lines(current_line->content, &block_of_lines))
				return ALLOCATION_ERROR;
			delete_line(text, text->size);
			insert_block_of_lines_after_position(text, block_of_lines, text->size);
			break;
		}
		current_line = current_line->next_line;
		line_number++;
	}
	return NO_ERROR;
}

errors_global delete_braces_from_text(Text *text, const unsigned int range_start, const unsigned int recommended_range_end)
{
	unsigned int delete_range_start = range_start, delete_range_end, delete_final_range_end = recommended_range_end,
				 amount_of_open_braces = 0,
				 length_from_start_position, length_from_finded_position,
				 first_line_part_length, second_line_part_length;
	char *first_line_part, *second_line_part,
		 *position_of_open_brace, *position_of_close_brace, *next_position_of_close_brace,
		*search_start_position, *end_of_string;
	Line *current_line;

	for (;;)
	{
		iterate_line_pointer(text, &current_line, delete_range_start);
		while (!amount_of_open_braces)
		{
			position_of_open_brace = strchr(current_line->content, '{');
			if (position_of_open_brace == NULL)
			{
				delete_range_start++;
				if (current_line->next_line != NULL && delete_range_start <= delete_final_range_end)
					current_line = current_line->next_line;
				else
					break;
			}
			else
			{
				amount_of_open_braces++;
				length_from_start_position = strlen(current_line->content);
				length_from_finded_position = strlen(position_of_open_brace);
				first_line_part_length = length_from_start_position - length_from_finded_position;
				if (first_line_part_length)
				{
					first_line_part = (char*)malloc(sizeof(char) * (first_line_part_length + 1));
					if (first_line_part == NULL)
						return ALLOCATION_ERROR;
					strncpy(first_line_part, current_line->content, sizeof(char) * first_line_part_length);
					first_line_part[first_line_part_length] = '\0';
				}
			}
		}
		if (!amount_of_open_braces)
			return NO_ERROR;
		delete_range_end = delete_range_start;
		end_of_string = strchr(current_line->content, '\0');
		search_start_position = position_of_open_brace + sizeof(char);
		if (search_start_position == end_of_string)
		{
			if (current_line->next_line != NULL)
			{
				delete_range_end++;
				current_line = current_line->next_line;
				search_start_position = current_line->content;
				end_of_string = strchr(current_line->content, '\0');
			}
			else
				amount_of_open_braces--;
		}
		while (amount_of_open_braces)
		{
			position_of_open_brace = strchr(search_start_position, '{');
			while (position_of_open_brace != NULL)
			{
				amount_of_open_braces++;
				position_of_open_brace++;
				if (position_of_open_brace == end_of_string)
					break;
				position_of_open_brace = strchr(position_of_open_brace, '{');
			}
			next_position_of_close_brace = position_of_close_brace = strchr(search_start_position, '}');
			while (next_position_of_close_brace != NULL && amount_of_open_braces)
			{
				position_of_close_brace = next_position_of_close_brace;
				amount_of_open_braces--;
				position_of_close_brace++;
				if (position_of_close_brace == end_of_string)
					break;
				next_position_of_close_brace = strchr(position_of_close_brace, '}');
			}
			if (!amount_of_open_braces)
			{
				if (position_of_close_brace == end_of_string)
					second_line_part_length = 0;
				else
				{
					second_line_part_length = strlen(position_of_close_brace);
					second_line_part = (char*)malloc(sizeof(char) * (second_line_part_length + 1));
					if (second_line_part == NULL)
					{
						if (first_line_part_length)
							free(first_line_part);
						return ALLOCATION_ERROR;
					}
					strncpy(second_line_part, position_of_close_brace, sizeof(char) * second_line_part_length);
					second_line_part[second_line_part_length] = '\0';
				}
			}
			else
			{
				if (current_line->next_line != NULL)
				{
					delete_range_end++;
					current_line = current_line->next_line;
					search_start_position = current_line->content;
					end_of_string = strchr(current_line->content, '\0');
				}
				else
				{
					second_line_part_length = 0;
					break;
				}
			}
		} /* end of second while */
		delete_range(text, delete_range_start, delete_range_end);
		if (first_line_part_length && second_line_part_length)
		{
			end_of_string = (char*)realloc(first_line_part, first_line_part_length + second_line_part_length + 1);
		 /* end_of_string used here just as buffer */
			if (end_of_string == NULL)
				return ALLOCATION_ERROR;
			first_line_part = end_of_string;
			strcat(first_line_part, second_line_part);
			free(second_line_part);
		}
		else
			if (second_line_part_length)
			{
				first_line_part_length = second_line_part_length;
				first_line_part = second_line_part;
			}
		if (first_line_part_length)
		{
			current_line = (Line*)malloc(sizeof(Line));
			if (current_line == NULL)
			{
				free(first_line_part);
				return ALLOCATION_ERROR;
			}
			current_line->content = first_line_part;
			current_line->next_line = NULL;
			insert_block_of_lines_after_position(text, current_line, delete_range_start - 1);
		}
		if (delete_range_end < delete_final_range_end)
		{
			delete_range_start--;
			delete_final_range_end -= delete_range_end - delete_range_start;
		}
		else
			break;
	} /* end of for */
	return NO_ERROR;
}

errors_global add_string_to_range(const unsigned int add_mode, Text *text, char *line, const unsigned int range_start, const unsigned int range_end)
{
	unsigned int i, line_length = strlen(line), current_line_length;
	char *extended_line;
	Line *current_line;

	iterate_line_pointer(text, &current_line, range_start);
	for (i = range_start; i <= range_end; i++, current_line = current_line->next_line)
	{
		current_line_length = strlen(current_line->content);
		extended_line = (char*)malloc(sizeof(char) * (line_length + current_line_length + 1));
		if (extended_line == NULL)
			return ALLOCATION_ERROR;
		if (current_line->content[current_line_length - 1] == '\n' && add_mode == AFTER_LINE)
			current_line->content[current_line_length - 1] = '\0';
		if (current_line_length > 1)
		{
			if (current_line->content[current_line_length - 2] == '\r' && add_mode == AFTER_LINE)
				current_line->content[current_line_length - 2] = '\0';
		}
		strcpy(extended_line, (add_mode == BEFORE_LINE) ? line : current_line->content);
		strcat(extended_line, (add_mode == BEFORE_LINE) ? current_line->content : line);
		free(current_line->content);
		current_line->content = extended_line;
	}
	return NO_ERROR;
}

const unsigned short replace_substring_in_range(Text *text, char *substring, char *replacement_pattern, const unsigned int range_start, const unsigned int range_end)
{
/*  returns amount of replaces + REPLACES_COUNT_DELTA on success or error code if error ocurred
	REPLACES_COUNT_DELTA is used to definitely not override error codes with amount of replaces */

	unsigned short total_replaces_counts = 0;
	unsigned int substrings_amount, i, result_string_length,
				 substring_length = strlen(substring), replacement_pattern_length = strlen(replacement_pattern);
	int delta = replacement_pattern_length - substring_length;
	char *result_string, *end_of_string, *search_start, *substring_start;
	
	Line *current_line;

	iterate_line_pointer(text, &current_line, range_start);
	for (i = range_start; i <= range_end; i++, current_line = current_line->next_line)
	{
		substrings_amount = detect_number_of_substrings_in_string(current_line->content, substring);
		if (substrings_amount)
		{
			total_replaces_counts += substrings_amount;
			end_of_string = strchr(current_line->content, '\0');
			result_string_length = strlen(current_line->content) + delta * substrings_amount;
			result_string = (char*)malloc(sizeof(char) * (result_string_length + 1));
			result_string[0] = '\0';
			if (result_string == NULL)
				return ALLOCATION_ERROR;
			search_start = current_line->content;
			while (search_start != NULL)
			{
				if (search_start == end_of_string)
					break;
				substring_start = strstr(search_start, substring);
				if (substring_start != NULL)
				{
					strncat(result_string, search_start, strlen(search_start) - strlen(substring_start));
					strcat(result_string, replacement_pattern);
				}
				else
				{
					if (search_start != current_line->content)
						strcat(result_string, search_start);
				}
				if (substring_start != NULL)
					search_start = substring_start + sizeof(char) * substring_length;
				else
					search_start = NULL;
			}
			result_string[result_string_length] = '\0';
			free(current_line->content);
			current_line->content = result_string;
		}
	}
	return total_replaces_counts + REPLACES_COUNT_DELTA;
}