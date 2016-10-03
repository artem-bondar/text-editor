#include <unistd.h>
#include <termios.h>
#include <sys/ioctl.h>

#include <string.h>

#include "terminal_controller.h"
#include "miscellaneous_stuff.h"

static struct termios default_terminal_attributes, character_by_character_terminal_attributes;

errors_terminal detect_environment(char **username, run_mode *mode)
{
	char *login = getlogin();
 /* getpwuid instead*/
	if (login != NULL)
		*username = login;
	if (isatty(0))
	{
		if (isatty(1))
			*mode = INTERACTIVE;
		else
			*mode = INPUT_ONLY;
	}
	else
	{
		if (isatty(1))
			*mode = OUTPUT_ONLY;
		else
			*mode = AUTONOMOUS;
	}
	if (*mode == INTERACTIVE || *mode == INPUT_ONLY)
		if (tcgetattr(0, &default_terminal_attributes))
			return FAILED_TO_GET_TERMINAL_ATTRIBUTES;
	memcpy(&character_by_character_terminal_attributes, &default_terminal_attributes, sizeof(struct termios));
	character_by_character_terminal_attributes.c_lflag &= ~(ECHO | ICANON);
	character_by_character_terminal_attributes.c_cc[VMIN] = 1;
	return NO_ERROR;
}

errors_terminal get_terminal_size(unsigned short *screen_width, unsigned short *screen_height)
{
	struct winsize window_sizes;

	if (ioctl(0, TIOCGWINSZ, &window_sizes) && ioctl(1, TIOCGWINSZ, &window_sizes))
		return FAILED_TO_GET_TERMINAL_SIZE;
	if (window_sizes.ws_col < MIN_TERMINAL_WIDTH)
	{
	 /* forces user to use terminal width bigger or equal to load screen width */
		printf_colored_attributed("editor: Terminal width is too small to run this program! \n", RED, BLACK, 2, ITALIC, BOLD);
		return TERMINAL_SIZE_TOO_SMALL;
	}
	if (window_sizes.ws_row < MIN_TERMINAL_HEIGHT)
	{
	 /* forces user to use terminal height bigger or equal to load screen height */
		printf_colored_attributed("editor: Terminal height is too small to run this program! \n", RED, BLACK, 2, ITALIC, BOLD);
		return TERMINAL_SIZE_TOO_SMALL;
	}
	*screen_width = window_sizes.ws_col;
	*screen_height = window_sizes.ws_row;

	return NO_ERROR;
}

errors_terminal change_terminal_input_mode_to(const unsigned int mode)
{
	if (mode == DEFAULT)
		if (tcsetattr(0, TCSANOW, &default_terminal_attributes))
			return FAILED_TO_SET_TERMINAL_ATTRIBUTES;
	if (mode == CHARACTER_BY_CHARACTER)
		if (tcsetattr(0, TCSANOW, &character_by_character_terminal_attributes))
			return FAILED_TO_SET_TERMINAL_ATTRIBUTES;
	return NO_ERROR;
}