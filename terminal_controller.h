#ifndef __TERMINAL_CONTROLLER_H__
#define __TERMINAL_CONTROLLER_H__

#define MIN_TERMINAL_WIDTH 32
#define MIN_TERMINAL_HEIGHT 2

#define DEFAULT 0
#define CHARACTER_BY_CHARACTER 1

typedef enum errors_terminal
{
	FAILED_TO_GET_TERMINAL_SIZE = 10,
	FAILED_TO_GET_TERMINAL_ATTRIBUTES,
	FAILED_TO_SET_TERMINAL_ATTRIBUTES,
	TERMINAL_SIZE_TOO_SMALL
} errors_terminal;

typedef enum run_mode
{
	INTERACTIVE,
	INPUT_ONLY,
	OUTPUT_ONLY,
	AUTONOMOUS
} run_mode;

errors_terminal detect_environment(char **username, run_mode *mode);
errors_terminal get_terminal_size(unsigned short *screen_width, unsigned short *screen_height);
errors_terminal change_terminal_input_mode_to(const unsigned int mode);

#endif /* __TERMINAL_CONTROLLER_H__ */