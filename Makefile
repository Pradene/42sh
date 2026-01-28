NAME     = 42sh
CC       = cc
CFLAGS   = -Wall -Wextra -Werror -Iinclude -MMD -MP -g
SRCS_DIR = src
OBJS_DIR = obj

SRCS      = $(shell find $(SRCS_DIR) -name '*.c')
OBJS      = $(patsubst $(SRCS_DIR)/%.c,$(OBJS_DIR)/%.o,$(SRCS))
DEPS      = $(OBJS:.o=.d)

all: $(NAME)
	@echo "\033[1;32m[OK]\033[0m Build complete: $(NAME)"

$(NAME): $(OBJS)
	@echo "\033[1;34m[LINK]\033[0m Creating executable: $(NAME)"
	@$(CC) $(CFLAGS) $(OBJS) -lreadline -o $(NAME)

$(OBJS_DIR)/%.o: $(SRCS_DIR)/%.c
	@mkdir -p $(@D)
	@echo "\033[1;36m[CC]\033[0m $<"
	@$(CC) $(CFLAGS) -c $< -o $@

clean:
	@echo "\033[1;31m[CLEAN]\033[0m Removing object files"
	@rm -rf $(OBJS_DIR)

fclean: clean
	@echo "\033[1;31m[FCLEAN]\033[0m Removing binaries"
	@rm -f $(NAME)

re: fclean all

-include $(DEPS)

.PHONY: all clean fclean re
