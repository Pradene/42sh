NAME     = 42sh
CC       = cc
CFLAGS   = -Wall -Wextra -Werror -Iinclude -MMD -MP -g
SRCS_DIR = src
OBJS_DIR = obj

SRCS      = $(shell find $(SRCS_DIR) -name '*.c')
OBJS      = $(patsubst $(SRCS_DIR)/%.c,$(OBJS_DIR)/%.o,$(SRCS))
DEPS      = $(OBJS:.o=.d)

all: $(NAME)

$(NAME): $(OBJS)
	@$(CC) $(CFLAGS) $(OBJS) -o $(NAME)

$(OBJS_DIR)/%.o: $(SRCS_DIR)/%.c
	@mkdir -p $(@D)
	@$(CC) $(CFLAGS) -c $< -o $@

clean:
	@rm -rf $(OBJS_DIR)

fclean: clean
	@rm -f $(NAME)

re: fclean all

-include $(DEPS)

.PHONY: all clean fclean re
